#include "spx_cgi_chunked.hpp"
#include "spx_cgi_module.hpp"
#include "spx_kqueue_module.hpp"

#include "spx_session_storage.hpp"

bool
CgiField::cgi_handler_(ReqField& req, event_list_t& change_list, struct kevent* cur_event) {
	int	  write_to_cgi[2];
	int	  read_from_cgi[2];
	pid_t pid;

	if (pipe(write_to_cgi) == -1) {
		// pipe error
		std::cerr << "pipe error" << std::endl;
		return false;
	}
	if (pipe(read_from_cgi) == -1) {
		// pipe error
		close(write_to_cgi[0]);
		close(write_to_cgi[1]);
		std::cerr << "pipe error" << std::endl;
		return false;
	}
	pid = fork();
	if (pid < 0) {
		// fork error
		close(write_to_cgi[0]);
		close(write_to_cgi[1]);
		close(read_from_cgi[0]);
		close(read_from_cgi[1]);
		return false;
	}
	if (pid == 0) {
		// child. run cgi
		dup2(write_to_cgi[0], STDIN_FILENO);
		close(write_to_cgi[1]);
		close(read_from_cgi[0]);
		dup2(read_from_cgi[1], STDOUT_FILENO);
		// set_cgi_envp()
		CgiModule cgi(req._uri_resolv, req._header, req._uri_loc);

		cgi.made_env_for_cgi_(req._req_mthd);

		char const* script[3];
		script[0] = req._uri_resolv.cgi_loc_->cgi_path_info.c_str();
		script[1] = req._uri_resolv.script_filename_.c_str();
		script[2] = NULL;

		execve(script[0], const_cast<char* const*>(script),
			   const_cast<char* const*>(&cgi.env_for_cgi_[0]));
		exit(EXIT_FAILURE);
	}
	// parent
	close(write_to_cgi[0]);
	if (req._req_mthd & (REQ_GET | REQ_HEAD | REQ_DELETE)) {
		close(write_to_cgi[1]);
	} else {
		fcntl(write_to_cgi[1], F_SETFL, O_NONBLOCK);
		add_change_list(change_list, write_to_cgi[1], EVFILT_WRITE, EV_ADD | EV_DISABLE, 0, 0, cur_event->udata);
		_write_to_cgi_fd = write_to_cgi[1];
	}
	close(read_from_cgi[1]);
	fcntl(read_from_cgi[0], F_SETFL, O_NONBLOCK);
	add_change_list(change_list, read_from_cgi[0], EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, cur_event->udata);
	add_change_list(change_list, pid, EVFILT_PROC, EV_ADD, NOTE_EXIT, 0, cur_event->udata);

	return true;
}

bool
ChunkedField::chunked_body_can_parse_chnkd_(Client& cl) {
	// spx_log_("cl._buf.buf_size_", cl._buf.buf_size_());
	if (_chnkd_size > 2) {
		_chnkd_size -= cl._buf.move_(_chnkd_body, _chnkd_size - 2);
		if (_chnkd_size != 2) {
			return true;
		}
	}
	// spx_log_("chunked_body_can_parse_chnkd");
	if (cl._buf.buf_size_() >= 2) {
		_chnkd_size -= cl._buf.delete_size_(2);
		if (_last_chnkd) {
			// chunked last
			cl._req._cnt_len = cl._req._body_size;
			cl._state		 = REQ_HOLD;
			return true;
		}
		return true;
	}
	spx_log_("chunked try next");
	return false;
}

bool
ChunkedField::chunked_body_can_parse_chnkd_skip_(Client& cl) {
	if (_chnkd_size > 2) {
		_chnkd_size -= cl._buf.delete_size_(_chnkd_size - 2);
		if (_chnkd_size != 2) {
			return true;
		}
	}
	if (cl._res._write_finished && cl._buf.buf_size_() >= 2) {
		// if (cl._buf.find_pos_(CR) != 0 || cl._buf.find_pos_(LF) != 1) {
		// 	// chunked error
		// 	throw(std::exception());
		// }
		_chnkd_size -= cl._buf.delete_size_(2);
		if (_last_chnkd) {
			// chunked last
			cl._req._cnt_len = cl._req._body_size;
			cl._state		 = REQ_CLEAR;
			return true;
		}
		return true;
	}
	spx_log_("chunked try next");
	return false;
}

bool
ChunkedField::chunked_body_(Client& cl) {
	std::string len;
	int			start_line_end;

	if (_first_chnkd) {
		_first_chnkd = false;
		if (cl._req._uri_resolv.is_cgi_) {
			add_change_list(*cl.change_list, cl._cgi._write_to_cgi_fd, EVFILT_WRITE, EV_ENABLE, 0, 0, &cl);
		} else {
			add_change_list(*cl.change_list, cl._req._body_fd, EVFILT_WRITE, EV_ENABLE, 0, 0, &cl);
		}
	}
	// if (cl._req._uri_resolv.is_cgi_) {
	// 	if (cl._cgi._to_cgi.buf_size_()) {
	// 		add_change_list(*cl.change_list, cl._cgi._write_to_cgi_fd, EVFILT_WRITE, EV_ENABLE, 0, 0, &cl);
	// 	}
	// } else {
	// 	if (cl._req._body_buf.buf_size_()) {
	// 		add_change_list(*cl.change_list, cl._req._body_fd, EVFILT_WRITE, EV_ENABLE, 0, 0, &cl);
	// 	}
	// }
	spx_log_("controller - req body chunked.");
	try {
		while (true) {
			if (_chnkd_size) {
				if ((chunked_body_can_parse_chnkd_(cl) == false) || _chnkd_size) {
					return false;
				}
				if (_last_chnkd) {
					return true;
				}
			}
			len.clear();
			if (cl._buf.get_crlf_line_(len) == true) {
				spx_log_("size str len", len);
				if (spx_chunked_syntax_start_line(len, _chnkd_size, cl._req._header) == spx_ok) {
					add_change_list(*cl.change_list, cl._client_fd, EVFILT_READ, EV_ENABLE, 0, _chnkd_size - cl._buf.buf_size_(), &cl);
					spx_log_("size str _chnkd_size", _chnkd_size);
					spx_log_("_req._body_size", cl._req._body_size);
					spx_log_("_req._body_limit", cl._req._body_limit);
					if (_chnkd_size == 0) {
						_last_chnkd = true;
					}
					cl._req._body_size += _chnkd_size;
					_chnkd_size += 2;
					if (cl._req._body_size > cl._req._body_limit) {
						throw(std::exception());
					}
					continue;
				} else {
					// chunked error
					spx_log_("chunked error. len", len);
					throw(std::exception());
				}
			} else {
				// chunked start line not exist.
				return false;
			}
		}
	} catch (...) {
		if (cl._req._body_size > cl._req._body_limit) {
			spx_log_("send over limit");
			// send over limit.
			close(cl._req._body_fd);
			remove(cl._req._upld_fn.c_str());
			add_change_list(*cl.change_list, cl._req._body_fd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
			cl.error_response_keep_alive_(HTTP_STATUS_PAYLOAD_TOO_LARGE);
			cl._state = REQ_SKIP_BODY_CHUNKED;
		} else {
			cl._res.make_error_response_(cl, HTTP_STATUS_BAD_REQUEST);
			if (cl._req._body_fd != -1) {
				add_change_list(*cl.change_list, cl._req._body_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, &cl);
			}
			cl._state = E_BAD_REQ;
		}
		return false;
	}
}

bool
ChunkedField::skip_chunked_body_(Client& cl) {
	std::string len;
	int			start_line_end;

	spx_log_("controller - req skip body chunked.");
	try {
		while (true) {
			if (_chnkd_size) {
				if ((chunked_body_can_parse_chnkd_skip_(cl) == false) || _chnkd_size) {
					return false;
				}
				if (_last_chnkd) {
					return true;
				}
			}
			len.clear();
			if (cl._buf.get_crlf_line_(len) == true) {
				spx_log_("size str len", len);
				if (spx_chunked_syntax_start_line(len, _chnkd_size, cl._req._header) == spx_ok) {
					if (_chnkd_size == 0) {
						_last_chnkd = true;
					}
					cl._req._body_size += _chnkd_size;
					_chnkd_size += 2;
					if (cl._req._body_size > cl._req._body_limit) {
						throw(std::exception());
					}
					continue;
				} else {
					// chunked error
					spx_log_("chunked error. len", len);
					throw(std::exception());
				}
			} else {
				// chunked start line not exist.
				return false;
			}
		}
	} catch (...) {
		if (cl._req._body_size > cl._req._body_limit) {
			// // send over limit.
			// close(cl._req._body_fd);
			// remove(cl._req._upld_fn.c_str());
			// add_change_list(*cl.change_list, cl._req._body_fd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
			// cl.error_response_keep_alive_(HTTP_STATUS_PAYLOAD_TOO_LARGE);
			// cl._state = REQ_SKIP_BODY_CHUNKED;
		} else {
			cl._res.make_error_response_(cl, HTTP_STATUS_BAD_REQUEST);
			if (cl._req._body_fd != -1) {
				add_change_list(*cl.change_list, cl._req._body_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, &cl);
			}
			cl._state = E_BAD_REQ;
		}
		return false;
	}
}

bool
CgiField::cgi_header_parser_() {
	std::string line;
	size_t		idx;

	while (true) {
		// idx = _from_cgi.find_pos_(LF);
		line.clear();
		if (_from_cgi.get_lf_line_(line, 200) <= 0) {
			return false;
		}
		spx_log_("cgi header!!!", line);
		if (line.size() == 0) {
			// request header parsed.
			spx_log_("cgi parsed!!!", line);
			break;
		}
		idx = line.find(':');
		if (idx != std::string::npos) {
			for (std::string::iterator it = line.begin();
				 it != line.begin() + idx; ++it) {
				if (isalpha(*it)) {
					*it = tolower(*it);
				}
			}
			size_t tmp = idx + 1;
			while (tmp < line.size() && syntax_(ows_, line[tmp])) {
				++tmp;
			}
			_cgi_header[line.substr(0, idx)] = line.substr(tmp, line.size() - tmp);
		}
		continue;
	}
	return true;
}

bool
CgiField::cgi_controller_(Client& cl) {
	switch (_cgi_state) {
	case CGI_HEADER: {
		if (cgi_header_parser_() == false) {
			// read more?
			spx_log_("cgi_header_parser false");
			return false;
		}
		_cgi_state = CGI_HOLD;

		std::map<std::string, std::string>::iterator it;
		it = _cgi_header.find("content-length");
		if (it != _cgi_header.end()) {
			_cgi_size = strtol(it->second.c_str(), NULL, 10);
			// TODO: make_cgi_response_header.
			// make_cgi_response_header();
		} else {
			// no content-length case.
			// res.cgi_state_ = CGI_BODY_CHUNKED;
			_cgi_size = -1;
		}
		cl._res.make_cgi_response_header_(cl);
	}
	case CGI_HOLD:
		break;
		// if (_cgi_state == CGI_BODY_CHUNKED) {
		// }
	}
	return true;
}
