#include "spx_client_buffer.hpp"
#include "spx_kqueue_module.hpp"

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
			header_field_line.assign(this->rdsaved_.begin() + this->rdchecked_, crlf_pos);
			crlf_pos += 2;
			this->rdchecked_ = crlf_pos - this->rdsaved_.begin();
			if (header_field_line.size() == 0) {
				// request header parsed.
				break;
			}
			if (spx_http_syntax_header_line(header_field_line) == -1) {
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
		this->state_ = REQ_SKIP_BODY;
	}
	it = field->find("transfer-encoding");
	if (it != field->end()) {
		for (std::string::iterator tmp = it->second.begin(); tmp != it->second.end(); ++tmp) {
			*tmp = tolower(*tmp);
		}
		if (it->second.find("chunked") != std::string::npos) {
			spx_log_("transfer-encoding: cunked");
			this->req_res_queue_.back().first.transfer_encoding_ |= TE_CHUNKED;
			this->state_ = REQ_SKIP_BODY;
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
			spx_log_("controller-req_line false. read more.");
			return false;
		}
		spx_log_("controller-req_line ok");
	case REQ_HEADER_PARSING: {
		if (this->header_field_parser() == false) {
			spx_log_("controller-header false. read more.");
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

		// spx_log_("uri_loc", req->uri_loc_);
		if (req->uri_loc_ == NULL || (req->uri_loc_->accepted_methods_flag & req->req_type_) == false) {
			if (req->uri_loc_ == NULL) {
				this->make_error_response(HTTP_STATUS_NOT_FOUND);
				if (this->req_res_queue_.back().second.body_fd_ != -1) {
					add_change_list(change_list, this->req_res_queue_.back().second.body_fd_, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, this);
				}
				this->state_ = REQ_HOLD;
			} else {
				this->make_error_response(HTTP_STATUS_METHOD_NOT_ALLOWED);
				if (this->req_res_queue_.back().second.body_fd_ != -1) {
					add_change_list(change_list, this->req_res_queue_.back().second.body_fd_, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, this);
				}
				this->state_ = REQ_HOLD;
			}
			break;
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
			this->req_res_queue_.back().first.body_fd_ = open(
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
			this->state_ = REQ_HOLD;
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
			this->req_res_queue_.back().first.body_fd_ = open(
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

			spx_log_("control - REQ_POST fd: ", this->req_res_queue_.back().first.body_fd_);
			add_change_list(change_list, this->req_res_queue_.back().first.body_fd_, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, this);
			// this->req_res_queue_.back().first.flag_ |= REQ_FILE_OPEN;
			this->state_ = REQ_HOLD;
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
	case REQ_CGI:
		break;
	}
	// if (rdsaved_.size() != rdchecked_) {
	// 	this->flag_ &= ~(RDBUF_CHECKED);
	// }
	this->state_ = REQ_LINE_PARSING;
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
		spx_log_("req_res_controller check finished.");
	};
	spx_log_("enable write");
	write_filter_enable(change_list, cur_event);
}

void
ClientBuffer::read_to_cgi_buffer(event_list_t& change_list, struct kevent* cur_event) {
	int n_read = read(cur_event->ident, this->rdbuf_, BUFFER_SIZE);
	if (n_read < 0) {
		// TODO: error handle
		return;
	}
	this->req_res_queue_.back().first.cgi_buffer_.insert(
		this->req_res_queue_.back().first.cgi_buffer_.end(), this->rdbuf_, this->rdbuf_ + n_read);
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
	case (REQ_HEAD):
		// make_redirect_response();
		break;
	case (REQ_GET):
		if (uri[uri.size() - 1] != '/') {
			req_fd = res.file_open(uri.c_str());
			spx_log_(uri.c_str());
		} else {
			spx_log_("folder skip");
		}
		res.body_fd_ = req_fd;
		spx_log_("uri_locations", req.uri_loc_);
		if (req_fd == 0) {
			make_error_response(HTTP_STATUS_FORBIDDEN);
			return;
		} else if (req_fd == -1 && req.uri_loc_ == NULL) {
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
			// } else {
			// make_error_response(HTTP_STATUS_NOT_FOUND);
			// return;
			// }
		}
		if (req_fd != -1) {
			spx_log_("res_header");
			res.setContentType(uri);
			off_t content_length = res.setContentLength(req_fd);
			if (req_method == REQ_GET)
				res.buf_size_ += content_length;
			// res.headers_.push_back(header("Accept-Ranges", "bytes"));

		} else {
			// autoindex case?
			res.headers_.push_back(header(CONTENT_TYPE, MIME_TYPE_HTML));
		}
		res.headers_.push_back(header(CONNECTION, KEEP_ALIVE));
		break;
	case REQ_POST:
		res.headers_.push_back(header(CONNECTION, KEEP_ALIVE));
		res.headers_.push_back(header(CONTENT_LENGTH, "0"));
		break;
	}

	// res.headers_.push_back(header("Set-Cookie", "SESSIONID=123456;"));

	// settting response_header size  + content-length size to res_field
	res.write_to_response_buffer(res.make_to_string());
	if (!content.empty()) {
		res.write_to_response_buffer(content);
	}
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
	int	   n_write;
	size_t buf_len = this->rdsaved_.size() - this->rdchecked_;
	size_t len	   = this->req_res_queue_.back().first.body_size_ - this->req_res_queue_.back().first.body_read_;

	if (req_res_queue_.back().first.transfer_encoding_ & TE_CHUNKED) {
		// chunked logic
		// buffer_t::iterator it;
		// while (true) {
		// 	it = std::find(this->rdsaved_.begin() + this->rdchecked_, this->rdsaved_.end(), '\r');
		// 	if (it != this->rdsaved_.end()) {

		// 	}
		// }
	} else {
		if (WRITE_BUFFER_MAX <= std::min(buf_len, len)) {
			spx_log_("write_for_upload: MAX_BUF ");
			n_write = write(this->req_res_queue_.back().first.body_fd_, &this->rdsaved_[this->rdchecked_], WRITE_BUFFER_MAX);
		} else if (buf_len <= len) {
			spx_log_("write_for_upload: buf_len: ", buf_len);
			n_write = write(this->req_res_queue_.back().first.body_fd_, &this->rdsaved_[this->rdchecked_], buf_len);
		} else {
			spx_log_("write_for_upload: len: ", len);
			n_write = write(this->req_res_queue_.back().first.body_fd_, &this->rdsaved_[this->rdchecked_], len);
		}
		spx_log_("body_fd: ", this->req_res_queue_.back().first.body_fd_);
		spx_log_("write len: ", n_write);
		this->req_res_queue_.back().first.body_read_ += n_write;
		this->rdchecked_ += n_write;
		if (this->rdsaved_.size() == this->rdchecked_) {
			this->rdsaved_.clear();
			this->rdchecked_ = 0;
		}
	}
	if (this->req_res_queue_.back().first.body_read_ == this->req_res_queue_.back().first.body_size_) {
		this->req_res_queue_.back().first.flag_ |= READ_BODY_END;
		add_change_list(change_list, cur_event->ident, EVFILT_WRITE, EV_DISABLE | EV_DELETE, 0, 0, NULL);
		this->state_ = REQ_LINE_PARSING;
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
	// write(this->client_fd_, "asdf", 4);
	// write(STDOUT_FILENO, &res->res_buffer_[res->sent_pos_],
	// 	  std::min((size_t)WRITE_BUFFER_MAX,
	// 			   res->res_buffer_.size() - res->sent_pos_));
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
