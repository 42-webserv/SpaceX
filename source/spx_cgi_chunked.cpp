#include "spx_cgi_chunked.hpp"
#include "spx_cgi_module.hpp"
#include "spx_core_util_box.hpp"
#include "spx_kqueue_module.hpp"
#include "spx_session_storage.hpp"

ChunkedField::ChunkedField()
	: _chnkd_body()
	, _chnkd_size(0)
	, _first_chnkd(true)
	, _last_chnkd(false) {
}
ChunkedField::~ChunkedField() {
}

void
ChunkedField::clear_() {
	_chnkd_body.clear_();
	_chnkd_size	 = 0;
	_first_chnkd = true;
	_last_chnkd	 = false;
}

bool
ChunkedField::chunked_body_can_parse_chnkd_(Client& cl) {
	if (_chnkd_size > 2) {
		_chnkd_size -= cl._buf.move_(_chnkd_body, _chnkd_size - 2);
		if (_chnkd_size != 2) {
			return true;
		}
	}
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
		if (cl._buf.find_pos_(CR) != 0 || cl._buf.find_pos_(LF) != 1) {
			// chunked error
			throw(std::exception());
		}
		_chnkd_size -= cl._buf.delete_size_(2);
		if (_last_chnkd) {
			// chunked last
			cl._req._cnt_len = cl._req._body_size;
			cl._state		 = REQ_CLEAR;
			return true;
		}
		return true;
	}
	return false;
}

bool
ChunkedField::chunked_body_(Client& cl) {
	std::string len;

	if (_first_chnkd) {
		_first_chnkd = false;
		if (cl._req._uri_resolv.is_cgi_) {
			add_change_list(*cl.change_list, cl._cgi._write_to_cgi_fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, &cl);
		} else {
			add_change_list(*cl.change_list, cl._req._body_fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, &cl);
		}
	}
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
				if (spx_chunked_syntax_start_line(len, _chnkd_size, cl._req._header) == spx_ok) {
					add_change_list(*cl.change_list, cl._client_fd, EVFILT_READ, EV_ADD, 0, _chnkd_size - cl._buf.buf_size_(), &cl);
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
					throw(std::exception());
				}
			} else {
				return false;
			}
		}
	} catch (...) {
		if (cl._req._body_size > cl._req._body_limit) {
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
				if (spx_chunked_syntax_start_line(len, _chnkd_size, cl._req._header) == spx_ok) {
					if (_chnkd_size == 0) {
						_last_chnkd = true;
						if (cl._state == REQ_HOLD) {
							cl._state = REQ_CLEAR;
						}
					}
					cl._req._body_size += _chnkd_size;
					_chnkd_size += 2;
					continue;
				} else {
					// chunked error
					throw(std::exception());
				}
			} else {
				// chunked start line not exist.
				return false;
			}
		}
	} catch (...) {
		// send over limit.
		cl._res.make_error_response_(cl, HTTP_STATUS_BAD_REQUEST);
		if (cl._req._body_fd != -1) {
			add_change_list(*cl.change_list, cl._req._body_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, &cl);
		}
		cl._state = E_BAD_REQ;
		return false;
	}
}

CgiField::CgiField()
	: _cgi_header()
	, _from_cgi()
	, _to_cgi()
	, _cgi_size(0)
	, _cgi_read(0)
	, _write_to_cgi_fd(0)
	, _read_from_cgi_fd(0)
	, _pid(0)
	, _cgi_state(CGI_HEADER)
	, _is_chnkd(false)
	, _cgi_done(false) {
}

CgiField::~CgiField() {
}

void
CgiField::clear_() {
	_cgi_header.clear();
	_from_cgi.clear_();
	_to_cgi.clear_();
	_cgi_size		  = 0;
	_cgi_read		  = 0;
	_write_to_cgi_fd  = 0;
	_read_from_cgi_fd = 0;
	_pid			  = 0;
	_cgi_state		  = CGI_HEADER;
	_is_chnkd		  = false;
	_cgi_done		  = false;
}

bool
CgiField::cgi_handler_(ReqField& req, event_list_t& change_list, struct kevent* cur_event) {
	int write_to_cgi[2];
	int read_from_cgi[2];

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
	_pid = fork();
	if (_pid < 0) {
		// fork error
		close(write_to_cgi[0]);
		close(write_to_cgi[1]);
		close(read_from_cgi[0]);
		close(read_from_cgi[1]);
		return false;
	}
	if (_pid == 0) {
		CgiModule	cgi(req._uri_resolv, req._header, req._uri_loc, req._serv_info);
		char const* script[3];
		script[0] = req._uri_resolv.cgi_path_info_.c_str();
		script[1] = req._uri_resolv.script_filename_.c_str();
		script[2] = NULL;
		cgi.made_env_for_cgi_(req._req_mthd);

		dup2(write_to_cgi[0], STDIN_FILENO);
		close(write_to_cgi[0]);
		close(write_to_cgi[1]);
		close(read_from_cgi[0]);
		dup2(read_from_cgi[1], STDOUT_FILENO);
		close(read_from_cgi[1]);

		execve(script[0], const_cast<char* const*>(script),
			   const_cast<char* const*>(&cgi.env_for_cgi_[0]));
		exit(EXIT_FAILURE);
	}
	close(write_to_cgi[0]);
	if (req._req_mthd & (REQ_GET | REQ_HEAD | REQ_DELETE)) {
		close(write_to_cgi[1]);
	} else {
		fcntl(write_to_cgi[1], F_SETFL, O_NONBLOCK);
		_write_to_cgi_fd = write_to_cgi[1];
	}
	close(read_from_cgi[1]);
	_read_from_cgi_fd = read_from_cgi[0];
	fcntl(read_from_cgi[0], F_SETFL, O_NONBLOCK);
	add_change_list(change_list, read_from_cgi[0], EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, cur_event->udata);
	add_change_list(change_list, _pid, EVFILT_PROC, EV_ADD, NOTE_EXIT | NOTE_EXITSTATUS, 0, cur_event->udata);

	return true;
}

bool
CgiField::cgi_header_parser_() {
	std::string line;
	size_t		idx;

	while (true) {
		line.clear();
		if (_from_cgi.get_lf_line_(line, 200) <= 0) {
			return false;
		}
		if (line.size() == 0) {
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
		if (_cgi_done) {
			if (cgi_header_parser_() == false) {
				return false;
			}
			_cgi_state = CGI_HOLD;

			std::map<std::string, std::string>::iterator it;
			it = _cgi_header.find("content-length");
			if (it != _cgi_header.end()) {
				_cgi_size = strtol(it->second.c_str(), NULL, 10);
			} else {
				_cgi_size = -1;
			}
			cl._res.make_cgi_response_header_(cl);
		}
	}
	case CGI_HOLD:
		break;
	}
	return true;
}
