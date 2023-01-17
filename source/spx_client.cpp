#include "spx_client.hpp"
#include "spx_cgi_module.hpp"
#include "spx_core_util_box.hpp"
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
	, _sockaddr()
	, _storage() {
}

Client::~Client() {
	reset_();
	_buf.clear_();
}

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
	_state			= E_BAD_REQ;
	_req._serv_info = &_port_info->my_port_default_server;
	error_response_keep_alive_(HTTP_STATUS_BAD_REQUEST);
	return false;
}

bool
Client::request_line_parser_() {
	std::string req_line;

	while (true) {
		if (_buf.get_crlf_line_(req_line) != true) {
			// read more case.
			return false;
		}
		spx_log_(req_line);
		if (req_line.size())
			break;
	}
	// bad request will return false and disconnect client.
	_state = REQ_HEADER_PARSING;
	return request_line_check_(req_line);
}

bool
Client::header_field_parser_() {
	std::string key_val;
	size_t		idx;

	while (true) {
		key_val.clear();
		if (_buf.get_crlf_line_(key_val)) {
			if (key_val.empty()) {
				break;
			}
			spx_log_(key_val);
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
	}
	it = field->find("transfer-encoding");
	if (it != field->end()) {
		for (std::string::iterator tmp = it->second.begin(); tmp != it->second.end(); ++tmp) {
			*tmp = tolower(*tmp);
		}
		if (it->second.find("chunked") != std::string::npos) {
			_req._is_chnkd = true;
		}
	}

	return true;
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
				std::string session_key = _storage->generate_session_id(_client_fd);
				_storage->add_new_session(session_key);
				_req.session_id = SESSIONID + session_key + "; " + MAX_AGE + AGE_TIME + ";" + " cookie-count=0";
			} else {
				session_t&		  session = _storage->find_value_by_key((*find_cookie).second);
				std::stringstream ss;
				ss << session.count_++;
				session.refresh_time();
				_req.session_id = SESSIONID + (*find_cookie).second + "; " + MAX_AGE + AGE_TIME + ";" + " cookie-count=" + ss.str();
			}
		}
	}
	// if first connection or (no session id on cookie)
	else {
		std::string session_key = _storage->generate_session_id(_client_fd);
		_storage->add_new_session(session_key);
		_req.session_id = SESSIONID + session_key + "; " + MAX_AGE + AGE_TIME + ";" + " cookie-count=0";
	}
}

void
Client::error_response_keep_alive_(http_status error_code) {
	_res.make_error_response_(*this, error_code);
	if (_res._body_fd != -1) {
		add_change_list(*change_list, _res._body_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, this);
	}
	if (_req._req_mthd & (REQ_GET | REQ_HEAD | REQ_DELETE)) {
		if (_req._is_chnkd) {
			_req._body_size = SIZE_T_MAX;
			_state			= REQ_SKIP_BODY_CHUNKED;
		} else {
			if (_req._cnt_len == SIZE_T_MAX || _req._cnt_len == 0) {
				_state = REQ_HOLD;
			} else {
				_state = REQ_SKIP_BODY;
			}
		}
	} else {
		if (_req._is_chnkd || _req._cnt_len == SIZE_T_MAX) {
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
}

void
Client::do_cgi_(struct kevent* cur_event) {
	if (_cgi.cgi_handler_(_req, *change_list, cur_event) == false) {
		error_response_keep_alive_(HTTP_STATUS_INTERNAL_SERVER_ERROR);
	} else {
		if (_req._is_chnkd) {
			if (_req._req_mthd & (REQ_GET | REQ_HEAD | REQ_DELETE)) {
				_state = REQ_SKIP_BODY_CHUNKED;
			} else {
				_state = REQ_CGI_BODY_CHUNKED;
			}
		} else {
			if (_req._cnt_len != 0) {
				if (_req._req_mthd & (REQ_GET | REQ_HEAD | REQ_DELETE)) {
					_state = REQ_SKIP_BODY;
				} else {
					_state = REQ_CGI_BODY;
				}
			} else {
				_state = REQ_HOLD;
			}
		}
		req_res_controller_(cur_event);
	}
}

bool
Client::state_req_body_() {
	int n_move = _buf.move_(_req._body_buf, _req._cnt_len - _req._body_read);

	_req._body_read += n_move;
	if (_req._body_read == _req._cnt_len) {
		_state = REQ_HOLD;
	}
	return true;
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
			return false;
		}
	case REQ_HEADER_PARSING: {
		if (header_field_parser_() == false) {
			return false;
		}

		if (_req.set_uri_info_and_cookie_(*this) == false) {
			return false;
		}

		if (_req._uri_loc == NULL
			|| (_req._uri_loc->accepted_methods_flag & _req._req_mthd) == false) {
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
			do_cgi_(cur_event);
			return false;
		}

		switch (_req._req_mthd) {
		case REQ_GET:
		case REQ_HEAD:
			return _req.res_for_get_head_req_(*this);
		case REQ_POST:
		case REQ_PUT:
			return _req.res_for_post_put_req_(*this);
		case REQ_DELETE:
			_req.res_for_delete_req_(*this);
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
	}
	return true;
}

void
Client::disconnect_client_() {
	if (_req._body_fd > 0) {
		remove(_req._uri_resolv.script_filename_.c_str());
		close(_req._body_fd);
	}
	if (_res._body_fd > 0) {
		close(_res._body_fd);
	}
	if (_cgi._read_from_cgi_fd > 0) {
		close(_cgi._read_from_cgi_fd);
	}
	if (_cgi._write_to_cgi_fd > 0) {
		add_change_list(*change_list, _cgi._write_to_cgi_fd, EVFILT_WRITE, EV_DELETE, 0, 0, this);
		close(_cgi._write_to_cgi_fd);
	}
}

// read only request header & body message
void
Client::read_to_client_buffer_(struct kevent* cur_event) {
	int n_read = _rdbuf->read_(cur_event->ident, _buf);
	if (n_read <= 0) {
		return;
	}

	if (_state != REQ_HOLD) {
		req_res_controller_(cur_event);
	}
}

void
Client::read_to_cgi_buffer_(struct kevent* cur_event) {
	int n_read = _rdbuf->read_(cur_event->ident, _cgi._from_cgi);

	if (n_read <= 0) {
		return;
	}
	if (_cgi._cgi_state != CGI_HOLD && _req._cnt_len != SIZE_T_MAX) {
		_cgi.cgi_controller_(*this);
	}
}

void
Client::read_to_res_buffer_(struct kevent* cur_event) {
	int n_read = _rdbuf->read_(cur_event->ident, _res._res_buf);

	if (n_read <= 0) {
		return;
	}

	_res._body_read += n_read;
	if (_res._body_read == _res._body_size) {
		close(cur_event->ident);
		add_change_list(*change_list, _client_fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, this);
	}
}

bool
Client::write_for_upload_(struct kevent* cur_event) {
	int n_write;

	if (_req._is_chnkd) {
		n_write = _chnkd._chnkd_body.write_(cur_event->ident);
		if (n_write < 0) {
			return false;
		}
		_req._body_read += n_write;
		if (_req._body_read == _req._cnt_len) {
			close(cur_event->ident);
			add_change_list(*change_list, cur_event->ident, EVFILT_WRITE, EV_DELETE, 0, 0, this);
			_state = REQ_HOLD;
			add_change_list(*change_list, _client_fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, this);
			_res.make_response_header_(*this);
		}
		return true;
	} else {
		n_write = _req._body_buf.write_(cur_event->ident);
		if (n_write < 0) {
			return false;
		}

		if (_req._body_buf.buf_size_() == 0 && _req._body_read == _req._cnt_len) {
			close(cur_event->ident);
			add_change_list(*change_list, cur_event->ident, EVFILT_WRITE, EV_DELETE, 0, 0, this);
			_state = REQ_HOLD;
			add_change_list(*change_list, _client_fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, this);
			_res.make_response_header_(*this);
		}
	}
	return true;
}

bool
Client::write_to_cgi_(struct kevent* cur_event) {
	int n_write;

	n_write = _cgi._to_cgi.write_(cur_event->ident);

	if (n_write < 0) {
		return false;
	}
	_cgi._cgi_read += n_write;
	if (_cgi._cgi_read == _req._cnt_len) {
		close(cur_event->ident);
		add_change_list(*change_list, cur_event->ident, EVFILT_WRITE, EV_DELETE, 0, 0, this);
		_cgi._cgi_state = CGI_HEADER;
		_state			= REQ_HOLD;
	}
	return true;
}

bool
Client::write_response_() {
	// no chunked case.
	size_t n_write;
	if (_res._header_sent == false) {

#ifdef CONSOLE_LOG
		spx_console_log_(_res._res_header.substr(0, _res._res_header.find_first_of('\r')),
						 this->_established_time,
						 _res._body_size,
						 _req._req_mthd,
						 _req._uri);
#endif
#ifdef DEBUG
		n_write = write(STDOUT_FILENO, _res._res_header.c_str(), _res._res_header.size()); // check response log
#endif
		n_write = write(_client_fd, _res._res_header.c_str(), _res._res_header.size());
		if (n_write < 0) {
			return false;
		}

		if (n_write != _res._res_header.size()) {
			_res._res_header.erase(0, n_write);
			return true;
		} else {
			_res._res_header.clear();
			_res._header_sent = true;
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
			n_write = _cgi._from_cgi.write_(_client_fd);
			if (n_write < 0) {
				return false;
			}

			_res._body_write += n_write;
			if ((_cgi._is_chnkd && _res._body_write == _cgi._cgi_read)
				|| (_cgi._is_chnkd == false && _res._body_write == _res._body_size)) {
				add_change_list(*change_list, _client_fd, EVFILT_WRITE, EV_DELETE, 0, 0, this);
				if (_state != REQ_SKIP_BODY_CHUNKED) {
					_state = REQ_CLEAR;
				}
				_res._write_finished = true;
			}
		} else {
			n_write = _res._res_buf.write_(_client_fd);
			if (n_write < 0) {
				return false;
			}
			_res._body_write += n_write;
			if (_res._body_write == _res._body_size) {
				add_change_list(*change_list, _client_fd, EVFILT_WRITE, EV_DELETE, 0, 0, this);
				if (_state != REQ_SKIP_BODY_CHUNKED) {
					_state = REQ_CLEAR;
				}
				_res._write_finished = true;
			}
		}
		return true;
	}
}
