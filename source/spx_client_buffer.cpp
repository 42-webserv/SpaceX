#include "spx_client_buffer.hpp"
#include "spx_cgi_module.hpp"
#include "spx_kqueue_module.hpp"

#include "spx_session_storage.hpp"

ClientBuffer::ClientBuffer()
	: req_res_queue_()
	, rdsaved_()
	, timeout_()
	, client_fd_()
	, port_info_()
	, skip_size_()
	, rdchecked_(0)
	, flag_(0)
	, state_(REQ_LINE_PARSING)
	, rdbuf_() {
}

ClientBuffer::~ClientBuffer() { }

bool
ClientBuffer::request_line_check(std::string& req_line) {
	// request line checker
	if (spx_http_syntax_start_line(req_line,
								   this->req_res_queue_.back().first.req_type_)
		== 0) {
		std::string::size_type r_pos				  = req_line.find_last_of(' ');
		std::string::size_type l_pos				  = req_line.find_first_of(' ');
		this->req_res_queue_.back().first.req_target_ = req_line.substr(
			l_pos + 1, r_pos - l_pos - 1);
		this->req_res_queue_.back().first.http_ver_ = "HTTP/1.1";
		return true;
	}
	return false;
}

bool
ClientBuffer::request_line_parser() {
	std::string		   req_line;
	buffer_t::iterator crlf_pos = this->rdsaved_.begin() + rdchecked_;

	// if (this->rdsaved_.size() == rdchecked_) {
	// 	spx_log_("errrrrr");
	// }

	spx_log_("REQ_LINE_PARSER");
	while (true) {
		crlf_pos = std::find(crlf_pos, this->rdsaved_.end(), LF);
		if (crlf_pos != this->rdsaved_.end()) {
			if (*(--crlf_pos) != CR) {
				this->flag_ |= E_BAD_REQ;
				return false;
			}
			if (this->rdsaved_.begin() + this->rdchecked_ != crlf_pos) {
				req_line.assign(this->rdsaved_.begin() + this->rdchecked_, crlf_pos);
				crlf_pos += 2;
				this->rdchecked_ = crlf_pos - this->rdsaved_.begin();
			} else {
				crlf_pos += 2;
				continue;
			}
			this->req_res_queue_.push(std::pair<req_field_t, res_field_t>());
			if (this->request_line_check(req_line) == false) {
				// spx_log_("syntax error");
				this->flag_ |= E_BAD_REQ;
				// error_res_();
				return false;
			}
			spx_log_("REQ_LINE_PARSER ok");
			break;
		}
		this->rdsaved_.erase(this->rdsaved_.begin(),
							 this->rdsaved_.begin() + this->rdchecked_);
		this->rdchecked_ = 0;
		return false;
	}
	return true;
}

bool
ClientBuffer::header_field_parser() {
	std::string		   header_field_line;
	buffer_t::iterator crlf_pos = this->rdsaved_.begin() + this->rdchecked_;
	int				   idx;

	while (true) {
		crlf_pos = std::find(crlf_pos, this->rdsaved_.end(), LF);
		if (crlf_pos != this->rdsaved_.end()) {
			if (*(--crlf_pos) != CR) {
				this->flag_ |= E_BAD_REQ;
				return false;
			}
			header_field_line.clear();
			if (this->rdsaved_.begin() + this->rdchecked_ != crlf_pos) {
				header_field_line.assign(this->rdsaved_.begin() + this->rdchecked_, crlf_pos);
			}
			crlf_pos += 2;
			this->rdchecked_ = crlf_pos - this->rdsaved_.begin();
			if (header_field_line.size() == 0) {
				// request header parsed.
				break;
			}
			if (spx_http_syntax_header_line(header_field_line) == -1) {
				spx_log_("syntax error");
				this->flag_ |= E_BAD_REQ;
				// error_res();
				return false;
			}
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
				// to do
				this->req_res_queue_.back().first.field_[header_field_line.substr(0, idx)]
					= header_field_line.substr(tmp, header_field_line.size() - tmp);
			}
			continue;
		}
		this->rdsaved_.erase(this->rdsaved_.begin(),
							 this->rdsaved_.begin() + this->rdchecked_);
		this->rdchecked_ = 0;
		return false;
	}
	std::map<std::string, std::string>*			 field = &this->req_res_queue_.back().first.field_;
	std::map<std::string, std::string>::iterator it;

	it = field->find("content-length");
	if (it != field->end()) {
		this->req_res_queue_.back().first.body_size_ = strtoul((it->second).c_str(), NULL, 10);
		spx_log_("content-length", this->req_res_queue_.back().first.body_size_);
		// this->state_ = REQ_SKIP_BODY;
	}
	it = field->find("transfer-encoding");
	if (it != field->end()) {
		for (std::string::iterator tmp = it->second.begin(); tmp != it->second.end(); ++tmp) {
			*tmp = tolower(*tmp);
		}
		if (it->second.find("chunked") != std::string::npos) {
			this->req_res_queue_.back().first.transfer_encoding_ |= TE_CHUNKED;
			spx_log_("transfer-encoding - chunked set", this->req_res_queue_.back().first.transfer_encoding_);
			// this->state_ = REQ_SKIP_BODY;
		}
	}
	// uri_location_t
	return true;
}

// skip content-length or transfer-encoding chunked.
// bool
// ClientBuffer::skip_body(ssize_t cont_len) {
// 	if (cont_len < 0) {
// 		// chunked case
// 	} else {
// 		int n = cont_len - this->req_res_queue_.back().first.body_recieved_;
// 		if (n > this->rdsaved_.size()) {
// 			this->req_res_queue_.back().first.body_recieved_ += this->rdsaved_.size();
// 			this->rdsaved_.clear();
// 			this->rdchecked_ = 0;
// 			// this->flag_ |= RDBUF_CHECKED;
// 		} else {
// 			this->req_res_queue_.back().first.body_recieved_ += n;
// 		}
// 	}
// 	return true;
// }

bool
ClientBuffer::cgi_handler(struct kevent* cur_event, event_list_t& change_list) {
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
		CgiModule cgi(this->req_res_queue_.back().second.uri_resolv_, this->req_res_queue_.back().first.field_,
					  this->req_res_queue_.back().first.uri_loc_);

		cgi.made_env_for_cgi_(this->req_res_queue_.back().first.req_type_);

		char const* script[3];
		script[0] = this->req_res_queue_.back().second.uri_resolv_.cgi_loc_->cgi_path_info.c_str();
		script[1] = this->req_res_queue_.back().second.uri_resolv_.script_filename_.c_str();
		script[2] = NULL;

		// char **envp = (char **)malloc(sizeof(char *) * (cgi.env_for_cgi_.size() + 1));
		// std::cout << cgi.env_for_cgi_.size() << std::endl;
		// for (int i = 0; i < cgi.env_for_cgi_.size(); i++) {
		// 	std::cout << i << "     ";
		// 	std::cout << cgi.env_for_cgi_[i] << std::endl;
		// }
		// execve(script[0], const_cast<char* const*>(script), const_cast<char* const*>(&cgi.env_for_cgi_[0]));

		char* envp[] = {
			// "REQUEST_METHOD=GET",
			// "SERVER_PROTOCOL=HTTP/1.1",
			// "PATH_INFO=./cgi_bin",
			// "GATEWAY_INTERFACE=CGI/1.1",
			"REMOTE_ADDR=127.0.0.1",
			"SERVER_SOFTWARE=SPX/1.0",
			"SERVER_PROTOCOL=HTTP/1.1",
			"REQUEST_METHOD=GET",
			// "REQUEST_URI=/directory/youpi.bla",
			// "SCRIPT_NAME=/directory/youpi.bla",
			"PATH_INFO=/cgi_bin",
			"HTTP_ACCEPT_ENCODING=gzip",
			"HTTP_HOST=localhost:8080",
			"HTTP_USER_AGENT=Go-http-client/1.1",
			0
		};

		cgi.env_for_cgi_.clear();
		// for (int i = 0; i < 12; i++) {
		// 	cgi.env_for_cgi_.push_back(envp[i]);
		// }

		execve(script[0], const_cast<char* const*>(script), envp);
		exit(EXIT_FAILURE);
	}
	// parent
	close(write_to_cgi[0]);
	if (this->req_res_queue_.back().first.req_type_ & (REQ_GET | REQ_HEAD | REQ_DELETE)) {
		close(write_to_cgi[1]);
	} else {
		fcntl(write_to_cgi[1], F_SETFL, O_NONBLOCK);
		add_change_list(change_list, write_to_cgi[1], EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, cur_event->udata);
	}
	fcntl(read_from_cgi[0], F_SETFL, O_NONBLOCK);
	close(read_from_cgi[1]);
	// buf.req_res_queue_.back().first.cgi_in_fd_	= write_to_cgi[1];
	// buf.req_res_queue_.back().first.cgi_out_fd_ = read_from_cgi[0];
	add_change_list(change_list, read_from_cgi[0], EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, cur_event->udata);
	add_change_list(change_list, pid, EVFILT_PROC, EV_ADD, NOTE_EXIT, 0, cur_event->udata);
	return true;
}

bool
ClientBuffer::host_check(std::string& host) {
	if (host.size() != 0 && host.find_first_of(" \t") == std::string::npos) {
		return true;
	}
	return false;
}

bool
ClientBuffer::req_res_controller(std::vector<struct kevent>& change_list,
								 struct kevent*				 cur_event) {
	switch (this->state_) {
	case REQ_LINE_PARSING:
		if (this->request_line_parser() == false) {
			this->state_ = REQ_LINE_PARSING;
			spx_log_("controller-req_line false. read more.", this->state_);
			return false;
		}
		spx_log_("controller-req_line ok");
	case REQ_HEADER_PARSING: {
		if (this->header_field_parser() == false) {
			this->state_ = REQ_HEADER_PARSING;
			spx_log_("controller-header false. read more. state", this->state_);
			return false;
		}
		spx_log_("controller-header ok");

		// write(STDOUT_FILENO, &this->rdsaved_[this->rdchecked_], this->rdsaved_.size() - rdchecked_);
		req_field_t* req = &this->req_res_queue_.back().first;
		// uri loc
		this->req_res_queue_.back().second.flag_ |= WRITE_READY;
		std::string& host = req->field_["host"];

		req->serv_info_ = &this->port_info_->search_server_config_(host);
		if (this->host_check(host) == false) {
			this->flag_ |= E_BAD_REQ;
			return false;
		}
		req->uri_loc_ = req->serv_info_->get_uri_location_t_(req->req_target_,
															 this->req_res_queue_.back().second.uri_resolv_);

		this->req_res_queue_.back().second.uri_resolv_.print_(); // NOTE :: add by yoma.

		// NOTE :: add by space.
		// if cookie exist in request
		SessionStorage								 storage; // this is temporary
		std::map<std::string, std::string>::iterator req_cookie = req->field_.find("Cookie");
		if (req_cookie != req->field_.end()) {
			std::string req_cookie_value = (*req_cookie).second;
			if (!req_cookie_value.empty() && storage.is_key_exsits(req_cookie_value)) {
				this->req_res_queue_.back().first.req_cookie_ = storage.find_value_by_key(req_cookie_value);
			}
		}

		// spx_log_("uri_loc", req->uri_loc_);
		if (req->uri_loc_ == NULL || (req->uri_loc_->accepted_methods_flag & req->req_type_) == false) {
			if (req->uri_loc_ == NULL) {
				this->make_error_response(HTTP_STATUS_NOT_FOUND);
				if (this->req_res_queue_.back().second.body_fd_ != -1) {
					add_change_list(change_list, this->req_res_queue_.back().second.body_fd_, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, this);
				}
				if (this->req_res_queue_.back().first.transfer_encoding_ & TE_CHUNKED) {
					this->req_res_queue_.back().first.content_length_ = -1;
					this->state_									  = REQ_SKIP_BODY_CHUNKED;
				} else {
					if (this->req_res_queue_.back().first.content_length_ != 0) {
						this->state_ = REQ_HOLD;
					} else {
						this->state_ = REQ_LINE_PARSING;
					}
				}
				return false;
			} else {
				this->make_error_response(HTTP_STATUS_METHOD_NOT_ALLOWED);
				if (this->req_res_queue_.back().second.body_fd_ != -1) {
					add_change_list(change_list, this->req_res_queue_.back().second.body_fd_, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, this);
				}
				if (this->req_res_queue_.back().first.transfer_encoding_ & TE_CHUNKED) {
					spx_log_("go to skip_body_chunked");
					this->req_res_queue_.back().first.content_length_ = -1;
					this->state_									  = REQ_SKIP_BODY_CHUNKED;
				} else {
					if (this->req_res_queue_.back().first.content_length_ != 0) {
						this->state_ = REQ_HOLD;
					} else {
						this->state_ = REQ_LINE_PARSING;
					}
				}
				spx_log_("not allowed - buf state", this->state_);
				return false;
			}
			break;
		} else if (this->req_res_queue_.back().second.uri_resolv_.is_cgi_) {
			// cgi case:
			if (this->cgi_handler(cur_event, change_list) == false) {
				this->make_error_response(HTTP_STATUS_INTERNAL_SERVER_ERROR);
				if (this->req_res_queue_.back().second.body_fd_ != -1) {
					add_change_list(change_list, this->req_res_queue_.back().second.body_fd_, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, this);
				}
				this->state_ = REQ_HOLD;
			} else {
				spx_log_("cgi true. forked, open pipes. flag", this->req_res_queue_.front().second.flag_ & WRITE_READY);
				if (this->req_res_queue_.back().first.transfer_encoding_ & TE_CHUNKED) {
					if (this->req_res_queue_.back().first.req_type_ & (REQ_GET | REQ_HEAD | REQ_DELETE)) {
						this->state_ = REQ_SKIP_BODY_CHUNKED;
					} else {
						this->state_ = REQ_BODY_CHUNKED;
					}
				} else {
					if (this->req_res_queue_.back().first.content_length_ != 0) {
						if (this->req_res_queue_.back().first.req_type_ & (REQ_GET | REQ_HEAD | REQ_DELETE)) {
							this->state_ = REQ_SKIP_BODY;
						} else {
							this->state_ = REQ_HOLD;
						}
					} else {
						this->state_ = REQ_LINE_PARSING;
					}
				}
			}
			spx_log_("queue size", this->req_res_queue_.size());
			this->req_res_queue_.front().second.flag_ &= ~(WRITE_READY);
			return false;
		}

		// spx_log_("req_uri set ok");
		switch (this->req_res_queue_.back().first.req_type_) {
		case REQ_GET:
			spx_log_("REQ_GET");
			this->make_response_header();
			// spx_log_("RES_OK");
			if (this->req_res_queue_.back().second.body_fd_ == -1) {
				spx_log_("No file descriptor");
			} else {
				spx_log_("READ event added");
				add_change_list(change_list, this->req_res_queue_.back().second.body_fd_, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, this);
			}
			this->req_res_queue_.back().second.flag_ |= WRITE_READY;
			if (this->req_res_queue_.back().first.transfer_encoding_ & TE_CHUNKED) {
				this->state_ = REQ_SKIP_BODY_CHUNKED;
			} else if (this->req_res_queue_.back().first.body_size_ > 0) {
				this->skip_size_ = this->req_res_queue_.back().first.body_size_;
				this->state_	 = REQ_SKIP_BODY;
			} else {
				this->state_ = REQ_LINE_PARSING;
				if (this->rdsaved_.size() == this->rdchecked_) {
					this->rdsaved_.clear();
					this->rdchecked_ = 0;
				}
			}
			break;
		case REQ_HEAD:
			// same with REQ_GET without body.
			spx_log_("REQ_HEAD");
			this->make_response_header();
			// spx_log_("RES_OK");
			if (this->req_res_queue_.back().second.body_fd_ == -1) {
				spx_log_("No file descriptor");
			} else {
				spx_log_("READ event added");
				add_change_list(change_list, this->req_res_queue_.back().second.body_fd_, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, this);
			}
			this->req_res_queue_.back().second.flag_ |= WRITE_READY;
			if (this->req_res_queue_.back().first.transfer_encoding_ & TE_CHUNKED) {
				this->state_ = REQ_SKIP_BODY_CHUNKED;
			} else if (this->req_res_queue_.back().first.body_size_ > 0) {
				this->skip_size_ = this->req_res_queue_.back().first.body_size_;
				this->state_	 = REQ_SKIP_BODY;
			} else {
				this->state_ = REQ_LINE_PARSING;
				if (this->rdsaved_.size() == this->rdchecked_) {
					this->rdsaved_.clear();
					this->rdchecked_ = 0;
				}
			}
			break;
		case REQ_POST:
			// POST - content len 0. (No body. error case?)
			if ((this->req_res_queue_.back().first.transfer_encoding_ & TE_CHUNKED) == false) {
				if (this->req_res_queue_.back().first.body_size_ == 0) {
					spx_log_("body size == 0");
					this->make_error_response(HTTP_STATUS_NOT_ACCEPTABLE);
					if (this->req_res_queue_.back().second.body_fd_ != -1) {
						add_change_list(change_list, this->req_res_queue_.back().second.body_fd_, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, this);
					}
					this->req_res_queue_.back().second.flag_ |= WRITE_READY;
					return false;
				}
			}

			spx_log_("file open");
			this->req_res_queue_.back().first.file_path_ = this->req_res_queue_.back().second.uri_resolv_.script_filename_;
			this->req_res_queue_.back().first.body_fd_	 = open(
				  this->req_res_queue_.back().second.uri_resolv_.script_filename_.c_str(),
				  O_WRONLY | O_CREAT | O_NONBLOCK | O_APPEND, 0644);
			// error case
			if (this->req_res_queue_.back().first.body_fd_ < 0) {
				// 405 not allowed error with keep-alive connection.
				this->make_error_response(HTTP_STATUS_METHOD_NOT_ALLOWED);
				if (this->req_res_queue_.back().second.body_fd_ != -1) {
					add_change_list(change_list, this->req_res_queue_.back().second.body_fd_, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, this);
				}
				this->req_res_queue_.back().second.flag_ |= WRITE_READY;
				return false;
			}

			spx_log_("control - REQ_POST fd: ", this->req_res_queue_.back().first.body_fd_);
			add_change_list(change_list, this->req_res_queue_.back().first.body_fd_, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, this);
			// this->req_res_queue_.back().first.flag_ |= REQ_FILE_OPEN;
			if (this->req_res_queue_.back().first.transfer_encoding_ & TE_CHUNKED) {
				this->state_ = REQ_BODY_CHUNKED;
			} else {
				this->state_ = REQ_HOLD;
			}
			return false;

		case REQ_PUT:
			if ((this->req_res_queue_.back().first.transfer_encoding_ & TE_CHUNKED) == false) {
				if (this->req_res_queue_.back().first.body_size_ == 0) {
					spx_log_("body size == 0");
					this->make_error_response(HTTP_STATUS_NOT_ACCEPTABLE);
					if (this->req_res_queue_.back().second.body_fd_ != -1) {
						add_change_list(change_list, this->req_res_queue_.back().second.body_fd_, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, this);
					}
					this->req_res_queue_.back().second.flag_ |= WRITE_READY;
					this->state_ = REQ_HOLD;
					return false;
				}
			}

			spx_log_("file open");
			this->req_res_queue_.back().first.file_path_ = this->req_res_queue_.back().second.uri_resolv_.script_filename_;
			this->req_res_queue_.back().first.body_fd_	 = open(
				  this->req_res_queue_.back().second.uri_resolv_.script_filename_.c_str(),
				  O_WRONLY | O_CREAT | O_NONBLOCK | O_TRUNC, 0644);
			// error case
			if (this->req_res_queue_.back().first.body_fd_ < 0) {
				// 405 not allowed error with keep-alive connection.
				this->make_error_response(HTTP_STATUS_METHOD_NOT_ALLOWED);
				if (this->req_res_queue_.back().second.body_fd_ != -1) {
					add_change_list(change_list, this->req_res_queue_.back().second.body_fd_, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, this);
				}
				this->req_res_queue_.back().second.flag_ |= WRITE_READY;
				return false;
			}

			spx_log_("control - REQ_PUT fd: ", this->req_res_queue_.back().first.body_fd_);
			add_change_list(change_list, this->req_res_queue_.back().first.body_fd_, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, this);
			// this->req_res_queue_.back().first.flag_ |= REQ_FILE_OPEN;
			if (this->req_res_queue_.back().first.transfer_encoding_ & TE_CHUNKED) {
				this->req_res_queue_.back().first.content_length_ = -1;
				this->state_									  = REQ_BODY_CHUNKED;
			} else {
				this->state_ = REQ_HOLD;
			}
			return false;
		case REQ_DELETE:
			if (this->req_res_queue_.back().second.header_ready_ == 0) {
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
		buffer_t::iterator crlf_pos = this->rdsaved_.begin() + this->rdchecked_;
		uint32_t		   size		= 0;
		int				   start_line_end;

		while (true) {
			crlf_pos = std::find(crlf_pos, this->rdsaved_.end(), LF);
			if (crlf_pos != this->rdsaved_.end()) {
				start_line_end = crlf_pos - this->rdsaved_.begin() + 1;
				if (spx_chunked_syntax_start_line(*this, size, this->req_res_queue_.back().first.field_) != -1) {
					if (size != 0) {
						if (this->rdsaved_.size() - start_line_end > size) {
							// can parse chunked size.
							this->req_res_queue_.back().first.chunked_body_buffer_.insert(this->req_res_queue_.back().first.chunked_body_buffer_.end(),
																						  crlf_pos + 1, crlf_pos + 1 + size);
							this->rdchecked_ = start_line_end + size;
							this->req_res_queue_.back().first.body_size_ += size;
						} else {
							// try next.
							this->rdsaved_.erase(this->rdsaved_.begin(), this->rdsaved_.begin() + rdchecked_);
							rdchecked_ = 0;
						}
					} else {
						// chunked last
						if (this->rdsaved_[start_line_end] == CR) {
							// no extention
							this->rdchecked_								  = start_line_end + 2;
							this->req_res_queue_.back().first.content_length_ = this->req_res_queue_.back().first.body_size_;
							this->state_									  = REQ_LINE_PARSING;
							break;
						} else {
							// yoma's code..? end check..??
						}
					}
				} else {
					// chunked error
					this->flag_ |= E_BAD_REQ;
					this->make_error_response(HTTP_STATUS_BAD_REQUEST);
					return false;
				}
			} else {
				// chunked start line not exist.
				break;
			}
		}
		if (this->req_res_queue_.back().first.body_size_ > this->req_res_queue_.back().first.body_limit_) {
			// send over limit.
			close(this->req_res_queue_.back().first.body_fd_);
			remove(this->req_res_queue_.back().first.file_path_.c_str());
			add_change_list(change_list, this->req_res_queue_.back().first.body_fd_, EVFILT_WRITE, EV_DISABLE | EV_DELETE, 0, 0, NULL);
			this->state_ = REQ_LINE_PARSING;
			this->make_error_response(HTTP_STATUS_RANGE_NOT_SATISFIABLE);
			if (this->req_res_queue_.back().second.body_fd_ != -1) {
				add_change_list(change_list, this->req_res_queue_.back().second.body_fd_, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, this);
			}
			this->req_res_queue_.back().second.flag_ |= WRITE_READY;
			this->flag_ |= E_BAD_REQ;
		}
		return false;
	}

	case REQ_SKIP_BODY:
		if (this->skip_size_) {
			if (this->skip_size_ >= this->rdsaved_.size() - this->rdchecked_) {
				this->skip_size_ -= this->rdsaved_.size() - this->rdchecked_;
				this->rdsaved_.clear();
				this->rdchecked_ = 0;
			} else {
				this->rdchecked_ += this->skip_size_;
				this->skip_size_ = 0;
			}
		}
		// req_field_t* req = &this->req_res_queue_.back().first;
		// if (req->)
		// 	// std::min(req->req->);
		// 	if (req->body_recieved_ == req->body_limit_) {
		// 		this->state_ = REQ_LINE_PARSING;
		// 	}
		break;

	case REQ_SKIP_BODY_CHUNKED: {
		spx_log_("skip body chunked part!!!!!!");

		// rdsaved_ -> chunked_body_buffer_
		buffer_t::iterator crlf_pos = this->rdsaved_.begin() + this->rdchecked_;
		uint32_t		   size		= 0;
		int				   start_line_end;

		while (true) {
			crlf_pos = std::find(crlf_pos, this->rdsaved_.end(), LF);
			if (crlf_pos != this->rdsaved_.end()) {
				// start_line_end: the next position from LF
				start_line_end = crlf_pos - this->rdsaved_.begin() + 1;
				if (spx_chunked_syntax_start_line(*this, size, this->req_res_queue_.back().first.field_) != -1) {
					if (size != 0) {
						spx_log_("chunked size != 0");
						if (this->rdsaved_.size() - start_line_end > size) {
							// can parse chunked size.
							this->rdchecked_ = start_line_end + size;
							this->req_res_queue_.back().first.body_size_ += size;
						} else {
							// try next.
							this->rdsaved_.erase(this->rdsaved_.begin(), this->rdsaved_.begin() + rdchecked_);
							rdchecked_ = 0;
						}
					} else {
						// chunked last
						if (this->rdsaved_[start_line_end] == CR) {
							spx_log_("chunked last");
							// no extention
							this->rdchecked_ = start_line_end + 2;
							// this->req_res_queue_.back().first.content_length_ = this->req_res_queue_.back().first.body_size_;
							this->state_ = REQ_LINE_PARSING;
							break;
						} else {
							// yoma's code..? end check..??
						}
					}
				} else {
					// chunked error
					this->flag_ |= E_BAD_REQ;
					this->make_error_response(HTTP_STATUS_BAD_REQUEST);
					return false;
				}
			} else {
				// chunked start line not exist.
				break;
			}
		}
		if (this->req_res_queue_.back().first.body_size_ > this->req_res_queue_.back().first.body_limit_) {
			// send over limit.
			close(this->req_res_queue_.back().first.body_fd_);
			this->make_error_response(HTTP_STATUS_RANGE_NOT_SATISFIABLE);
			if (this->req_res_queue_.back().second.body_fd_ != -1) {
				add_change_list(change_list, this->req_res_queue_.back().second.body_fd_, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, this);
			}
			this->req_res_queue_.back().second.flag_ |= WRITE_READY;
			this->flag_ |= E_BAD_REQ;
		}
		return false;
	}

		// case REQ_CGI: {
		// 	req_field_t &req = this->req_res_queue_.back().first;
		// 	res_field_t &res = this->req_res_queue_.back().second;

		// 	if (req.transfer_encoding_ & TE_CHUNKED) {
		// 		// chunked logic
		// 	} else {
		// 		if (req.content_length_ == 0) {

		// 		}
		// 	}
		// 	break;
		// }
	}
	// if (rdsaved_.size() != rdchecked_) {
	// 	this->flag_ &= ~(RDBUF_CHECKED);
	// }
	// this->state_ = REQ_LINE_PARSING;
	return true;
}

// time out case?
void
ClientBuffer::disconnect_client(std::vector<struct kevent>& change_list) {
	// client status, tmp file...? check.
	add_change_list(change_list, this->client_fd_, EVFILT_READ, EV_DISABLE | EV_DELETE, 0, 0,
					NULL);
	add_change_list(change_list, this->client_fd_, EVFILT_WRITE, EV_DISABLE | EV_DELETE, 0,
					0, NULL);
	while (this->req_res_queue_.size()) {
		if (this->req_res_queue_.front().second.body_size_ != this->req_res_queue_.front().second.body_read_) {
			close(this->req_res_queue_.front().second.body_fd_);
		}
		this->req_res_queue_.pop();
	}
	close(this->client_fd_);
}

void
ClientBuffer::write_filter_enable(event_list_t& change_list, struct kevent* cur_event) {
	if (this->req_res_queue_.size() != 0
		&& this->req_res_queue_.front().second.flag_ & WRITE_READY) {
		add_change_list(change_list, cur_event->ident, EVFILT_WRITE,
						EV_ENABLE, 0, 0, this);
	}
}

// read only request header & body message
void
ClientBuffer::read_to_client_buffer(std::vector<struct kevent>& change_list,
									struct kevent*				cur_event) {
	int n_read = read(this->client_fd_, this->rdbuf_, BUFFER_SIZE);
	if (n_read < 0) {
		// TODO: error handle
		return;
	}
	this->rdsaved_.insert(this->rdsaved_.end(), this->rdbuf_, this->rdbuf_ + n_read);
	write(STDOUT_FILENO, &this->rdsaved_[this->rdchecked_], this->rdsaved_.size() - rdchecked_);
	spx_log_("read_to_client: ", n_read);
	if (this->state_ != REQ_HOLD) {
		this->req_res_controller(change_list, cur_event);
		spx_log_("req_res_controller check finished. buf stat", this->state_);
	};
	spx_log_("enable write", this->req_res_queue_.front().second.flag_ & WRITE_READY);
	write_filter_enable(change_list, cur_event);
}

bool
ClientBuffer::cgi_header_parser() {
	std::string		   header_field_line;
	res_field_t&	   res		= this->req_res_queue_.back().second;
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
			// 	this->flag_ |= E_BAD_REQ;
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
		cgi_buf.erase(cgi_buf.begin(),
					  cgi_buf.begin() + res.cgi_checked_);
		res.cgi_checked_ = 0;
		return false;
	}
	return true;
}

bool
ClientBuffer::cgi_controller() {
	res_field_t& res = this->req_res_queue_.back().second;

	// switch (state) {
	// case CGI_HEADER: {
	if (this->cgi_header_parser() == false) {
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
		// this->make_cgi_response_header();
	} else {
		// chunked case.
		res.cgi_state_ = CGI_BODY_CHUNKED;
		res.cgi_size_  = -1;
	}
	this->make_cgi_response_header();
	// this->req_res_queue_.back().second.flag_ |= WRITE_READY;
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
ClientBuffer::read_to_cgi_buffer(event_list_t& change_list, struct kevent* cur_event) {
	int n_read = read(cur_event->ident, this->rdbuf_, BUFFER_SIZE);
	spx_log_("read to cgi buffer. n_read", n_read);
	write(STDOUT_FILENO, this->rdbuf_, n_read);
	if (n_read < 0) {
		// TODO: error handle
		this->disconnect_client(change_list);
		return;
	}
	this->req_res_queue_.back().second.cgi_buffer_.insert(
		this->req_res_queue_.back().second.cgi_buffer_.end(), this->rdbuf_, this->rdbuf_ + n_read);

	// if (cgi_controller(this->req_res_queue_.back().second.cgi_state_, change_list) == false) {
	// 	spx_log_("cgi controller false");
	// 	return;
	// }
	// spx_log_("cgi controller ok");
	return;
}

void
ClientBuffer::read_to_res_buffer(event_list_t& change_list, struct kevent* cur_event) {
	int n_read = read(cur_event->ident, this->rdbuf_, BUFFER_SIZE);
	if (n_read < 0) {
		// TODO: error handle
		return;
	}
	spx_log_("read_to_res_buffer");
	this->req_res_queue_.back().second.res_buffer_.insert(
		this->req_res_queue_.back().second.res_buffer_.end(), this->rdbuf_, this->rdbuf_ + n_read);
	this->req_res_queue_.back().second.body_read_ += n_read;
	// if all content read, close fd.
	if (this->req_res_queue_.back().second.body_read_ == this->req_res_queue_.back().second.body_size_) {
		close(cur_event->ident);
		add_change_list(change_list, cur_event->ident, EVFILT_READ, EV_DISABLE | EV_DELETE, 0, 0, NULL);
	}
}

void
ClientBuffer::make_error_response(http_status error_code) {
	res_field_t& res = req_res_queue_.front().second;
	req_field_t& req = req_res_queue_.front().first;

	res.status_		 = http_status_str(error_code);
	res.status_code_ = error_code;

	// res.headers_.push_back(header("Server", "SpaceX/12.26"));

	if (error_code == HTTP_STATUS_BAD_REQUEST)
		res.headers_.push_back(header(CONNECTION, CONNECTION_CLOSE));
	else
		res.headers_.push_back(header(CONNECTION, KEEP_ALIVE));

	std::string page_path = req.serv_info_->get_error_page_path_(error_code);
	spx_log_("page_path = ", page_path);
	int error_req_fd = open(page_path.c_str(), O_RDONLY);
	spx_log_("error_req_fd : ", error_req_fd);
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
	res.body_fd_ = error_req_fd;

	res.setContentType(page_path);
	int	  req_method	 = req.req_type_;
	off_t content_length = res.setContentLength(error_req_fd);

	if (req_method == REQ_GET)
		res.buf_size_ += content_length;
	res.write_to_response_buffer(res.make_to_string());
}

// this is main logic to make response
void
ClientBuffer::make_response_header() {
	const req_field_t& req
		= req_res_queue_.back().first;
	res_field_t& res
		= req_res_queue_.back().second;

	const std::string& uri		  = res.uri_resolv_.script_filename_;
	int				   req_fd	  = -1;
	int				   req_method = req.req_type_;
	std::string		   content;

	// Set Date Header
	res.setDate();
	switch (req_method) {
	case REQ_HEAD:
	case REQ_GET:
		if (uri[uri.size() - 1] != '/') {
			spx_log_("uri.cstr()", uri.c_str());
			req_fd = res.file_open(uri.c_str());
		} else {
			spx_log_("folder skip");
			// make_error_response(HTTP_STATUS_NOT_FOUND);
			return;
		}
		spx_log_("uri_locations", req.uri_loc_);
		spx_log_("req_fd", req_fd);
		if (req_fd == 0) {
			spx_log_("folder skip");
			make_error_response(HTTP_STATUS_FORBIDDEN);
			return;
		} else if (req_fd == -1 && (req.uri_loc_ == NULL || req.uri_loc_->autoindex_flag == Kautoindex_off)) {
			make_error_response(HTTP_STATUS_NOT_FOUND);
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
				make_error_response(HTTP_STATUS_FORBIDDEN);
				return;
			}
		}
		if (req_fd != -1) {
			spx_log_("res_header");
			res.setContentType(uri);
			off_t content_length = res.setContentLength(req_fd);
			if (req_method == REQ_GET) {
				res.body_fd_ = req_fd;
				res.buf_size_ += content_length;
			} else {
				res.body_fd_ = -1;
				close(req_fd);
			}
			// res.headers_.push_back(header("Accept-Ranges", "bytes"));
		} else {
			// autoindex case?
			res.headers_.push_back(header(CONTENT_TYPE, MIME_TYPE_HTML));
		}
		break;
	case REQ_POST:
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
ClientBuffer::make_cgi_response_header() {
	const req_field_t& req		  = req_res_queue_.back().first;
	res_field_t&	   res		  = req_res_queue_.back().second;
	int				   req_method = req.req_type_;

	// Set Date Header
	res.setDate();
	spx_log_("make_cgi_res_header");
	std::map<std::string, std::string>::iterator it;

	it = res.cgi_field_.find("status");
	if (it != res.cgi_field_.end()) {
		res.status_code_ = strtol(it->second.c_str(), NULL, 10);
	} else {
		res.status_code_ = 200;
	}
	// res.headers_.push_back(header("Set-Cookie", "SESSIONID=123456;"));
	// settting response_header size  + content-length size to res_field
	res.headers_.push_back(header(CONNECTION, KEEP_ALIVE));
	std::stringstream ss;
	ss << res.cgi_buffer_.size() - res.cgi_checked_;
	res.headers_.push_back(header(CONTENT_LENGTH, ss.str().c_str()));
	res.write_to_response_buffer(res.make_to_string());
}

void
ClientBuffer::make_redirect_response() {
	const req_field_t& req
		= req_res_queue_.front().first;
	res_field_t& res
		= req_res_queue_.front().second;

	res.setDate();
	if (req.uri_loc_ == NULL || req.uri_loc_->redirect.empty()) {
		make_error_response(HTTP_STATUS_NOT_FOUND);
		return;
	};
	res.status_code_ = HTTP_STATUS_MOVED_PERMANENTLY;
	res.status_		 = http_status_str(HTTP_STATUS_MOVED_PERMANENTLY);
	res.headers_.push_back(header("Location", req.uri_loc_->redirect));
	res.write_to_response_buffer(res.make_to_string());
}

bool
ClientBuffer::write_for_upload(event_list_t& change_list, struct kevent* cur_event) {
	int			 n_write;
	req_field_t& req = this->req_res_queue_.back().first;
	size_t		 buf_len;

	if (req_res_queue_.back().first.transfer_encoding_ & TE_CHUNKED) {
		// chunked logic
		buf_len = req.chunked_body_buffer_.size() - req.chunked_checked_;
		if (WRITE_BUFFER_MAX <= buf_len) {
			spx_log_("write_for_upload: MAX_BUF ");
			n_write = write(req.body_fd_, &req.chunked_body_buffer_[req.chunked_checked_], WRITE_BUFFER_MAX);
			req.body_read_ += n_write;
		} else {
			n_write = write(req.body_fd_, &req.chunked_body_buffer_[req.chunked_checked_], buf_len);
			req.body_read_ += n_write;
			if (req.chunked_checked_ == req.chunked_body_buffer_.size()) {
				req.chunked_body_buffer_.clear();
				req.chunked_checked_ = 0;
			}
		}
		if (req.body_read_ == req.content_length_) {
			this->req_res_queue_.back().first.flag_ |= READ_BODY_END;
			add_change_list(change_list, cur_event->ident, EVFILT_WRITE, EV_DISABLE | EV_DELETE, 0, 0, NULL);
			this->state_ = REQ_HOLD;
		}
	} else {
		buf_len	   = this->rdsaved_.size() - this->rdchecked_;
		size_t len = req.body_size_ - req.body_read_;
		if (WRITE_BUFFER_MAX <= std::min(buf_len, len)) {
			spx_log_("write_for_upload: MAX_BUF ");
			n_write = write(req.body_fd_, &this->rdsaved_[this->rdchecked_], WRITE_BUFFER_MAX);
		} else if (buf_len <= len) {
			spx_log_("write_for_upload: buf_len: ", buf_len);
			n_write = write(req.body_fd_, &this->rdsaved_[this->rdchecked_], buf_len);
		} else {
			spx_log_("write_for_upload: len: ", len);
			n_write = write(req.body_fd_, &this->rdsaved_[this->rdchecked_], len);
		}
		spx_log_("body_fd: ", req.body_fd_);
		spx_log_("write len: ", n_write);
		req.body_read_ += n_write;
		this->rdchecked_ += n_write;
		if (this->rdsaved_.size() == this->rdchecked_) {
			this->rdsaved_.clear();
			this->rdchecked_ = 0;
		}
	}
	if (this->req_res_queue_.back().first.body_read_ == this->req_res_queue_.back().first.body_size_) {
		close(cur_event->ident);
		add_change_list(change_list, cur_event->ident, EVFILT_WRITE, EV_DISABLE | EV_DELETE, 0, 0, NULL);
		this->req_res_queue_.back().first.flag_ |= READ_BODY_END;
		this->state_ = REQ_HOLD;
	}
	return true;
}

bool
ClientBuffer::write_to_cgi(struct kevent* cur_event, std::vector<struct kevent>& change_list) {
	req_field_t& req = this->req_res_queue_.back().first;
	res_field_t& res = this->req_res_queue_.back().second;
	size_t		 buf_len;
	int			 n_write;

	if (req.transfer_encoding_ & TE_CHUNKED) {
		// unchunked to req.chunked_body_buffer
		buf_len = req.chunked_body_buffer_.size() - req.chunked_checked_;
		if (WRITE_BUFFER_MAX <= buf_len) {
			spx_log_("write_to_cgi: MAX_BUF ");
			n_write = write(cur_event->ident, &req.chunked_body_buffer_[req.chunked_checked_], WRITE_BUFFER_MAX);
			req.body_read_ += n_write;
		} else {
			n_write = write(cur_event->ident, &req.chunked_body_buffer_[req.chunked_checked_], buf_len);
			req.body_read_ += n_write;
			if (req.chunked_checked_ == req.chunked_body_buffer_.size()) {
				req.chunked_body_buffer_.clear();
				req.chunked_checked_ = 0;
			}
		}
		if (req.body_read_ == req.content_length_) {
			this->req_res_queue_.back().first.flag_ |= READ_BODY_END;
			add_change_list(change_list, cur_event->ident, EVFILT_WRITE, EV_DISABLE | EV_DELETE, 0, 0, NULL);
			this->state_ = REQ_HOLD;
		}
	} else {
		buf_len	   = this->rdsaved_.size() - this->rdchecked_;
		size_t len = req.body_size_ - req.body_read_;
		if (WRITE_BUFFER_MAX <= std::min(buf_len, len)) {
			spx_log_("write_to_cgi: MAX_BUF ");
			n_write = write(cur_event->ident, &this->rdsaved_[this->rdchecked_], WRITE_BUFFER_MAX);
		} else if (buf_len <= len) {
			spx_log_("write_to_cgi: buf_len: ", buf_len);
			n_write = write(cur_event->ident, &this->rdsaved_[this->rdchecked_], buf_len);
		} else {
			spx_log_("write_to_cgi: len: ", len);
			n_write = write(cur_event->ident, &this->rdsaved_[this->rdchecked_], len);
		}
		// spx_log_("body_fd: ", req.body_fd_);
		// spx_log_("write len: ", n_write);
		req.body_read_ += n_write;
		this->rdchecked_ += n_write;
		if (this->rdsaved_.size() == this->rdchecked_) {
			this->rdsaved_.clear();
			this->rdchecked_ = 0;
		}
		if (this->req_res_queue_.back().first.body_read_ == this->req_res_queue_.back().first.body_size_) {
			this->req_res_queue_.back().first.flag_ |= READ_BODY_END;
			close(cur_event->ident);
			add_change_list(change_list, cur_event->ident, EVFILT_WRITE, EV_DISABLE | EV_DELETE, 0, 0, NULL);
			this->state_ = REQ_HOLD;
		}
	}
	return true;
}

bool
ClientBuffer::write_response(std::vector<struct kevent>& change_list) {
	res_field_t* res = &this->req_res_queue_.front().second;
	// no chunked case.
	int n_write = write(this->client_fd_, &res->res_buffer_[res->sent_pos_],
						std::min((size_t)WRITE_BUFFER_MAX,
								 res->res_buffer_.size() - res->sent_pos_));
	if (n_write) {
		write(STDOUT_FILENO, &res->res_buffer_[res->sent_pos_],
			  std::min((size_t)WRITE_BUFFER_MAX, res->res_buffer_.size() - res->sent_pos_));
	}
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
		this->req_res_queue_.pop();
		// this->flag_ &= ~(RDBUF_CHECKED);
	}
	return true;
}

void
ResField::write_to_response_buffer(const std::string& content) {
	res_buffer_.insert(res_buffer_.end(), content.begin(), content.end());
	buf_size_ += content.size();
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
