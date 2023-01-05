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
	, _rdbuf(BUFFER_SIZE, IOV_VEC_SIZE)
	, _rd_ofs(0)
	, _state(REQ_LINE_PARSING)
	, _port_info()
	, _sockaddr(NULL)
	, storage() {
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
		if (_rdbuf.get_crlf_line_(req_line) == false) {
			// read more case.
			return false;
		}
		if (req_line.size())
			break;
	}
	spx_log_("REQ_LINE_PARSER ok");
	// bad request will return false and disconnect client.
	return request_line_check_(req_line);
}

bool
Client::header_field_parser_() {
	std::string key_val;
	int			idx;

	while (true) {
		key_val.clear();
		if (_rdbuf.get_crlf_line_(key_val)) {
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
		_req._body_size = strtoul((it->second).c_str(), NULL, 10);
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
		CgiModule cgi(req._uri_resolv_, req.field_, req.uri_loc_);

		cgi.made_env_for_cgi_(req.req_type_);

		char const* script[3];
		script[0] = req.uri_resolv_.cgi_loc_->cgi_path_info.c_str();
		script[1] = req.uri_resolv_.script_filename_.c_str();
		script[2] = NULL;

		execve(script[0], const_cast<char* const*>(script), const_cast<char* const*>(&cgi.env_for_cgi_[0]));
		exit(EXIT_FAILURE);
	}
	// parent
	close(write_to_cgi[0]);
	if (req.req_type_ & (REQ_GET | REQ_HEAD | REQ_DELETE)) {
		close(write_to_cgi[1]);
	} else {
		fcntl(write_to_cgi[1], F_SETFL, O_NONBLOCK);
		add_change_list(change_list, write_to_cgi[1], EVFILT_WRITE, EV_ADD | EV_DISABLE, 0, 0, cur_event->udata);
		req.cgi_write_fd_ = write_to_cgi[1];
	}
	fcntl(read_from_cgi[0], F_SETFL, O_NONBLOCK);
	close(read_from_cgi[1]);
	// buf.req.cgi_in_fd_	= write_to_cgi[1];
	// buf.req.cgi_out_fd_ = read_from_cgi[0];
	add_change_list(change_list, read_from_cgi[0], EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, cur_event->udata);
	add_change_list(change_list, pid, EVFILT_PROC, EV_ADD, NOTE_EXIT, 0, cur_event->udata);
	return true;
}

bool
Client::host_check_(std::string& host) {
	if (host.size() && host.find_first_of(" \t") == std::string::npos) {
		return true;
	}
	return false;
}

// Client::header_parsing(){

// }

bool
Client::req_res_controller_(struct kevent* cur_event) {
	switch (_state) {
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
			_state = E_BAD_REQ;
			return false;
		}
		_req._uri_loc = _req._serv_info->get_uri_location_t_(_req._uri, _res._uri_resolv);

		if (_req._uri_loc) {
			_req._body_limit = _req._uri_loc->client_max_body_size;
		}
#ifdef DEBUG
		spx_log_("\nreq->uri_loc_ : ", req->uri_loc_);
		_req.uri_resolv_.print_(); // NOTE :: add by yoma.
#endif

		// NOTE :: add by space.
		// if session exist in request
		// 1. storage - find session(session ID from request)
		// 2. session - find session value (sessionKey)
		// 	2-1. increase session value
		//  2-2. save session value
		// 3. storage - save session state
		cookie_t			   cookie;
		req_header_t::iterator req_cookie = _req._header.find("cookie");
		if (req_cookie != _req._header.end()) {
			spx_log_("COOKIE", "FOUND");
			std::string req_cookie_value = (*req_cookie).second;
			spx_log_("REQ_COOKIE_VALUE", req_cookie_value);
			if (!req_cookie_value.empty()) {
				cookie.parse_cookie_header(req_cookie_value);
				cookie_t::key_val_t::iterator find_cookie = cookie.content.find("sessionID");
				if (find_cookie == cookie.content.end() || ((*find_cookie).second).empty() || !storage.is_key_exsits((*find_cookie).second)) {
					spx_log_("COOKIE ERROR ", "Invalid_COOKIE");
					spx_log_("MAKING NEW SESSION");
					std::string hash_value = storage.make_hash(_client_fd);
					storage.add_new_session(hash_value);
					_req.session_id = SESSIONID + hash_value;
				} else {
					session_t& session = storage.find_value_by_key((*find_cookie).second);
					session.count_++;
					_req.session_id = SESSIONID + (*find_cookie).second;
					spx_log_("SESSIONCOUNT", session.count_);
				}
			}
		} else // if first connection or (no session id on cookie)
		{
			spx_log_("MAKING NEW SESSION");
			std::string hash_value = storage.make_hash(_client_fd);
			storage.add_new_session(hash_value);
			_req.session_id = SESSIONID + hash_value;
		}

		// COOKIE & SESSION END

		if (_req._uri_loc == NULL || (_req._uri_loc->accepted_methods_flag & _req._req_mthd) == false) {
			spx_log_("uri_loc == NULL or not allowed");
			if (_req._uri_loc == NULL) {
				make_error_response_(HTTP_STATUS_NOT_FOUND);
			} else {
				make_error_response_(HTTP_STATUS_METHOD_NOT_ALLOWED);
			}
			if (_req._body_fd != -1) {
				add_change_list(*change_list, _req._body_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, this);
			}
			if (_req._is_chnkd) {
				_req._body_size = -1;
				_state			= REQ_SKIP_BODY_CHUNKED;
			} else {
				if (_req._body_size != 0) {
					_state = REQ_SKIP_BODY;
				} else {
					_state = REQ_LINE_PARSING;
				}
			}
			return false;
		} else if (_res._uri_resolv.is_cgi_) {
			// cgi case:
			if (cgi_handler_(cur_event) == false) {
				make_error_response_(HTTP_STATUS_INTERNAL_SERVER_ERROR);
				if (_req._body_fd != -1) {
					add_change_list(*change_list, _req._body_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, this);
				}
				_state = REQ_HOLD;
			} else {
				spx_log_("cgi true. forked, open pipes. flag", req_res_queue_.front().second.flag_ & WRITE_READY);
				if (_req._is_chnkd & TE_CHUNKED) {
					if._body_size & (REQ_GET | REQ_HEAD | REQ_DELETE)) {
							_state = REQ_SKIP_BODY_CHUNKED;
						}
					else {
						_state = _body_size;
					}
				} else {
					if (_req.content_length_ != 0) {
						if (_req.req_type_ & (REQ_GET | REQ_HEAD | REQ_DELETE)) {
							_state = REQ_SKIP_BODY;
						} else {
							_state = REQ_HOLD;
						}
					} else {
						_state = REQ_LINE_PARSING;
					}
				}
			}
			spx_log_("queue size", req_res_queue_.size());
			// req_res_queue_.front().second.flag_ &= ~(WRITE_READY);
			return false;
		}

		// spx_log_("req_uri set ok");
		switch (_req.req_type_) {
		case REQ_GET:
			spx_log_("REQ_GET");
			make_response_header();
			// spx_log_("RES_OK");
			if (_req._body_fd == -1) {
				spx_log_("No file descriptor");
			} else {
				spx_log_("READ event added");
				add_change_list(change_list, _req._body_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, this);
			}
			// _req.flag_ |= WRITE_READY;
			if (_req._is_chnkd & TE_CHUNKED) {
				_state = REQ_S._body_size;
			} else if (_req.body_size_ > 0) {
				skip_size_ = _req.body_size_;
				_state	   = REQ_SKIP_BODY;
				_body_size
			} else {
				_state = REQ_LINE_PARSING;
				if (rdsaved_.size() == rdchecked_) {
					rdsaved_.clear();
					rdchecked_ = 0;
				}
			}
			break;
		case REQ_HEAD:
			// same with REQ_GET without body.
			spx_log_("REQ_HEAD");
			make_response_header();
			// spx_log_("RES_OK");
			if (_req._body_fd == -1) {
				spx_log_("No file descriptor");
			} else {
				spx_log_("READ event added");
				add_change_list(change_list, _req._body_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, this);
			}
			// _req.flag_ |= WRITE_READY;
			if (_req._is_chnkd & TE_CHUNKED) {
				_state = REQ_S._body_size;
			} else if (_req.body_size_ > 0) {
				skip_size_ = _req.body_size_;
				_state	   = REQ_SKIP_BODY;
				_body_size
			} else {
				_state = REQ_LINE_PARSING;
				if (rdsaved_.size() == rdchecked_) {
					rdsaved_.clear();
					rdchecked_ = 0;
				}
			}
			break;
		case REQ_POST:

			_req.file_path_ = _req.uri_resolv_.script_filename_;
			_req._body_fd	= open(
				  _req.uri_resolv_.script_filename_.c_str(),
				  O_WRONLY | O_CREAT | O_NONBLOCK | O_APPEND, 0644);
			// error case
			if (_req._body_fd < 0) {
				// 405 not allowed error with keep-alive connection.
				/*
				make_error_response_(HTTP_STATUS_METHOD_NOT_ALLOWED);
				if (_req._body_fd != -1) {
					add_change_list(change_list, _req._body_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, this);
				}
				return false;
				*/
				_req._body_fd = open(
					(_req.uri_resolv_.script_filename_).c_str(),
					O_WRONLY | O_CREAT | O_NONBLOCK | O_APPEND, 0644);
				_req.file_path_ = _req.uri_resolv_.script_filename_;
				// _req.flag_ |= WRITE_READY;
			}

			spx_log_("control - REQ_POST fd: ", _req._body_fd);
			// _req.flag_ |= REQ_FILE_OPEN;
			if (_req._is_chnkd & TE_CHUNKED) {
				_req._body_size = -1;
				_state			= REQ_BODY_CHUNKED;
				add_change_list(change_list, _req._body_fd, EVFILT_WRITE, EV_ADD | EV_DISABLE, 0, 0, this);
				return _body_size(change_list, cur_event);
			} else {
				add_change_list(change_list, _req._body_fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, this);
				_state = REQ_HOLD;
			}
			return false;

		case REQ_PUT:

			_req.file_path_ = _req.uri_resolv_.script_filename_;
			_req._body_fd	= open(
				  _req.uri_resolv_.script_filename_.c_str(),
				  O_WRONLY | O_CREAT | O_NONBLOCK | O_TRUNC, 0644);
			// error case
			if (_req._body_fd < 0) {
				// 405 not allowed error with keep-alive connection.
				make_error_response_(HTTP_STATUS_METHOD_NOT_ALLOWED);
				if (_req._body_fd != -1) {
					add_change_list(change_list, _req._body_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, this);
				}
				// _req.flag_ |= WRITE_READY;
				return false;
			}

			spx_log_("control - REQ_PUT fd: ", _req._body_fd);
			// _req.flag_ |= REQ_FILE_OPEN;
			if (_req._is_chnkd & TE_CHUNKED) {
				_req._body_size = -1;
				_state			= REQ_BODY_CHUNKED;
				add_change_list(change_list, _req._body_fd, EVFILT_WRITE, EV_ADD | EV_DISABLE, 0, 0, this);
				return _body_size(change_list, cur_event);
			} else {
				add_change_list(change_list, _req._body_fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, this);
				_state = REQ_HOLD;
			}
			return false;

		case REQ_DELETE:
			if (_req.header_ready_ == 0) {
			}
			//  HEADER?
			break;
		case REQ_UNDEFINED:
			// error. disconnect client.
			break;
		}
		break;
	}
	case REQ_BODY_CHUNKED: {
		// rdsaved_ -> chunked_body_buffer_
		buffer_t::iterator crlf_pos = rdsaved_.begin() + rdchecked_;
		uint32_t		   size		= 0;
		int				   start_line_end;
		req_field_t&	   req = _req;

		spx_log_("controller - req body chunked. body limit", req.body_limit_);
		while (req.body_size_ <= req.body_limit_) {
			crlf_pos = std::find(crlf_pos, rdsaved_.end(), LF);
			if (crlf_pos != rdsaved_.end()) {
				start_line_end = crlf_pos - rdsaved_.begin() + 1;
				if (spx_chunked_syntax_start_line(*this, size, req.field_) != -1) {
					if (rdsaved_.size() >= start_line_end + size + 2) {
						rdchecked_ = start_line_end + size;
						req.body_size_ += size;
						if (size != 0) {
							// can parse chunked size.
							req.chunked_body_buffer_.insert(req.chunked_body_buffer_.end(), crlf_pos + 1, crlf_pos + 1 + size);
							// chunked valid check
							if (rdsaved_[rdchecked_++] != CR
								|| rdsaved_[rdchecked_++] != LF) {
								// chunked error
								make_error_response_(HTTP_STATUS_BAD_REQUEST);
								if (_req._body_fd != -1) {
									add_change_list(change_list, _req._body_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, this);
								}
								_state = REQ_HOLD;
								return false;
							}
							crlf_pos = rdsaved_.begin() + rdchecked_;
							spx_log_("chunked parsed ok");
						} else {
							// chunked last
							spx_log_("chunked last piece");
							if (rdsaved_[start_line_end] == CR) {
								spx_log_("chunked last no extension");
								// no extention
								rdchecked_ += 2;
								req.content_length_ = req.body_size_;
								// _state		= REQ_LINE_PARSING;
								if (req.content_length_ > req.body_limit_) {
									break;
								}
								if (_req.uri_resolv_.is_cgi_) {
									add_change_list(change_list, _req.cgi_write_fd_, EVFILT_WRITE, EV_ENABLE, 0, 0, this);
								} else {
									add_change_list(change_list, req._body_fd, EVFILT_WRITE, EV_ENABLE, 0, 0, this);
								}
								return false;
							} else {
								spx_log_("chunked extension");
								// yoma's code..? end check..??
								return false;
							}
						}
					} else {
						// try next.
						spx_log_("chunked try next");
						// rdsaved_.erase(rdsaved_.begin(), rdsaved_.begin() + rdchecked_);
						// rdchecked_ = 0;
						return false;
					}
				} else {
					// chunked error
					spx_log_("chunked error");
					flag_ |= E_BAD_REQ;
					make_error_response_(HTTP_STATUS_BAD_REQUEST);
					if (_req._body_fd != -1) {
						add_change_list(change_list, _req._body_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, this);
					}
					_state = REQ_HOLD;
					return false;
				}
			} else {
				// chunked start line not exist.
				return false;
			}
		}
		// if (req.body_size_ > req.body_limit_) {
		// send over limit.
		close(req._body_fd);
		remove(req.file_path_.c_str());
		add_change_list(change_list, req._body_fd, EVFILT_WRITE, EV_DISABLE | EV_DELETE, 0, 0, NULL);
		make_error_response_(HTTP_STATUS_PAYLOAD_TOO_LARGE);
		if (_req._body_fd != -1) {
			add_change_list(change_list, _req._body_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, this);
		}
		_state = REQ_SKIP_BODY_CHUNKED;
		// _req.flag_ |= WRITE_READY;
		// flag_ |= E_BAD_REQ;
		// }
		return false;
	}

	case REQ_SKIP_BODY:
		if (skip_size_) {
			if (skip_size_ >= rdsaved_.size() - rdchecked_) {
				skip_size_ -= rdsaved_.size() - rdchecked_;
				rdsaved_.clear();
				rdchecked_ = 0;
			} else {
				rdchecked_ += skip_size_;
				skip_size_ = 0;
			}
		}
		// req_field_t* req = &_req;
		// if (req->)
		// 	// std::min(req->req->);
		// 	if (req->body_recieved_ == req->body_limit_) {
		// 		_state = REQ_LINE_PARSING;
		// 	}
		break;

	case REQ_SKIP_BODY_CHUNKED: {
		spx_log_("skip body chunked part!!!!!!");

		// rdsaved_ -> chunked_body_buffer_
		buffer_t::iterator crlf_pos = rdsaved_.begin() + rdchecked_;
		uint32_t		   size		= 0;
		int				   start_line_end;

		while (true) {
			crlf_pos = std::find(crlf_pos, rdsaved_.end(), LF);
			if (crlf_pos != rdsaved_.end()) {
				// start_line_end: the next position from LF
				start_line_end = crlf_pos - rdsaved_.begin() + 1;
				if (spx_chunked_syntax_start_line(*this, size, _req.field_) != -1) {
					if (size != 0) {
						spx_log_("chunked size != 0");
						if (rdsaved_.size() - start_line_end > size) {
							// can parse chunked size.
							rdchecked_ = start_line_end + size;
							_req.body_size_ += size;
						} else {
							// try next.
							rdsaved_.erase(rdsaved_.begin(), rdsaved_.begin() + rdchecked_);
							rdchecked_ = 0;
							break;
						}
					} else {
						// chunked last
						spx_log_("chunked last");
						if (rdsaved_[start_line_end] == CR) {
							spx_log_("chunked last no extension");
							// no extention
							rdchecked_ = start_line_end + 2;
							// _req.content_length_ = _req.body_size_;
							_state = REQ_LINE_PARSING;
							break;
						} else {
							// yoma's code..? end check..??
						}
					}
				} else {
					// chunked error
					flag_ |= E_BAD_REQ;
					make_error_response_(HTTP_STATUS_BAD_REQUEST);
					return false;
				}
			} else {
				// chunked start line not exist.
				break;
			}
		}
		if (_req.chunked_checked_ == _req.content_length_) {
			_state = REQ_LINE_PARSING;
			return true;
		}
		// if (_req.body_size_ > _req.body_limit_) {
		// 	// send over limit.
		// 	close(_req._body_fd);
		// 	make_error_response_(HTTP_STATUS_RANGE_NOT_SATISFIABLE);
		// 	if (_req._body_fd != -1) {
		// 		add_change_list(change_list, _req._body_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, this);
		// 	}
		// 	// _req.flag_ |= WRITE_READY;
		// 	flag_ |= E_BAD_REQ;
		// }
		return false;
	}

		// case REQ_CGI: {
		// 	req_field_t &req = _req;
		// 	res_field_t &res = _req;

		// 	if (req._is_chnkd & TE_CHUNKED) {
		// 		._body_size
		// 	} else {
		// 		if (req.content_length_ == 0) {
		_body_size
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
Client::disconnect_client(std::vector<struct kevent>& change_list) {
	// client status, tmp file...? check.
	add_change_list(change_list, client_fd_, EVFILT_READ, EV_DISABLE | EV_DELETE, 0, 0,
					NULL);
	add_change_list(change_list, client_fd_, EVFILT_WRITE, EV_DISABLE | EV_DELETE, 0,
					0, NULL);
	while (req_res_queue_.size()) {
		if (req_res_queue_.front().second.body_size_ != req_res_queue_.front().second.body_read_) {
			close(req_res_queue_.front().second._body_fd);
		}
		req_res_queue_.pop();
	}
	// exit(1);
	close(client_fd_);
}

void
Client::write_filter_enable(event_list_t& change_list, struct kevent* cur_event) {
	if (req_res_queue_.size() != 0
		&& req_res_queue_.front().second.flag_ & WRITE_READY) {
		add_change_list(change_list, cur_event->ident, EVFILT_WRITE, EV_ENABLE, 0, 0, this);
	}
}

// read only request header & body message
void
Client::read_to_client_buffer(std::vector<struct kevent>& change_list,
							  struct kevent*			  cur_event) {
	int n_read = read(client_fd_, rdbuf_, BUFFER_SIZE);
	if (n_read < 0) {
		// TODO: error handle
		return;
	}
#ifdef DEBUG
	write(STDOUT_FILENO, &rdbuf_, std::min(200, n_read));
#endif
	rdsaved_.insert(rdsaved_.end(), rdbuf_, rdbuf_ + n_read);
	spx_log_("\nread_to_client", n_read);
	spx_log_("read_to_client state", _state);
	spx_log_("queue size", req_res_queue_.size());
	if (_state != REQ_HOLD) {
		req_res_controller(change_list, cur_event);
		spx_log_("req_res_controller check finished. buf stat", _state);
	};
	// spx_log_("enable write", req_res_queue_.front().second.flag_ & WRITE_READY);
	write_filter_enable(change_list, cur_event);
}

bool
Client::cgi_header_parser() {
	std::string		   header_field_line;
	res_field_t&	   res		= _req;
	buffer_t&		   cgi_buf	= res.cgi_buffer_;
	buffer_t::iterator crlf_pos = cgi_buf.begin() + res.cgi_checked_;
	int				   idx;

	while (true) {
		crlf_pos = std::find(crlf_pos, cgi_buf.end(), LF);
		if (crlf_pos != cgi_buf.end()) {
			header_field_line.clear();
			if (cgi_buf.begin() + res.cgi_checked_ != crlf_pos) {
				header_field_line.assign(cgi_buf.begin() + res.cgi_checked_, crlf_pos);
			}
			spx_log_("cgi header!!!", header_field_line);
			++crlf_pos;
			res.cgi_checked_ = crlf_pos - cgi_buf.begin();
			if (header_field_line.size() < 2) {
				// request header parsed.
				spx_log_("cgi parsed!!!", header_field_line);
				break;
			}
			// if (spx_http_syntax_header_line(header_field_line) == -1) {
			// 	spx_log_("cgi syntax error");
			// 	flag_ |= E_BAD_REQ;
			// 	// error_res();
			// 	return false;
			// }
			idx = header_field_line.find(':');
			if (idx != std::string::npos) {
				for (std::string::iterator it = header_field_line.begin();
					 it != header_field_line.begin() + idx; ++it) {
					if (isalpha(*it)) {
						*it = tolower(*it);
					}
				}
				size_t tmp = idx + 1;
				while (tmp < header_field_line.size() && syntax_(ows_, header_field_line[tmp])) {
					++tmp;
				}
				res.cgi_field_[header_field_line.substr(0, idx)]
					= header_field_line.substr(tmp, header_field_line.size() - tmp);
			}
			continue;
		}
		// cgi_buf.erase(cgi_buf.begin(), cgi_buf.begin() + res.cgi_checked_);
		// res.cgi_checked_ = 0;
		return false;
	}
	return true;
}

bool
Client::cgi_controller() {
	res_field_t& res = _req;

	// switch (state) {
	// case CGI_HEADER: {
	if (cgi_header_parser() == false) {
		// read more?
		spx_log_("cgi_header_parser false");
		// break;
	}
	std::map<std::string, std::string>::iterator it;

	it = res.cgi_field_.find("content-length");
	if (it != res.cgi_field_.end()) {
		res.cgi_size_  = strtol(it->second.c_str(), NULL, 10);
		res.cgi_state_ = CGI_HOLD;
		// TODO: make_cgi_response_header.
		// make_cgi_response_header();
	} else {
		// no content-length case.
		res.cgi_state_ = CGI_BODY_CHUNKED;
		res.cgi_size_  = -1;
	}
	spx_log_("cgi controller. cgi_buffer size", res.cgi_buffer_.size());
	spx_log_("cgi controller. cgi_checked", res.cgi_checked_);
	make_cgi_response_header();
	// _req.flag_ |= WRITE_READY;
	// }
	// case CGI_BODY_CHUNKED:
	// 	// chunked logic
	// 	spx_log_("cgi body chunked");
	// 	res.cgi_state_ = CGI_HOLD;
	// 	break;
	// }
	return true;
}

void
Client::read_to_cgi_buffer(event_list_t& change_list, struct kevent* cur_event) {
	int n_read = read(cur_event->ident, rdbuf_, BUFFER_SIZE);
	// write(STDOUT_FILENO, rdbuf_, n_read);
	if (n_read < 0) {
		// TODO: error handle
		disconnect_client(change_list);
		return;
	}
	_req.cgi_buffer_.insert(
		_req.cgi_buffer_.end(), rdbuf_, rdbuf_ + n_read);
	spx_log_("read to cgi buffer. n_read", n_read);
	spx_log_("read to cgi buffer. buf_size", _req.cgi_buffer_.size());

	// if (cgi_controller(_req.cgi_state_, change_list) == false) {
	// 	spx_log_("cgi controller false");
	// 	return;
	// }
	// spx_log_("cgi controller ok");
	return;
}

void
Client::read_to_res_buffer(event_list_t& change_list, struct kevent* cur_event) {
	int n_read = read(cur_event->ident, rdbuf_, BUFFER_SIZE);
	if (n_read < 0) {
		// TODO: error handle
		return;
	}
	spx_log_("read_to_res_buffer");
	_req.res_buffer_.insert(
		_req.res_buffer_.end(), rdbuf_, rdbuf_ + n_read);
	_req.body_read_ += n_read;
	// if all content read, close fd.
	if (_req.body_read_ == _req.body_size_) {
		close(cur_event->ident);
		add_change_list(change_list, cur_event->ident, EVFILT_READ, EV_DISABLE | EV_DELETE, 0, 0, NULL);
	}
}

void
Client::make_error_response_(http_status error_code) {
	res_field_t& res = req_res_queue_.front().second;
	req_field_t& req = req_res_queue_.front().first;

	res.status_		 = http_status_str(error_code);
	res.status_code_ = error_code;

	// res.headers_.push_back(header("Server", "SpaceX/12.26"));

	// if (error_code == HTTP_STATUS_BAD_REQUEST)
	// 	res.headers_.push_back(header(CONNECTION, CONNECTION_CLOSE));
	// else
	res.headers_.push_back(header(CONNECTION, KEEP_ALIVE));

	// page_path null case added..
	std::string page_path;
	int			error_req_fd;
	if (req.serv_info_) {
		page_path = req.serv_info_->get_error_page_path_(error_code);
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

		ss << error_page.length();
		res.headers_.push_back(header(CONTENT_LENGTH, ss.str()));
		res.headers_.push_back(header(CONTENT_TYPE, MIME_TYPE_HTML));
		std::string tmp = res.make_to_string();
		res.write_to_response_buffer(tmp);
		if (req.req_type_ != REQ_HEAD)
			res.write_to_response_buffer(error_page);
		return;
	}
	res._body_fd = error_req_fd;

	res.setContentType(page_path);
	int	  req_method	 = req.req_type_;
	off_t content_length = res.setContentLength(error_req_fd);

	if (req_method == REQ_GET)
		res.buf_size_ += content_length;
	res.write_to_response_buffer(res.make_to_string());
}

// this is main logic to make response
void
Client::make_response_header() {
	const req_field_t& req
		= _req;
	res_field_t& res
		= _req;

	const std::string& uri		  = res.uri_resolv_.script_filename_;
	int				   req_fd	  = -1;
	int				   req_method = req.req_type_;
	std::string		   content;

	// Set Date Header
	res.setDate();
	if (!req.session_id.empty())
		res.headers_.push_back(header("Set-Cookie", req.session_id));

	// Redirect
	if (req.uri_loc_ != NULL && !(req.uri_loc_->redirect.empty())) {
		make_redirect_response();
		return;
	}

	switch (req_method) {
	case REQ_HEAD:
	case REQ_GET:
		if (uri[uri.size() - 1] != '/') {
			spx_log_("uri.cstr()", uri.c_str());
			req_fd = res.file_open(uri.c_str());
		} else {
			spx_log_("folder skip");
			// make_error_response_(HTTP_STATUS_NOT_FOUND);
			// return;
		}
		spx_log_("uri_locations", req.uri_loc_);
		spx_log_("req_fd", req_fd);
		if (req_fd == 0) {
			spx_log_("folder skip");
			make_error_response_(HTTP_STATUS_FORBIDDEN);
			return;
		} else if (req_fd == -1 && (req.uri_loc_ == NULL || req.uri_loc_->autoindex_flag == Kautoindex_off)) {
			make_error_response_(HTTP_STATUS_NOT_FOUND);
			return;
		} else if (req_fd == -1 && req.uri_loc_->autoindex_flag == Kautoindex_on) {
			// if (res.uri_resolv_.is_same_location_) {
			spx_log_("uri=======", res.uri_resolv_.script_filename_);
			content = generate_autoindex_page(req_fd, res.uri_resolv_);
			std::stringstream ss;
			ss << content.size();
			res.headers_.push_back(header(CONTENT_LENGTH, ss.str()));
			// ???? autoindex fail case?
			if (content.empty()) {
				make_error_response_(HTTP_STATUS_FORBIDDEN);
				return;
			}
		}
		if (req_fd != -1) {
			spx_log_("res_header");
			res.setContentType(uri);
			off_t content_length = res.setContentLength(req_fd);
			if (req_method == REQ_GET) {
				res._body_fd = req_fd;
				res.buf_size_ += content_length;
			} else {
				res._body_fd = -1;
				close(req_fd);
			}
			// res.headers_.push_back(header("Accept-Ranges", "bytes"));
		} else {
			// autoindex case?
			res.headers_.push_back(header(CONTENT_TYPE, MIME_TYPE_HTML));
		}
		break;
	case REQ_POST:
	case REQ_PUT:
		res.headers_.push_back(header(CONTENT_LENGTH, "0"));
		break;
	}
	// res.headers_.push_back(header("Set-Cookie", "SESSIONID=123456;"));

	// settting response_header size  + content-length size to res_field
	res.headers_.push_back(header(CONNECTION, KEEP_ALIVE));
	res.write_to_response_buffer(res.make_to_string());
	if (!content.empty()) {
		res.write_to_response_buffer(content);
	}
}

void
Client::make_cgi_response_header() {
	const req_field_t& req		  = _req;
	res_field_t&	   res		  = _req;
	int				   req_method = req.req_type_;

	// Set Date Header
	res.setDate();
	spx_log_("make_cgi_res_header");
	std::map<std::string, std::string>::iterator it;

	res.headers_.clear();
	it = res.cgi_field_.find("status");
	if (it != res.cgi_field_.end()) {
		res.status_code_ = strtol(it->second.c_str(), NULL, 10);
	} else {
		res.status_code_ = 200;
	}
	// res.headers_.push_back(header("Set-Cookie", "SESSIONID=123456;"));
	// settting response_header size  + content-length size to res_field
	res.headers_.push_back(header(CONNECTION, KEEP_ALIVE));

	it = res.cgi_field_.find("content-length");
	if (it != res.cgi_field_.end()) {
		res.headers_.push_back(header(CONTENT_LENGTH, it->second));
	} else {
		std::stringstream ss;
		ss << (res.cgi_buffer_.size() - res.cgi_checked_);
		res.headers_.push_back(header(CONTENT_LENGTH, ss.str().c_str()));
		// res.headers_.push_back(header(CONTENT_LENGTH, "0"));
	}
	res.write_to_response_buffer(res.make_to_string());
}

void
Client::make_redirect_response() {
	const req_field_t& req
		= req_res_queue_.front().first;
	res_field_t& res
		= req_res_queue_.front().second;

	spx_log_("uri_loc->redirect", req.uri_loc_->redirect);
	res.status_code_ = HTTP_STATUS_MOVED_PERMANENTLY;
	res.status_		 = http_status_str(HTTP_STATUS_MOVED_PERMANENTLY);
	res.headers_.push_back(header("Location", req.uri_loc_->redirect));
	res.write_to_response_buffer(res.make_to_string());
}

bool
Client::write_for_upload(event_list_t& change_list, struct kevent* cur_event) {
	int			 n_write;
	req_field_t& req = _req;
	size_t		 buf_len;

	if (_req._is_chnkd & TE_CHUNKED) {
		._body_size
			buf_len
			= req.chunked_body_buffer_.size() - req.chunked_checked_;
		// spx_log_("write_for_upload - content len", req.content_length_);
		if (_body_size <= buf_len) {
			spx_log_("write_for_upload: MAX_BUF ");
			n_write = write(cur_event->ident, &req.chunked_body_buffer_[req.chunked_checked_], WRITE_BUFFER_MAX);
			req.body_read_ += n_write;
			req.chunked_checked_ += n_write;
		} else {
			n_write = write(cur_event->ident, &req.chunked_body_buffer_[req.chunked_checked_], buf_len);
			spx_log_("write_for_upload: chunked_buf_len ", n_write);
			req.body_read_ += n_write;
			req.chunked_checked_ += n_write;
			if (req.chunked_checked_ == req.chunked_body_buffer_.size()) {
				req.chunked_body_buffer_.clear();
				req.chunked_checked_ = 0;
			}
		}
		if (req.body_read_ == req.content_length_) {
			spx_log_("read_end. queue size", req_res_queue_.size());
			_req.flag_ |= READ_BODY_END;
			// add_change_list(change_list, cur_event->ident, EVFILT_WRITE, EV_DISABLE | EV_DELETE, 0, 0, NULL);
			_state = REQ_LINE_PARSING;
			spx_log_("chunked upload write len", req.body_read_);
		}
		return true;
	} else {
		buf_len	   = rdsaved_.size() - rdchecked_;
		size_t len = req.body_size_ - req.body_read_;

		if (_req.body_limit_ < _req.body_size_) {
			close(cur_event->ident);
			remove(_req.file_path_.c_str());
			add_change_list(change_list, cur_event->ident, EVFILT_WRITE, EV_DISABLE | EV_DELETE, 0, 0, NULL);
			return false;
		}

		if (WRITE_BUFFER_MAX <= std::min(buf_len, len)) {
			spx_log_("write_for_upload: MAX_BUF ");
			n_write = write(cur_event->ident, &rdsaved_[rdchecked_], WRITE_BUFFER_MAX);
		} else if (buf_len <= len) {
			spx_log_("write_for_upload: buf_len: ", buf_len);
			n_write = write(cur_event->ident, &rdsaved_[rdchecked_], buf_len);
		} else {
			spx_log_("write_for_upload: len: ", len);
			n_write = write(cur_event->ident, &rdsaved_[rdchecked_], len);
		}
		spx_log_("body_fd: ", cur_event->ident);
		spx_log_("write len: ", n_write);
		req.body_read_ += n_write;
		rdchecked_ += n_write;
		if (rdsaved_.size() == rdchecked_) {
			rdsaved_.clear();
			rdchecked_ = 0;
		}
	}
	if (_req.body_read_ == _req.body_size_) {
		close(cur_event->ident);
		add_change_list(change_list, cur_event->ident, EVFILT_WRITE, EV_DISABLE | EV_DELETE, 0, 0, NULL);
		_req.flag_ |= READ_BODY_END;
		_state = REQ_HOLD;
	}
	return true;
}

bool
Client::write_to_cgi(struct kevent* cur_event, std::vector<struct kevent>& change_list) {
	req_field_t& req = _req;
	res_field_t& res = _req;
	size_t		 buf_len;
	int			 n_write;

	if (req._is_chnkd & TE_CHUNKED) {
		._body_size req.chunked_body_buffer
					buf_len
			= req.chunked_body_buffer_.size() - req.chunked_checked_;
		if (WRITE_BUFFER_MAX <= buf_len) {
			_body_size("write_to_cgi: MAX_BUF ");
			n_write = write(cur_event->ident, &req.chunked_body_buffer_[req.chunked_checked_], WRITE_BUFFER_MAX);
			req.chunked_checked_ += n_write;
			req.body_read_ += n_write;
		} else {
			spx_log_("write_to_cgi: buf_len ", buf_len);
			n_write = write(cur_event->ident, &req.chunked_body_buffer_[req.chunked_checked_], buf_len);
			req.chunked_checked_ += n_write;
			req.body_read_ += n_write;
			if (req.chunked_checked_ == req.chunked_body_buffer_.size()) {
				req.chunked_body_buffer_.clear();
				req.chunked_checked_ = 0;
			}
		}
		if (req.body_read_ == req.content_length_) {
			_req.flag_ |= READ_BODY_END;
			close(cur_event->ident);
			add_change_list(change_list, cur_event->ident, EVFILT_WRITE, EV_DISABLE | EV_DELETE, 0, 0, NULL);
			_state = REQ_HOLD; //?
			spx_log_("chunked cgi write len", req.body_read_);
		}
		return true;
	} else {
		buf_len	   = rdsaved_.size() - rdchecked_;
		size_t len = req.body_size_ - req.body_read_;
		if (WRITE_BUFFER_MAX <= std::min(buf_len, len)) {
			spx_log_("write_to_cgi: MAX_BUF ");
			n_write = write(cur_event->ident, &rdsaved_[rdchecked_], WRITE_BUFFER_MAX);
		} else if (buf_len <= len) {
			spx_log_("write_to_cgi: buf_len: ", buf_len);
			n_write = write(cur_event->ident, &rdsaved_[rdchecked_], buf_len);
		} else {
			spx_log_("write_to_cgi: len: ", len);
			n_write = write(cur_event->ident, &rdsaved_[rdchecked_], len);
		}
		// spx_log_("body_fd: ", req._body_fd);
		// spx_log_("write len: ", n_write);
		req.body_read_ += n_write;
		rdchecked_ += n_write;
		if (rdsaved_.size() == rdchecked_) {
			rdsaved_.clear();
			rdchecked_ = 0;
		}
		if (_req.body_read_ == _req.body_size_) {
			_req.flag_ |= READ_BODY_END;
			// close(cur_event->ident);
			// add_change_list(change_list, cur_event->ident, EVFILT_WRITE, EV_DISABLE | EV_DELETE, 0, 0, NULL);
			// _state = REQ_HOLD;
		}
	}
	return true;
}

bool
Client::write_response(std::vector<struct kevent>& change_list) {
	res_field_t* res = &req_res_queue_.front().second;
	// no chunked case.
	if (req_res_queue_.front().second.uri_resolv_.is_cgi_ == false) {
		int n_write = write(client_fd_, &res->res_buffer_[res->sent_pos_],
							std::min((size_t)WRITE_BUFFER_MAX,
									 res->res_buffer_.size() - res->sent_pos_));
		// #ifdef DEBUG
		if (n_write) {
			write(STDOUT_FILENO, &res->res_buffer_[res->sent_pos_],
				  std::min((size_t)100, res->res_buffer_.size() - res->sent_pos_));
		}
		// #endif
		spx_log_("write_bufsize: ", res->buf_size_);
		if (n_write < 0) {
			spx_log_("write error");
			// client fd error. maybe disconnected.
			// error handle code
			return false;
		}
		res->sent_pos_ += n_write;
		res->buf_size_ -= n_write;
		if (res->res_buffer_.size() == res->sent_pos_) {
			res->res_buffer_.clear();
			res->sent_pos_ = 0;
		}
		spx_log_("write_bufsize: ", res->buf_size_);
		if (res->buf_size_ == 0) {
			req_res_queue_.pop();
			// flag_ &= ~(RDBUF_CHECKED);
		}
	} else {
		if (res->res_buffer_.size()) {
			int n_write = write(client_fd_, &res->res_buffer_[res->sent_pos_],
								std::min((size_t)WRITE_BUFFER_MAX,
										 res->res_buffer_.size() - res->sent_pos_));
#ifdef DEBUG
			if (n_write) {
				write(STDOUT_FILENO, &res->res_buffer_[res->sent_pos_],
					  std::min((size_t)150, res->res_buffer_.size() - res->sent_pos_));
			}
#endif
			// res->headers_.erase(res->headers_.begin(), res->headers_.begin() + n_write);
			if (n_write < 0) {
				spx_log_("write error");
				// client fd error. maybe disconnected.
				// error handle code
				return false;
			}
			res->sent_pos_ += n_write;
			res->buf_size_ -= n_write;
			if (res->res_buffer_.size() == res->sent_pos_) {
				res->res_buffer_.clear();
				res->sent_pos_ = 0;
			}
			if (res->res_buffer_.size() == 0 && res->cgi_buffer_.size() == res->cgi_checked_) {
				req_res_queue_.pop();
				return true;
			}
			return false;
		} else if (res->cgi_buffer_.size()) {
			int n_write = write(client_fd_, &res->cgi_buffer_[res->cgi_checked_],
								std::min((size_t)WRITE_BUFFER_MAX,
										 res->cgi_buffer_.size() - res->cgi_checked_));
#ifdef DEBUG
			if (n_write) {
				write(STDOUT_FILENO, &res->cgi_buffer_[res->cgi_checked_],
					  std::min((size_t)150, res->cgi_buffer_.size() - res->cgi_checked_));
			}
#endif
			spx_log_("cgi body write", n_write);
			res->cgi_checked_ += n_write;
			if (res->cgi_buffer_.size() == res->cgi_checked_) {
				req_res_queue_.pop();
				_state = REQ_LINE_PARSING;
			}
		}
	}
	return true;
}

void
ResField::write_to_response_buffer(const std::string& content) {
	res_buffer_.insert(res_buffer_.end(), content.begin(), content.end());
	buf_size_ += content.size();
	flag_ |= WRITE_READY;
}

std::string
ResField::make_to_string() const {
	std::stringstream stream;
	stream << "HTTP/" << version_major_ << "." << version_minor_ << " " << status_code_ << " " << status_
		   << CRLF;
	for (std::vector<header>::const_iterator it = headers_.begin();
		 it != headers_.end(); ++it)
		stream << it->first << ": " << it->second << CRLF;
	stream << CRLF;
	return stream.str();
}

int
ResField::file_open(const char* dir) const {
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
ResField::setContentLength(int fd) {
	if (fd < 0)
		return 0;
	off_t length = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);

	std::stringstream ss;
	ss << length;
	body_size_ += length;

	headers_.push_back(header(CONTENT_LENGTH, ss.str()));
	return length;
}

void
ResField::setContentType(std::string uri) {

	std::string::size_type uri_ext_size = uri.find_last_of('.');
	std::string			   ext;

	if (uri_ext_size != std::string::npos) {
		ext = uri.substr(uri_ext_size + 1);
	}
	if (ext == "html" || ext == "htm")
		headers_.push_back(header(CONTENT_TYPE, MIME_TYPE_HTML));
	else if (ext == "png")
		headers_.push_back(header(CONTENT_TYPE, MIME_TYPE_PNG));
	else if (ext == "jpg")
		headers_.push_back(header(CONTENT_TYPE, MIME_TYPE_JPG));
	else if (ext == "jpeg")
		headers_.push_back(header(CONTENT_TYPE, MIME_TYPE_JPEG));
	else if (ext == "txt")
		headers_.push_back(header(CONTENT_TYPE, MIME_TYPE_TEXT));
	else
		headers_.push_back(header(CONTENT_TYPE, MIME_TYPE_DEFUALT));
}

void
ResField::setDate(void) {
	std::time_t now			 = std::time(nullptr);
	std::tm*	current_time = std::gmtime(&now);

	char date_buf[32];
	std::strftime(date_buf, sizeof(date_buf), "%a, %d %b %Y %T GMT", current_time);
	headers_.push_back(header("Date", date_buf));
}
