#include "spx_client.hpp"
#include "spx_cgi_module.hpp"
#include "spx_kqueue_module.hpp"

#include "spx_session_storage.hpp"

#include "spx_core_util_box.hpp"

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
	, _sockaddr()
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
	// std::cerr << req_line << std::endl;
	spx_log_("REQ_LINE_PARSER ok", req_line);
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
			// spx_log_("key_val", key_val);
			// spx_log_("key_val size", key_val.size());
			// std::cerr << key_val << std::endl;
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
		std::string req_cookie_value = (*req_cookie).second;
		if (!req_cookie_value.empty()) {
			cookie.parse_cookie_header(req_cookie_value);
			cookie_t::key_val_t::iterator find_cookie = cookie.content.find("sessionID");
			if (find_cookie == cookie.content.end() || ((*find_cookie).second).empty() || !_storage->is_key_exsits((*find_cookie).second)) {
				spx_log_("MAKING NEW SESSION");
				std::string session_key = _storage->generate_session_id(_client_fd);
				_storage->add_new_session(session_key);
				_req.session_id = SESSIONID + session_key + "; " + MAX_AGE + AGE_TIME;
			} else {
				session_t& session = _storage->find_value_by_key((*find_cookie).second);
				session.count_++;
				session.refresh_time();
				_req.session_id = SESSIONID + (*find_cookie).second + "; " + MAX_AGE + AGE_TIME;
				// spx_log_("SESSIONCOUNT", session.count_);
			}
		}
	} else // if first connection or (no session id on cookie)
	{
		spx_log_("NOT FOUND SESSION => MAKING NEW SESSION");
		std::string session_key = _storage->generate_session_id(_client_fd);
		_storage->add_new_session(session_key);
		_req.session_id = SESSIONID + session_key + "; " + MAX_AGE + AGE_TIME;
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
		spx_log_("cgi true. forked, open pipes. flag", _res._write_finished);
		if (_req._is_chnkd) {
			if (_req._req_mthd & (REQ_GET | REQ_HEAD | REQ_DELETE)) {
				_state = REQ_SKIP_BODY_CHUNKED;
			} else {
				_state = REQ_CGI_BODY_CHUNKED;
				// _cgi._cgi_state = CGI_HOLD;
			}
		} else {
			if (_req._cnt_len != 0) {
				spx_log_("cgi cntlen", _req._cnt_len);
				if (_req._req_mthd & (REQ_GET | REQ_HEAD | REQ_DELETE)) {
					_state = REQ_SKIP_BODY;
				} else {
					_state = REQ_CGI_BODY;
					// _cgi._cgi_state = CGI_HOLD;
				}
			} else {
				_state = REQ_HOLD;
			}
		}
		req_res_controller_(cur_event);
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
		if (_res._body_size) {
			add_change_list(*change_list, _res._body_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, this);
		} else {
			close(_res._body_fd);
			_res._body_fd = -1;
		}
	}
	// _res._write_ready = true;
	if (_req._is_chnkd) {
		_state = REQ_SKIP_BODY_CHUNKED;
	} else if (_req._cnt_len == 0 || _req._cnt_len == -1) {
		_state = REQ_HOLD;
		// add_change_list(*change_list, _client_fd, EVFILT_READ, EV_DELETE, 0, 0, this);
	} else {
		_skip_size = _req._cnt_len;
		_state	   = REQ_SKIP_BODY;
		// _state = REQ_SKIP_BODY_CHUNKED;
	}
	return false;
}

bool
Client::state_req_body_() {
	int n_move = _buf.move_(_req._body_buf, _req._cnt_len - _req._body_read);

	_req._body_read += n_move;
	if (_req._body_read == _req._cnt_len) {
		_state = REQ_HOLD;
		_res.make_response_header_(*this);
	}
	return true;
}

bool
Client::res_for_post_put_req_() {
	_req._upld_fn = _req._uri_resolv.script_filename_;
	spx_log_("res_for_post. scriptfilename", _req._uri_resolv.script_filename_);
	if (_req._req_mthd & REQ_POST) {
		_req._body_fd = open(_req._upld_fn.c_str(), O_WRONLY | O_CREAT | O_NONBLOCK | O_APPEND, 0644);
	} else {
		_req._body_fd = open(_req._upld_fn.c_str(), O_WRONLY | O_CREAT | O_NONBLOCK | O_TRUNC, 0644);
	}
	// error case
	if (_req._body_fd < 0) {
		// 405 not allowed error with keep-alive connection.
		error_response_keep_alive_(HTTP_STATUS_NOT_FOUND);
		return false;
	}

	if (_req._is_chnkd || _req._cnt_len == -1) {
		_req._body_size = 0;
		_req._cnt_len	= -1;
		_state			= REQ_BODY_CHUNKED;
		// add_change_list(*change_list, _req._body_fd, EVFILT_WRITE, EV_DELETE, 0, 0, this);
		_chnkd.chunked_body_(*this);
	} else {
		add_change_list(*change_list, _req._body_fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, this);
		_state = REQ_BODY;
		state_req_body_();
	}
	return false;
}

bool
Client::req_res_controller_(struct kevent* cur_event) {
	switch (_state) {
	case REQ_CLEAR:
		reset_();
		_state = REQ_LINE_PARSING;
	case REQ_LINE_PARSING:
#ifdef CONSOLE_LOG
		gettimeofday(&(static_cast<client_t*>(cur_event->udata))->_established_time, NULL);
#endif
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
		_req._uri_loc = _req._serv_info->get_uri_location_t_(_req._uri, _req._uri_resolv, _req._req_mthd);

		if (_req._uri_loc) {
			_req._body_limit = _req._uri_loc->client_max_body_size;
		}

		set_cookie_();

		if (_req._uri_loc == NULL || (_req._uri_loc->accepted_methods_flag & _req._req_mthd) == false) {
			spx_log_("uri_loc == NULL or not allowed error. req_mthd", _req._req_mthd);
			_req._uri_resolv.is_cgi_ = false;
			if (_req._uri_loc == NULL) {
				error_response_keep_alive_(HTTP_STATUS_NOT_FOUND);
				return false;
			} else {
				error_response_keep_alive_(HTTP_STATUS_METHOD_NOT_ALLOWED);
				return false;
			}
			return false;
		} else if (_req._uri_resolv.is_cgi_) {
			// cgi case:
			do_cgi_(cur_event);
			return false;
		}

		// set cookie

		// spx_log_("req_uri set ok");
		switch (_req._req_mthd) {
		case REQ_GET:
		case REQ_HEAD:
			return res_for_get_head_req_();
		case REQ_POST:
		case REQ_PUT:
			return res_for_post_put_req_();
		case REQ_DELETE:
			if (remove(_req._uri_resolv.script_filename_.c_str()) == -1) {
				error_response_keep_alive_(HTTP_STATUS_NOT_FOUND);
			} else {
				_res.make_response_header_(*this);
			}

			// if (_req._header.empty()) {
			// }
			//  HEADER?
			break;
		}
		break;
	}

	case REQ_BODY:
		return state_req_body_();

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

	case REQ_CGI_BODY: {
		int n_move = _buf.move_(_cgi._to_cgi, _req._cnt_len - _req._body_read);

		spx_log_("moved", _cgi._to_cgi.buf_size_());
		_req._body_read += n_move;
		if (_req._body_read == _req._cnt_len) {
			_state = REQ_HOLD;
			add_change_list(*change_list, _cgi._write_to_cgi_fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, this);
		}
		break;
	}

	case REQ_CGI_BODY_CHUNKED:
		if (_chnkd.chunked_body_(*this) == false) {
			if (_chnkd._chnkd_body.buf_size_()) {
				_chnkd._chnkd_body.move_(_cgi._to_cgi, -1);
			}
			return false;
		} else {
			_chnkd._chnkd_body.move_(_cgi._to_cgi, -1);
			_state = REQ_HOLD;
		}
		break;

	case REQ_HOLD:
		break;
		// case E_BAD_REQ:
		// 	disconnect_client_();
	}
	return true;
}

// time out case?
void
Client::disconnect_client_() {
	// client status, tmp file...? check.
	if (_req._body_fd > 0) {
		close(_req._body_fd);
		add_change_list(*change_list, _req._body_fd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
		// remove(_req._uri_resolv.script_filename_.c_str());
	}
	if (_res._body_fd > 0) {
		close(_res._body_fd);
		add_change_list(*change_list, _res._body_fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
	}
	if (_cgi._read_from_cgi_fd > 0) {
		close(_cgi._read_from_cgi_fd);
		add_change_list(*change_list, _cgi._read_from_cgi_fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
	}
	if (_cgi._write_to_cgi_fd > 0) {
		close(_cgi._write_to_cgi_fd);
		add_change_list(*change_list, _cgi._write_to_cgi_fd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
	}
	add_change_list(*change_list, _client_fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
	add_change_list(*change_list, _client_fd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
	close(_client_fd);
}

// read only request header & body message
void
Client::read_to_client_buffer_(struct kevent* cur_event) {
	// spx_log_("cur_event->data", cur_event->data);
	int n_read = _rdbuf->read_(cur_event->ident, _buf);
	// int fd	   = open("aaa", O_CREAT | O_TRUNC | O_WRONLY, 0644);
	// _buf.write_debug_(fd);
	if (n_read < 0) {
		// TODO: error handle
		return;
	}
	spx_log_("read_to_client", n_read);
	spx_log_("read_to_client state", _state);
	// spx_log_("buf partial point", _buf.get_partial_point_());
	if (_state != REQ_HOLD) {
		req_res_controller_(cur_event);
		// spx_log_("req_res_controller check finished. buf stat", _state);
	};
	// spx_log_("enable write", _res.flag_ & WRITE_READY);
}

void
Client::read_to_cgi_buffer_(struct kevent* cur_event) {
	int n_read = _rdbuf->read_(cur_event->ident, _cgi._from_cgi);
	// _cgi._from_cgi.write_debug_();
	if (n_read < 0) {
		error_response_keep_alive_(HTTP_STATUS_INTERNAL_SERVER_ERROR);
		close(cur_event->ident);
		close(_cgi._write_to_cgi_fd);
		add_change_list(*change_list, cur_event->ident, EVFILT_READ, EV_DELETE, 0, 0, NULL);
		add_change_list(*change_list, _cgi._write_to_cgi_fd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
		return;
	}
	spx_log_("read to cgi buffer. n_read", n_read);
	if (_cgi._cgi_state != CGI_HOLD && _req._cnt_len != -1) {
		_cgi.cgi_controller_(*this);
	}
}

void
Client::read_to_res_buffer_(struct kevent* cur_event) {
	int n_read = _rdbuf->read_(cur_event->ident, _res._res_buf);
	// _res._res_buf.write_debug_();
	if (n_read < 0) {
		spx_log_("INTERNAL");
		// std::cerr << "INTERNAL!!" << std::endl;
		// std::cerr << "cur_event->fd : " << cur_event->ident << std::endl;
		// std::cerr << "cur_event->data : " << cur_event->data << std::endl;
		// std::cerr << "cur_event->flter : " << cur_event->filter << std::endl;
		// std::cerr << "cur_event->flags : " << cur_event->flags << std::endl;
		// std::cerr << "cur_event->fflags : " << cur_event->fflags << std::endl;
		error_response_keep_alive_(HTTP_STATUS_INTERNAL_SERVER_ERROR);
		close(cur_event->ident);
		add_change_list(*change_list, cur_event->ident, EVFILT_READ, EV_DELETE, 0, 0, NULL);
		return;
	}

	_res._body_read += n_read;
	spx_log_("read_to_res_buffer. _client_fd", _client_fd);
	spx_log_("read_to_res_buffer. n_read", n_read);
	spx_log_("read_to_res_buffer. res._body_read", _res._body_read);
	spx_log_("read_to_res_buffer. res._body_size", _res._body_size);
	if (_res._body_read == _res._body_size) {
		// all content read, close fd.
		spx_log_("read_to_res_buffer finished");
		close(cur_event->ident);
		add_change_list(*change_list, cur_event->ident, EVFILT_READ, EV_DELETE, 0, 0, NULL);
		add_change_list(*change_list, _client_fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, this);
	}
}

bool
Client::write_for_upload_(struct kevent* cur_event) {
	int	   n_write;
	size_t buf_len;

	if (_req._is_chnkd) {
		n_write = _chnkd._chnkd_body.write_(cur_event->ident);
		_req._body_read += n_write;
		// spx_log_("uploading body size", _req._body_read);
		if (_req._body_read == _req._cnt_len) {
			// upload finished.
			spx_log_("upload_finished");
			close(cur_event->ident);
			add_change_list(*change_list, cur_event->ident, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
			_state = REQ_HOLD;
			add_change_list(*change_list, _client_fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, this);
			_res.make_response_header_(*this);
		}
		return true;
	} else {
		if (_req._body_limit < _req._cnt_len) {
			// over sized.
			close(cur_event->ident);
			remove(_req._upld_fn.c_str());
			add_change_list(*change_list, cur_event->ident, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
			return false;
		}
		n_write = _req._body_buf.write_(cur_event->ident);
		spx_log_("body_fd: ", cur_event->ident);
		spx_log_("write len: ", n_write);
		_req._body_read += n_write;
	}
	if (_req._body_read == _req._cnt_len) {
		close(cur_event->ident);
		add_change_list(*change_list, cur_event->ident, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
		_state = REQ_HOLD;
		add_change_list(*change_list, _client_fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, this);
		_res.make_response_header_(*this);
	}
	return true;
}

bool
Client::write_to_cgi_(struct kevent* cur_event) {
	size_t buf_len;
	int	   n_write;

	// if (_req._is_chnkd) {
	// 	n_write = _chnkd._chnkd_body.write_(cur_event->ident);
	// 	spx_log_("write to chnkd. n_write", n_write);
	// } else {
	// _cgi._to_cgi.write_debug_();
	n_write = _cgi._to_cgi.write_(cur_event->ident);
	// exit(1);
	// spx_log_("write to cgi. n_write", n_write);
	// spx_log_("write to cgi. _req._cnt_len", _req._cnt_len);
	// }
	_cgi._cgi_read += n_write;
	spx_log_("write to cgi. _cgi._cgi_read", _cgi._cgi_read);
	spx_log_("write to cgi. _req._cnt_len", _req._cnt_len);
	if (_cgi._cgi_read == _req._cnt_len) {
		close(cur_event->ident);
		add_change_list(*change_list, cur_event->ident, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
		_cgi._cgi_state = CGI_HEADER;
		_state			= REQ_HOLD; //?
	}
	return true;
}

bool
Client::write_response_() {
	// no chunked case.
	int n_write;
	spx_log_("write_response_ _res._header_sent", _res._header_sent);
	if (_res._header_sent == false) {

#ifdef CONSOLE_LOG
		spx_console_log_(_res._res_header.substr(0, _res._res_header.find("\n")),
						 this->_established_time,
						 _res._body_size,
						 _req._req_mthd,
						 _req._uri);
#endif
#ifdef DEBUG
		n_write = write(STDOUT_FILENO, _res._res_header.c_str(), _res._res_header.size()); // NOTE : check response log
#endif
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
			if (_res._body_size == 0 || (_req._req_mthd & REQ_HEAD)) {
				add_change_list(*change_list, _client_fd, EVFILT_WRITE, EV_DELETE, 0, 0, this);
				if (_state != REQ_SKIP_BODY_CHUNKED) {
					_state = REQ_CLEAR;
				}
				_res._write_finished = true;
			}
			return true;
		}
	} else {
		// file write
		if (_req._uri_resolv.is_cgi_) {
			// n_write = _cgi._from_cgi.write_debug_();
			n_write = _cgi._from_cgi.write_(_client_fd);
		} else {
			// n_write = _res._res_buf.write_debug_();
			n_write = _res._res_buf.write_(_client_fd);
			if (n_write == 0) {
				spx_log_("_client_fd", _client_fd);
				spx_log_("write error");
				exit(1);
			}
		}
		if (n_write < 0) {
			spx_log_("write error");
			// disconnect_client_();
			return false;
		}
		_res._body_write += n_write;
		spx_log_("_req._uri_resolv.is_cgi_", _req._uri_resolv.is_cgi_);
		spx_log_("_cgi._cgi_read", _cgi._cgi_read);
		spx_log_("_res._body_write", _res._body_write);
		spx_log_("_res._body_size", _res._body_size);
		if (_req._uri_resolv.is_cgi_) {
			if ((_cgi._is_chnkd && _res._body_write == _cgi._cgi_read)
				|| (_cgi._is_chnkd == false && _res._body_write == _res._body_size)) {
				add_change_list(*change_list, _client_fd, EVFILT_WRITE, EV_DELETE, 0, 0, this);
				if (_state != REQ_SKIP_BODY_CHUNKED) {
					_state = REQ_CLEAR;
				}
				_res._write_finished = true;
			}
		} else {
			if (_res._body_write == _res._body_size) {

				add_change_list(*change_list, _client_fd, EVFILT_WRITE, EV_DELETE, 0, 0, this);
				if (_state != REQ_SKIP_BODY_CHUNKED) {
					_state = REQ_CLEAR;
				}
				_res._write_finished = true;
			}
		}
	}
	// spx_log_("n_write", n_write);
	// spx_log_("client fd", _client_fd);
	// spx_log_("body size", _res._body_size);
	// spx_log_("body read", _res._body_write);
	return true;
}
