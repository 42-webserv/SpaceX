#include "spx_client.hpp"
#include "spx_cgi_module.hpp"
#include "spx_kqueue_module.hpp"

#include "spx_session_storage.hpp"

Client::Client(event_list_t* change_list)
	: change_list(change_list)
	, _req()
	, _res()
	, _cgi()
	, _chnkd()
	, _client_fd(0)
	, _rdbuf()
	, _state(REQ_LINE_PARSING)
	, _skip_size(0)
	, _port_info()
	, _sockaddr(NULL)
	, _storage() {
}

Client::~Client() { }

void
Client::reset_() {
	_req.clear_();
	_res.clear_();
	_cgi.clear_();
	_chnkd.clear_();
}

bool
Client::request_line_check_(std::string& req_line) {
	// request line checker
	if (spx_http_syntax_start_line(req_line, _req._req_mthd) == spx_ok) {
		std::string::size_type r_pos = req_line.find_last_of(' ');
		std::string::size_type l_pos = req_line.find_first_of(' ');
		_req._uri					 = req_line.substr(l_pos + 1, r_pos - l_pos - 1);
		_req._http_ver				 = "HTTP/1.1";
		return true;
	}
	_state = E_BAD_REQ;
	return false;
}

bool
Client::request_line_parser_() {
	std::string req_line;

	spx_log_("REQ_LINE_PARSER");
	while (true) {
		if (_buf.get_crlf_line_(req_line) == false) {
			// read more case.
			return false;
		}
		if (req_line.size())
			break;
	}
	spx_log_("REQ_LINE_PARSER ok");
	// bad request will return false and disconnect client.
	_state = REQ_HEADER_PARSING;
	return request_line_check_(req_line);
}

bool
Client::header_field_parser_() {
	std::string key_val;
	int			idx;

	while (true) {
		key_val.clear();
		if (_buf.get_crlf_line_(key_val)) {
			spx_log_("key_val", key_val);
			spx_log_("key_val size", key_val.size());
			if (key_val.empty()) {
				break;
			}
			idx = key_val.find(':');
			if (idx != std::string::npos) {
				for (std::string::iterator it = key_val.begin(); it != key_val.begin() + idx; ++it) {
					if (isalpha(*it)) {
						*it = tolower(*it);
					}
				}
				size_t tmp = idx + 1;
				while (tmp < key_val.size() && syntax_(ows_, key_val[tmp])) {
					++tmp;
				}
				_req._header[key_val.substr(0, idx)] = key_val.substr(tmp);
			}
			continue;
		}
		// read more case.
		return false;
	}
	req_header_t*		   field = &_req._header;
	req_header_t::iterator it;

	it = field->find("content-length");
	if (it != field->end()) {
		_req._cnt_len = strtoul((it->second).c_str(), NULL, 10);
		spx_log_("content-length", _req._body_size);
	}
	it = field->find("transfer-encoding");
	if (it != field->end()) {
		for (std::string::iterator tmp = it->second.begin(); tmp != it->second.end(); ++tmp) {
			*tmp = tolower(*tmp);
		}
		if (it->second.find("chunked") != std::string::npos) {
			_req._is_chnkd = true;
			spx_log_("transfer-encoding - chunked set");
		}
	}
	return true;
}

bool
CgiField::cgi_handler_(req_field_t& req, event_list_t& change_list, struct kevent* cur_event) {
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
	fcntl(read_from_cgi[0], F_SETFL, O_NONBLOCK);
	close(read_from_cgi[1]);
	add_change_list(change_list, read_from_cgi[0], EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, cur_event->udata);
	add_change_list(change_list, pid, EVFILT_PROC, EV_ADD, NOTE_EXIT, 0, cur_event->udata);
	return true;
}

bool
Client::host_check_(std::string& host) {
	if (host.empty() || (host.size() && host.find_first_of(" \t") == std::string::npos)) {
		return true;
	}
	return false;
}

void
Client::set_cookie_() {
	cookie_t			   cookie;
	req_header_t::iterator req_cookie = _req._header.find("cookie");
	if (req_cookie != _req._header.end()) {
		spx_log_("COOKIE", "FOUND");
		std::string req_cookie_value = (*req_cookie).second;
		spx_log_("REQ_COOKIE_VALUE", req_cookie_value);
		if (!req_cookie_value.empty()) {
			cookie.parse_cookie_header(req_cookie_value);
			cookie_t::key_val_t::iterator find_cookie = cookie.content.find("sessionID");
			if (find_cookie == cookie.content.end() || ((*find_cookie).second).empty() || !_storage->is_key_exsits((*find_cookie).second)) {
				// spx_log_("COOKIE ERROR ", "Invalid_COOKIE");
				spx_log_("MAKING NEW SESSION");
				std::string hash_value = _storage->make_hash(_client_fd);
				_storage->add_new_session(hash_value);
				_req.session_id = SESSIONID + hash_value;
			} else {
				session_t& session = _storage->find_value_by_key((*find_cookie).second);
				session.count_++;
				_req.session_id = SESSIONID + (*find_cookie).second;
				// spx_log_("SESSIONCOUNT", session.count_);
			}
		}
	} else // if first connection or (no session id on cookie)
	{
		spx_log_("MAKING NEW SESSION NOT FOUND SESSION");
		std::string hash_value = _storage->make_hash(_client_fd);
		_storage->add_new_session(hash_value);
		_req.session_id = SESSIONID + hash_value;
	}
	// COOKIE & SESSION END
}

void
Client::error_response_keep_alive_(http_status error_code) {
	_res.make_error_response_(*this, error_code);
	if (_res._body_fd != -1) {
		add_change_list(*change_list, _res._body_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, this);
	}
	if (_req._req_mthd & (REQ_GET | REQ_HEAD | REQ_DELETE)) {
		if (_req._is_chnkd) {
			_req._body_size = -1;
			_state			= REQ_SKIP_BODY_CHUNKED;
		} else {
			if (_req._cnt_len == -1 || _req._cnt_len == 0) {
				_state = REQ_HOLD;
			} else {
				_state = REQ_SKIP_BODY;
			}
		}
	} else {
		if (_req._is_chnkd || _req._cnt_len == -1) {
			_req._body_size = -1;
			_state			= REQ_SKIP_BODY_CHUNKED;
		} else {
			if (_req._cnt_len == 0) {
				_state = REQ_HOLD;
			} else {
				_state = REQ_SKIP_BODY;
			}
		}
	}
	// _res._write_ready = WRITE_READY;
}

void
Client::do_cgi_(struct kevent* cur_event) {
	if (_cgi.cgi_handler_(_req, *change_list, cur_event) == false) {
		error_response_keep_alive_(HTTP_STATUS_INTERNAL_SERVER_ERROR);
	} else {
		spx_log_("cgi true. forked, open pipes. flag", _res._write_ready);
		if (_req._is_chnkd) {
			if (_req._req_mthd & (REQ_GET | REQ_HEAD | REQ_DELETE)) {
				_state = REQ_SKIP_BODY_CHUNKED;
			} else {
				_state = REQ_BODY_CHUNKED;
			}
		} else {
			if (_req._cnt_len != 0) {
				if (_req._req_mthd & (REQ_GET | REQ_HEAD | REQ_DELETE)) {
					_state = REQ_SKIP_BODY;
				} else {
					_state = REQ_HOLD;
				}
			} else {
				_state = REQ_HOLD;
			}
		}
	}
}

bool
Client::res_for_get_head_req_() {
	spx_log_("REQ_GET OR HEAD");
	_res.make_response_header_(*this);
	// spx_log_("RES_OK");
	if (_res._body_fd == -1) {
		spx_log_("No file descriptor");
	} else {
		spx_log_("READ event added");
		add_change_list(*change_list, _res._body_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, this);
	}
	// _res._write_ready = true;
	if (_req._is_chnkd) {
		_state = REQ_SKIP_BODY_CHUNKED;
	} else if (_req._cnt_len == 0 || _req._cnt_len == -1) {
		_state = REQ_HOLD;
	} else {
		_skip_size = _req._cnt_len;
		_state	   = REQ_SKIP_BODY;
		// _state = REQ_SKIP_BODY_CHUNKED;
	}
	return false;
}

bool
Client::res_for_post_put_req_() {
	_req._upld_fn = _req._uri_resolv.script_filename_;
	if (_req._req_mthd & REQ_POST) {
		_req._body_fd = open(_req._upld_fn.c_str(), O_WRONLY | O_CREAT | O_NONBLOCK | O_APPEND, 0644);
	} else {
		_req._body_fd = open(_req._upld_fn.c_str(), O_WRONLY | O_CREAT | O_NONBLOCK | O_TRUNC, 0644);
	}
	// error case
	if (_req._body_fd < 0) {
		// 405 not allowed error with keep-alive connection.
		error_response_keep_alive_(HTTP_STATUS_METHOD_NOT_ALLOWED);
		return false;
		// _req.flag_ |= WRITE_READY;
	}

	// spx_log_("control - REQ_POST fd: ", _req._body_fd);
	// _req.flag_ |= REQ_FILE_OPEN;
	if (_req._is_chnkd || _req._cnt_len == -1) {
		_req._body_size = 0;
		_req._cnt_len	= -1;
		_state			= REQ_BODY_CHUNKED;
		add_change_list(*change_list, _req._body_fd, EVFILT_WRITE, EV_ADD | EV_DISABLE, 0, 0, this);
	} else {
		add_change_list(*change_list, _req._body_fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, this);
		_state = REQ_BODY;
	}
	return false;
}

bool
ChunkedField::chunked_body_can_parse_chnkd_(Client& cl, size_t size) {
	if (cl._buf.buf_size_() >= size) {
		cl._buf.move_(_chnkd_body, size);
		cl._req._body_size += size;
		if (cl._req._body_size > cl._req._body_limit) {
			throw(std::exception());
		}
		if (size != 0) {
			if (cl._buf.find_pos_(CR) != 0 || cl._buf.find_pos_(LF) != 1) {
				// chunked error
				throw(std::exception());
			}
			cl._buf.delete_size_(2);
			return true;
		} else {
			// chunked last
			spx_log_("chunked last piece");
			if (cl._buf.find_pos_(CR) == 0) {
				spx_log_("chunked last no extension");
				// no extention
				cl._req._cnt_len = cl._req._body_size;
				cl._state		 = REQ_HOLD;
				return true;
			} else {
				spx_log_("chunked extension");
				throw(std::exception());
				// yoma's code..? end check..??
			}
		}
	} else {
		// try next.
		spx_log_("chunked try next");
		return false;
	}
}

bool
ChunkedField::chunked_body_can_parse_chnkd_skip_(Client& cl, size_t size) {
	if (cl._buf.buf_size_() >= size) {
		cl._buf.delete_size_(size);
		cl._req._body_size += size;
		if (cl._req._body_size > cl._req._body_limit) {
			throw(std::exception());
		}
		if (size != 0) {
			if (cl._buf.find_pos_(CR) != 0 || cl._buf.find_pos_(LF) != 1) {
				// chunked error
				throw(std::exception());
			}
			cl._buf.delete_size_(2);
			return true;
		} else {
			// chunked last
			spx_log_("chunked last piece");
			if (cl._buf.find_pos_(CR) == 0) {
				spx_log_("chunked last no extension");
				// no extention
				cl._req._cnt_len = cl._req._body_size;
				cl._state		 = REQ_HOLD;
				return true;
			} else {
				spx_log_("chunked extension");
				throw(std::exception());
				// yoma's code..? end check..??
			}
		}
	} else {
		// try next.
		spx_log_("chunked try next");
		return false;
	}
}

bool
ChunkedField::chunked_body_(Client& cl) {
	// rdsaved_ -> chunked_body_buffer_
	// buffer_t::iterator crlf_pos = rdsaved_.begin() + rdchecked_;
	std::string len;
	uint32_t	size = 0;
	int			start_line_end;

	if (_first_chnkd) {
		_first_chnkd = false;
		if (cl._req._uri_resolv.is_cgi_) {
			add_change_list(*cl.change_list, cl._cgi._write_to_cgi_fd, EVFILT_WRITE, EV_ENABLE, 0, 0, &cl);
		} else {
			add_change_list(*cl.change_list, cl._req._body_fd, EVFILT_WRITE, EV_ENABLE, 0, 0, &cl);
		}
	}
	spx_log_("controller - req body chunked.");
	try {
		while (true) {
			len.clear();
			if (cl._buf.get_crlf_line_(len) == true) {
				spx_log_("size str len", len);
				if (spx_chunked_syntax_start_line(len, size, cl._req._header) != spx_error) {
					if (chunked_body_can_parse_chnkd_(cl, size) == false) {
						return false;
					} else if (cl._state == REQ_HOLD) {
						// parsed
						spx_log_("parsed");
						return false;
					}
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
		if (cl._req._body_read > cl._req._body_limit) {
			// send over limit.
			close(cl._req._body_fd);
			remove(cl._req._upld_fn.c_str());
			add_change_list(*cl.change_list, cl._req._body_fd, EVFILT_WRITE, EV_DISABLE | EV_DELETE, 0, 0, NULL);
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
	uint32_t	size = 0;
	int			start_line_end;

	spx_log_("controller - req skip body chunked.");
	// spx_log_("req skip body chunked. req body size", cl._req._body_size);
	// spx_log_("req skip body chunked. req body limit", cl._req._body_limit);
	// spx_log_("req skip body chunked. is chunked", cl._req._is_chnkd);
	try {
		while (true) {
			len.clear();
			if (cl._buf.get_crlf_line_(len) == true) {
				if (spx_chunked_syntax_start_line(len, size, cl._req._header) == spx_ok) {
					if (chunked_body_can_parse_chnkd_skip_(cl, size) == false) {
						return false;
					} else if (cl._state == REQ_HOLD) {
						// parsed
						return false;
					}
				} else {
					// chunked error
					spx_log_("chunked error");
					throw(std::exception());
				}
			} else {
				// chunked start line not exist.
				return false;
			}
		}
	} catch (...) {
		if (cl._req._body_read > cl._req._body_limit) {
			// send over limit.
			close(cl._req._body_fd);
			remove(cl._req._upld_fn.c_str());
			add_change_list(*cl.change_list, cl._req._body_fd, EVFILT_WRITE, EV_DISABLE | EV_DELETE, 0, 0, NULL);
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
Client::req_res_controller_(struct kevent* cur_event) {
	switch (_state) {
	case REQ_CLEAR:
		reset_();
		_state = REQ_LINE_PARSING;
	case REQ_LINE_PARSING:
		if (request_line_parser_() == false) {
			spx_log_("controller-req_line false. read more.", _state);
			return false;
		}
		spx_log_("controller-req_line ok");
	case REQ_HEADER_PARSING: {
		if (header_field_parser_() == false) {
			spx_log_("controller-header false. read more. state", _state);
			return false;
		}
		spx_log_("controller-header ok.");

		std::string& host = _req._header["host"];

		_req._serv_info = &_port_info->search_server_config_(host);
		if (host_check_(host) == false) {
			_res.make_error_response_(*this, HTTP_STATUS_BAD_REQUEST);
			_state = E_BAD_REQ;
			return false;
		}
		_req._uri_loc = _req._serv_info->get_uri_location_t_(_req._uri, _req._uri_resolv);

		if (_req._uri_loc) {
			_req._body_limit = _req._uri_loc->client_max_body_size;
		}

		if (_req._uri_loc == NULL || (_req._uri_loc->accepted_methods_flag & _req._req_mthd) == false) {
			spx_log_("uri_loc == NULL or not allowed");
			if (_req._uri_loc == NULL) {
				error_response_keep_alive_(HTTP_STATUS_NOT_FOUND);
			} else {
				error_response_keep_alive_(HTTP_STATUS_METHOD_NOT_ALLOWED);
			}
			return false;
		} else if (_req._uri_resolv.is_cgi_) {
			// cgi case:
			do_cgi_(cur_event);
			return false;
		}

		// set cookie
		set_cookie_();

		// spx_log_("req_uri set ok");
		switch (_req._req_mthd) {
		case REQ_GET:
		case REQ_HEAD:
			return res_for_get_head_req_();
		case REQ_POST:
		case REQ_PUT:
			return res_for_post_put_req_();
		case REQ_DELETE:
			if (_req._header.empty()) {
			}
			//  HEADER?
			break;
		}
		break;
	}

	case REQ_BODY: {
		int n_move = _buf.move_(_req._body_buf, _req._cnt_len - _req._body_read);

		_req._body_read += n_move;
		if (_req._body_read == _req._cnt_len) {
			_state = REQ_HOLD;
		}
		break;
	}

	case REQ_BODY_CHUNKED:
		return _chnkd.chunked_body_(*this);

	case REQ_SKIP_BODY:
		if (_skip_size) {
			if (_skip_size >= _buf.buf_size_()) {
				_skip_size -= _buf.buf_size_();
				_buf.clear_();
			} else {
				_buf.delete_size_(_skip_size);
				_skip_size = 0;
			}
		}
		break;

	case REQ_SKIP_BODY_CHUNKED:
		return _chnkd.skip_chunked_body_(*this);

		// case REQ_CGI: {
		// 	req_field_t &req = _req;
		// 	res_field_t &res = _req;

		// 	if (req._is_chnkd & TE_CHUNKED) {
		// 		._body_size
		// 	} else {
		// 		if (req.content_length_ == 0) {
		// 		}
		// 	}
		// 	break;
		// }
	}
	// if (rdsaved_.size() != rdchecked_) {
	// 	flag_ &= ~(RDBUF_CHECKED);
	// }
	// _state = REQ_LINE_PARSING;
	return true;
}

// time out case?
void
Client::disconnect_client_() {
	// client status, tmp file...? check.
	add_change_list(*change_list, _client_fd, EVFILT_READ, EV_DISABLE | EV_DELETE, 0, 0, NULL);
	add_change_list(*change_list, _client_fd, EVFILT_WRITE, EV_DISABLE | EV_DELETE, 0, 0, NULL);
	close(_client_fd);
}

// read only request header & body message
void
Client::read_to_client_buffer_(struct kevent* cur_event) {
	int n_read = _rdbuf->read_(cur_event->ident, _buf);
	if (n_read < 0) {
		// TODO: error handle
		return;
	}
	// #ifdef DEBUG
	// 	write(STDOUT_FILENO, &rdbuf_, std::min(200, n_read));
	// #endif
	_buf.write_debug_();
	spx_log_("\nread_to_client", n_read);
	spx_log_("read_to_client state", _state);
	if (_state != REQ_HOLD) {
		req_res_controller_(cur_event);
		spx_log_("req_res_controller check finished. buf stat", _state);
	};
	// spx_log_("enable write", _res.flag_ & WRITE_READY);
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
		if (_cgi_state == CGI_BODY_CHUNKED) {
		}
	}
	return true;
}

void
Client::read_to_cgi_buffer_(struct kevent* cur_event) {
	int n_read = _rdbuf->read_(cur_event->ident, _cgi._from_cgi);
	if (n_read < 0) {
		error_response_keep_alive_(HTTP_STATUS_INTERNAL_SERVER_ERROR);
		close(cur_event->ident);
		close(_cgi._write_to_cgi_fd);
		add_change_list(*change_list, cur_event->ident, EVFILT_READ, EV_DISABLE | EV_DELETE, 0, 0, NULL);
		add_change_list(*change_list, _cgi._write_to_cgi_fd, EVFILT_WRITE, EV_DISABLE | EV_DELETE, 0, 0, NULL);
		return;
	}
	spx_log_("read to cgi buffer. n_read", n_read);
	_cgi.cgi_controller_(*this);
}

void
Client::read_to_res_buffer_(struct kevent* cur_event) {
	int n_read = _rdbuf->read_(cur_event->ident, _res._res_buf);
	_res._res_buf.write_debug_();
	if (n_read < 0) {
		error_response_keep_alive_(HTTP_STATUS_INTERNAL_SERVER_ERROR);
		close(cur_event->ident);
		add_change_list(*change_list, cur_event->ident, EVFILT_READ, EV_DISABLE | EV_DELETE, 0, 0, NULL);
		return;
	}

	_res._body_read += n_read;
	// spx_log_("read_to_res_buffer. n_read", n_read);
	// spx_log_("read_to_res_buffer. res._body_read", _res._body_read);
	// spx_log_("read_to_res_buffer. res._body_size", _res._body_size);
	if (_res._body_read == _res._body_size) {
		// all content read, close fd.
		spx_log_("read_to_res_buffer finished");
		close(cur_event->ident);
		add_change_list(*change_list, cur_event->ident, EVFILT_READ, EV_DISABLE | EV_DELETE, 0, 0, NULL);
		// add_change_list(*change_list, _client_fd, EVFILT_WRITE, EV_ENABLE, 0, 0, this);
	}
}

void
ResField::make_error_response_(Client& cl, http_status error_code) {

	_status		 = http_status_str(error_code);
	_status_code = error_code;

	// headers_.push_back(header("Server", "SpaceX/12.26"));

	if (error_code == HTTP_STATUS_BAD_REQUEST) {
		_headers.push_back(header(CONNECTION, CONNECTION_CLOSE));
	} else {
		_headers.push_back(header(CONNECTION, KEEP_ALIVE));
	}

	// page_path null case added..
	std::string page_path;
	int			error_req_fd;

	if (cl._req._serv_info) {
		page_path = cl._req._serv_info->get_error_page_path_(error_code);
		spx_log_("page_path = ", page_path);
		error_req_fd = open(page_path.c_str(), O_RDONLY);
		spx_log_("error_req_fd : ", error_req_fd);
	} else {
		spx_log_("serv info is NULL");
		error_req_fd = -1;
	}
	if (error_req_fd < 0) {
		std::stringstream ss;
		const std::string error_page = generator_error_page_(error_code);

		_body_size = error_page.length();
		ss << _body_size;
		_headers.push_back(header(CONTENT_LENGTH, ss.str()));
		_headers.push_back(header(CONTENT_TYPE, MIME_TYPE_HTML));
		std::string tmp = make_to_string_();
		write_to_response_buffer_(tmp);
		if ((cl._req._req_mthd & REQ_HEAD) == false) {
			// write_to_response_buffer_(error_page);
			cl._res._res_buf.add_str(error_page);
		}
		add_change_list(*cl.change_list, cl._client_fd, EVFILT_WRITE, EV_ENABLE, 0, 0, &cl);
		return;
	}
	_body_fd = error_req_fd;

	setContentType_(page_path);
	setContentLength_(error_req_fd);

	spx_log_("ERROR_RESPONSE!!");
	write_to_response_buffer_(make_to_string_());
	add_change_list(*cl.change_list, cl._client_fd, EVFILT_WRITE, EV_ENABLE, 0, 0, &cl);
}

// this is main logic to make response
void
ResField::make_response_header_(Client& cl) {

	const std::string& uri	  = cl._req._uri_resolv.script_filename_;
	int				   req_fd = -1;
	std::string		   content;

	// Set Date Header
	setDate_();
	if (!cl._req.session_id.empty()) {
		_headers.push_back(header("Set-Cookie", cl._req.session_id));
	}

	// Redirect
	if (cl._req._uri_loc != NULL && !(cl._req._uri_loc->redirect.empty())) {
		make_redirect_response_(cl);
		return;
	}

	switch (cl._req._req_mthd) {
	case REQ_GET:
	case REQ_HEAD:
		if (uri[uri.size() - 1] != '/') {
			spx_log_("uri.cstr()", uri.c_str());
			req_fd = file_open_(uri.c_str());
		} else {
			spx_log_("folder skip");
		}
		spx_log_("uri_locations", cl._req._uri_loc);
		spx_log_("req_fd", req_fd);
		if (req_fd == 0) {
			spx_log_("folder skip");
			make_error_response_(cl, HTTP_STATUS_FORBIDDEN);
			return;
		} else if (req_fd == -1
				   && (cl._req._uri_loc == NULL || cl._req._uri_loc->autoindex_flag == Kautoindex_off)) {
			make_error_response_(cl, HTTP_STATUS_NOT_FOUND);
			return;
		} else if (req_fd == -1
				   && cl._req._uri_loc->autoindex_flag == Kautoindex_on) {
			spx_log_("uri=======", cl._req._uri_resolv.script_filename_);
			content = generate_autoindex_page(req_fd, cl._req._uri_resolv);
			std::stringstream ss;
			ss << content.size();
			_headers.push_back(header(CONTENT_LENGTH, ss.str()));
			if (content.empty()) {
				// autoindex fail case
				make_error_response_(cl, HTTP_STATUS_FORBIDDEN);
				return;
			}
		}
		if (req_fd != -1) {
			spx_log_("res_header");
			setContentType_(uri);
			setContentLength_(req_fd);
			if (cl._req._req_mthd == REQ_GET) {
				_body_fd = req_fd;
			} else {
				_body_fd = -1;
				close(req_fd);
			}
		} else {
			// autoindex case?
			_headers.push_back(header(CONTENT_TYPE, MIME_TYPE_HTML));
		}
		break;
	case REQ_POST:
	case REQ_PUT:
		_headers.push_back(header(CONTENT_LENGTH, "0"));
		break;
	}
	_headers.push_back(header(CONNECTION, KEEP_ALIVE));
	write_to_response_buffer_(make_to_string_());
	if (!content.empty()) {
		// write_to_response_buffer_(content);
		_res_buf.add_str(content);
	}
	if (cl._req._req_mthd & REQ_HEAD) {
		_body_size = 0;
	}
	add_change_list(*cl.change_list, cl._client_fd, EVFILT_WRITE, EV_ENABLE, 0, 0, &cl);
}

void
ResField::make_cgi_response_header_(Client& cl) {

	// Set Date Header
	setDate_();
	spx_log_("make_cgi_res_header");
	std::map<std::string, std::string>::iterator it;

	_headers.clear();
	it = cl._cgi._cgi_header.find("status");
	if (it != cl._cgi._cgi_header.end()) {
		_status_code = strtol(it->second.c_str(), NULL, 10);
	} else {
		_status_code = 200;
	}
	// headers_.push_back(header("Set-Cookie", "SESSIONID=123456;"));
	// settting response_header size  + content-length size to res_field
	_headers.push_back(header(CONNECTION, KEEP_ALIVE));

	it = cl._cgi._cgi_header.find("content-length");
	if (it != cl._cgi._cgi_header.end()) {
		_headers.push_back(header(CONTENT_LENGTH, it->second));
	} else {
		std::stringstream ss;
		ss << cl._cgi._from_cgi.buf_size_();
		_headers.push_back(header(CONTENT_LENGTH, ss.str().c_str()));
		// headers_.push_back(header(CONTENT_LENGTH, "0"));
	}
	write_to_response_buffer_(make_to_string_());
	add_change_list(*cl.change_list, cl._client_fd, EVFILT_WRITE, EV_ENABLE, 0, 0, &cl);
}

void
ResField::make_redirect_response_(Client& cl) {
	spx_log_("uri_loc->redirect", cl._req._uri_loc->redirect);
	_status_code = HTTP_STATUS_MOVED_PERMANENTLY;
	_status		 = http_status_str(HTTP_STATUS_MOVED_PERMANENTLY);
	_headers.push_back(header("Location", cl._req._uri_loc->redirect));
	write_to_response_buffer_(make_to_string_());
	add_change_list(*cl.change_list, cl._client_fd, EVFILT_WRITE, EV_ENABLE, 0, 0, &cl);
}

bool
Client::write_for_upload_(struct kevent* cur_event) {
	int	   n_write;
	size_t buf_len;

	if (_req._is_chnkd) {
		n_write = _chnkd._chnkd_body.write_(cur_event->ident);
		_req._body_read += n_write;
		if (_req._body_read == _req._cnt_len) {
			// upload finished.
			close(cur_event->ident);
			add_change_list(*change_list, cur_event->ident, EVFILT_WRITE, EV_DISABLE | EV_DELETE, 0, 0, NULL);
			_state = REQ_LINE_PARSING;
		}
		return true;
	} else {
		if (_req._body_limit < _req._cnt_len) {
			// over sized.
			close(cur_event->ident);
			remove(_req._upld_fn.c_str());
			add_change_list(*change_list, cur_event->ident, EVFILT_WRITE, EV_DISABLE | EV_DELETE, 0, 0, NULL);
			return false;
		}
		n_write = _req._body_buf.write_(cur_event->ident);
		spx_log_("body_fd: ", cur_event->ident);
		spx_log_("write len: ", n_write);
		_req._body_read += n_write;
	}
	if (_req._body_read == _req._cnt_len) {
		close(cur_event->ident);
		add_change_list(*change_list, cur_event->ident, EVFILT_WRITE, EV_DISABLE | EV_DELETE, 0, 0, NULL);
		_state = REQ_LINE_PARSING;
	}
	return true;
}

bool
Client::write_to_cgi_(struct kevent* cur_event) {
	size_t buf_len;
	int	   n_write;

	if (_req._is_chnkd) {
		n_write = _chnkd._chnkd_body.write_(cur_event->ident);
	} else {
		n_write = _cgi._to_cgi.write_(cur_event->ident);
	}
	_req._body_read += n_write;
	if (_req._body_read == _req._cnt_len) {
		close(cur_event->ident);
		add_change_list(*change_list, cur_event->ident, EVFILT_WRITE, EV_DISABLE | EV_DELETE, 0, 0, NULL);
		_state = REQ_LINE_PARSING; //?
	}
	return true;
}

bool
Client::write_response_() {
	// no chunked case.
	int n_write;
	if (_res._header_sent == false) {
		n_write = write(STDOUT_FILENO, _res._res_header.c_str(), _res._res_header.size());
		n_write = write(_client_fd, _res._res_header.c_str(), _res._res_header.size());
		if (n_write < 0) {
			spx_log_("write error");
			// disconnect_client_();
			return false;
		}
		if (n_write != _res._res_header.size()) {
			_res._res_header.erase(0, n_write);
			return true;
		} else {
			_res._res_header.clear();
			_res._header_sent = true;
			spx_log_("write response body size", _res._body_size);
			if (_res._body_size == 0) {
				add_change_list(*change_list, _client_fd, EVFILT_WRITE, EV_DISABLE, 0, 0, this);
				if (_state != REQ_SKIP_BODY_CHUNKED) {
					_state = REQ_CLEAR;
				}
			}
			return true;
		}
	} else {
		// file write
		if (_req._uri_resolv.is_cgi_) {
			n_write = _cgi._from_cgi.write_debug_();
			n_write = _cgi._from_cgi.write_(_client_fd);
		} else {
			n_write = _res._res_buf.write_debug_();
			n_write = _res._res_buf.write_(_client_fd);
		}
		if (n_write < 0) {
			spx_log_("write error");
			// disconnect_client_();
			return false;
		}
		_res._body_write += n_write;
	}
	spx_log_("body size", _res._body_size);
	spx_log_("body read", _res._body_write);
	if (_res._body_write == _res._body_size) {
		add_change_list(*change_list, _client_fd, EVFILT_WRITE, EV_DISABLE, 0, 0, this);
		if (_state != REQ_SKIP_BODY_CHUNKED) {
			_state = REQ_CLEAR;
		}
	}
	return true;
}

void
ResField::write_to_response_buffer_(const std::string& content) {
	_res_header.insert(_res_header.end(), content.begin(), content.end());
	// _buf_size += content.size();
	// _write_ready = WRITE_READY;
}

std::string
ResField::make_to_string_() const {
	std::stringstream stream;
	stream << "HTTP/" << _version_major << "." << _version_minor
		   << " " << _status_code << " " << _status << CRLF;
	for (std::vector<header>::const_iterator it = _headers.begin(); it != _headers.end(); ++it) {
		stream << it->first << ": " << it->second << CRLF;
	}
	stream << CRLF;
	return stream.str();
}

int
ResField::file_open_(const char* dir) const {
	struct stat buf;

	stat(dir, &buf);
	if (S_ISDIR(buf.st_mode))
		return -1;
	int fd = open(dir, O_RDONLY | O_NONBLOCK, 0644);
	if (fd < 0 && errno == EACCES)
		return 0;
	return fd;
}

off_t
ResField::setContentLength_(int fd) {
	if (fd < 0)
		return 0;
	off_t length = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);

	std::stringstream ss;
	ss << length;
	spx_log_("setContentLength. length", length);
	_body_size = length;
	// _body_size += length;

	_headers.push_back(header(CONTENT_LENGTH, ss.str()));
	return length;
}

void
ResField::setContentType_(std::string uri) {

	std::string::size_type uri_ext_size = uri.find_last_of('.');
	std::string			   ext;

	if (uri_ext_size != std::string::npos) {
		ext = uri.substr(uri_ext_size + 1);
	}
	if (ext == "html" || ext == "htm")
		_headers.push_back(header(CONTENT_TYPE, MIME_TYPE_HTML));
	else if (ext == "png")
		_headers.push_back(header(CONTENT_TYPE, MIME_TYPE_PNG));
	else if (ext == "jpg")
		_headers.push_back(header(CONTENT_TYPE, MIME_TYPE_JPG));
	else if (ext == "jpeg")
		_headers.push_back(header(CONTENT_TYPE, MIME_TYPE_JPEG));
	else if (ext == "txt")
		_headers.push_back(header(CONTENT_TYPE, MIME_TYPE_TEXT));
	else
		_headers.push_back(header(CONTENT_TYPE, MIME_TYPE_DEFUALT));
}

void
ResField::setDate_(void) {
	std::time_t now			 = std::time(nullptr);
	std::tm*	current_time = std::gmtime(&now);

	char date_buf[32];
	std::strftime(date_buf, sizeof(date_buf), "%a, %d %b %Y %T GMT", current_time);
	_headers.push_back(header("Date", date_buf));
}
