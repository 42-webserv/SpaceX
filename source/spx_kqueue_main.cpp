#include "spx_client_buffer.hpp"

void
add_change_list(std::vector<struct kevent>& change_list,
				uintptr_t ident, int64_t filter, uint16_t flags,
				uint32_t fflags, intptr_t data, void* udata) {
	struct kevent tmp_event;
	EV_SET(&tmp_event, ident, filter, flags, fflags, data, udata);
	change_list.push_back(tmp_event);
}

ClientBuffer::ClientBuffer()
	: req_res_queue_()
	, rdsaved_()
	, timeout_()
	, client_fd_()
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
		// TODO : need to parse the query string into map
		return true;
	}
	return false;
}

bool
ClientBuffer::request_line_parser() {
	std::string		   req_line;
	buffer_t::iterator crlf_pos = this->rdsaved_.begin();

	while (true) {
		crlf_pos = std::find(crlf_pos, this->rdsaved_.end(), '\n');
		if (crlf_pos != this->rdsaved_.end()) {
			if (*(--crlf_pos) != '\r') {
				this->flag_ |= E_BAD_REQ;
				return false;
			}
			req_line.assign(this->rdsaved_.begin() + this->rdchecked_,
							crlf_pos);
			this->rdchecked_ = crlf_pos - this->rdsaved_.begin() + 2;
			if (req_line.size() == 0) {
				crlf_pos += 2;
				// request line is empty. get next crlf.
				continue;
			}
			this->req_res_queue_.push(
				std::pair<req_field_t, res_field_t>());
			if (this->request_line_check(req_line) == false) {
				this->flag_ |= E_BAD_REQ;
				// error_res_();
				return false;
			}
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
		crlf_pos = std::find(crlf_pos, this->rdsaved_.end(), '\n');
		if (crlf_pos != this->rdsaved_.end()) {
			if (*(--crlf_pos) != '\r') {
				this->flag_ |= E_BAD_REQ;
				return false;
			}
			header_field_line.assign(
				this->rdsaved_.begin() + this->rdchecked_, crlf_pos);
			this->rdchecked_ = crlf_pos - this->rdsaved_.begin() + 2;
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
				size_t tmp = idx + 1;
				while (syntax_(ows_, header_field_line[tmp])) {
					++tmp;
				}
				// to do
				for (std::string::iterator it = header_field_line.begin();
					 it != header_field_line.end(); ++it) {
					if (isupper(*it)) {
						*it = tolower(*it);
					}
				}
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
	const std::map<std::string, std::string>*		   field = &this->req_res_queue_.back().first.field_;
	std::map<std::string, std::string>::const_iterator it;
	it = field->find("content-length");
	if (it != field->end()) {
		this->req_res_queue_.back().first.content_length_ = strtoul((it->second).c_str(), NULL, 10);
		this->state_									  = REQ_BODY;
	}
	it = field->find("transfer-encoding");
	if (it != field->end() && it->second.find("chunked") != std::string::npos) {
		this->req_res_queue_.back().first.transfer_encoding_ |= TE_CHUNKED;
		this->state_ = REQ_BODY;
	}
	// uri_location_t
	return true;
}

// skip content-length or transfer-encoding chunked.
bool
ClientBuffer::skip_body(ssize_t cont_len) {
	if (cont_len < 0) {
		// chunked case
	} else {
		int n = cont_len - this->req_res_queue_.back().first.body_recieved_;
		if (n > this->rdsaved_.size()) {
			this->req_res_queue_.back().first.body_recieved_ += this->rdsaved_.size();
			this->rdsaved_.clear();
			this->flag_ |= RDBUF_CHECKED;
		} else {
			this->req_res_queue_.back().first.body_recieved_ += n;
		}
	}
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
			// need to read more from the client socket. or error?
			this->flag_ |= RDBUF_CHECKED;
			return false;
		}
	case REQ_HEADER_PARSING: {
		if (this->header_field_parser() == false) {
			// need to read more from the client socket. or error?
			this->flag_ |= RDBUF_CHECKED;
			return false;
		}
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

		this->state_ = REQ_BODY;

		if (req->uri_loc_ && (req->uri_loc_->accepted_methods_flag & req->req_type_) == false) {
			// Not Allowed / Not Supported error.
			// make_response_header(*this);
			this->state_ = REQ_LINE_PARSING;
			break;
		}

		switch (this->req_res_queue_.back().first.req_type_) {
		case REQ_GET:
			// if (this->req_res_queue_.back().second.res_buffer_.size() == 0) {
			// }
			// set_get_res();
			// server file descriptor will be added to kqueue
			// if it has a body, status will be changed to RES_BODY
			// plus, client fd EV_READ will be disabled.
			if (this->req_res_queue_.back().first.content_length_ > this->req_res_queue_.back().first.body_limit_) {
				// TODO: error response
				return false;
			}

			if (this->req_res_queue_.back().first.content_length_ > 0) {
				this->skip_body(this->req_res_queue_.back().first.content_length_);
			} else if (this->req_res_queue_.back().first.transfer_encoding_ & TE_CHUNKED) {
				this->skip_body(-1);
			}
			break;
		case REQ_HEAD:
			// same with REQ_GET without body.
			// if (this->req_res_queue_.back().second.header_ready_ == 0) {
			// }
			// set_head_res();
			if (this->req_res_queue_.back().first.content_length_ > this->req_res_queue_.back().first.body_limit_) {
				// TODO: error response
				return false;
			}

			if (this->req_res_queue_.back().first.content_length_ > 0) {
				this->skip_body(this->req_res_queue_.back().first.content_length_);
			} else if (this->req_res_queue_.back().first.transfer_encoding_ & TE_CHUNKED) {
				this->skip_body(-1);
			}
			break;
		case REQ_POST:
			// set_post_res();
			if ((this->req_res_queue_.back().first.flag_ & REQ_FILE_OPEN) == false) {
				uintptr_t fd = open(
					this->req_res_queue_.back().first.file_path_.c_str(),
					O_RDONLY | O_CREAT | O_NONBLOCK | O_APPEND, 0644);
				if (fd < 0) {
					// open error
					// 405 not allowed error with keep-alive connection.
				}
				// add_change_list(change_list, cur_event->ident,
				// 				EVFILT_READ, EV_DISABLE, 0, 0,
				// 				this);
				add_change_list(change_list, fd, EVFILT_READ,
								EV_ADD | EV_ENABLE, 0, 0, this);
				this->req_res_queue_.back().first.flag_ |= REQ_FILE_OPEN;
				return false;
			}
			break;
		case REQ_PUT:
			if ((this->req_res_queue_.back().first.flag_ & REQ_FILE_OPEN) == false) {
				uintptr_t fd = open(
					this->req_res_queue_.back().first.file_path_.c_str(),
					O_RDONLY | O_CREAT | O_NONBLOCK | O_TRUNC, 0644);
				if (fd < 0) {
					// open error
					// 405 not allowed error with keep-alive connection.
				}
				// add_change_list(change_list, cur_event->ident,
				// 				EVFILT_READ, EV_DISABLE, 0, 0,
				// 				this);
				add_change_list(change_list, fd, EVFILT_READ,
								EV_ADD | EV_ENABLE, 0, 0, this);
				this->req_res_queue_.back().first.flag_ |= REQ_FILE_OPEN;
				return false;
			}
			break;
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
	case REQ_BODY:
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
	this->state_ = REQ_LINE_PARSING;
	return true;
}

// time out case?
void
ClientBuffer::disconnect_client(std::vector<struct kevent>& change_list) {
	// client status, tmp file...? check.
	add_change_list(change_list, this->client_fd_, EVFILT_READ, EV_DELETE, 0, 0,
					NULL);
	add_change_list(change_list, this->client_fd_, EVFILT_WRITE, EV_DELETE, 0,
					0, NULL);
	close(this->client_fd_);
}

bool
create_client_event(uintptr_t serv_sd, struct kevent* cur_event,
					std::vector<struct kevent>& change_list, port_info_t& port_info) {
	uintptr_t client_fd = accept(serv_sd, NULL, NULL);

	if (client_fd == -1) {
		std::cerr << strerror(errno) << std::endl;
		return false;
	} else {
		// std::cout << "accept new client: " << client_fd << std::endl;
		fcntl(client_fd, F_SETFL, O_NONBLOCK);
		client_buf_t* new_buf = new client_buf_t;
		new_buf->client_fd_	  = client_fd;
		new_buf->port_info_	  = &port_info;
		add_change_list(change_list, client_fd, EVFILT_READ,
						EV_ADD | EV_ENABLE, 0, 0, new_buf);
		add_change_list(change_list, client_fd, EVFILT_WRITE,
						EV_ADD | EV_DISABLE, 0, 0, new_buf);
		// add_change_list(change_list, client_fd, EVFILT_TIMER,
		// 				EV_ADD, NOTE_SECONDS, 60, new_buf);
		return true;
	}
}

void
ClientBuffer::write_filter_enable(event_list_t& change_list, struct kevent* cur_event) {
	if (this->req_res_queue_.size() != 0
		&& (this->req_res_queue_.front().second.flag_ & WRITE_READY) == true) {
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
	if (this->req_res_queue_.size() == 0
		|| (this->req_res_queue_.front().second.flag_ & WRITE_READY) == false) {
		this->req_res_controller(change_list, cur_event);
	};
	write_filter_enable(change_list, cur_event);
}

void
ClientBuffer::read_to_cgi_buffer(event_list_t& change_list, struct kevent* cur_event) {
	int n_read = read(this->client_fd_, this->rdbuf_, BUFFER_SIZE);
	if (n_read < 0) {
		// TODO: error handle
		return;
	}
	this->req_res_queue_.back().first.cgi_buffer_.insert(
		this->req_res_queue_.back().first.cgi_buffer_.end(), this->rdbuf_, this->rdbuf_ + n_read);
}

void
ClientBuffer::read_to_res_buffer(event_list_t& change_list, struct kevent* cur_event) {
	int n_read = read(this->client_fd_, this->rdbuf_, BUFFER_SIZE);
	if (n_read < 0) {
		// TODO: error handle
		return;
	}
	this->req_res_queue_.back().second.res_buffer_.insert(
		this->req_res_queue_.back().second.res_buffer_.end(), this->rdbuf_, this->rdbuf_ + n_read);
}

void
read_event_handler(std::vector<port_info_t>& port_info, struct kevent* cur_event,
				   std::vector<struct kevent>& change_list) {
	// server port will be updated.
	if (cur_event->ident < port_info.size()) {
		if (create_client_event(cur_event->ident, cur_event, change_list, port_info[cur_event->ident]) == false) {
			// TODO: error ???
		}
	}

	client_buf_t* buf = static_cast<client_buf_t*>(cur_event->udata);
	if (cur_event->ident == buf->client_fd_) {
		buf->read_to_client_buffer(change_list, cur_event);
	} else if (buf->state_ == REQ_CGI) {
		// read from cgi output.
		buf->read_to_cgi_buffer(change_list, cur_event);
	} else {
		// server file read case for res_body.
		buf->read_to_res_buffer(change_list, cur_event);
	}
}

void
kevnet_error_handler(std::vector<port_info_t>& port_info, struct kevent* cur_event,
					 std::vector<struct kevent>& change_list) {
	if (cur_event->ident < port_info.size()) {
		for (int i = 0; i < port_info.size(); i++) {
			if (port_info[i].listen_sd == i) {
				close(i);
			}
		}
		error_exit_msg("kevent()");
	} else {
		client_buf_t* buf = static_cast<client_buf_t*>(cur_event->udata);
		std::cerr << "client socket error" << std::endl;
		buf->disconnect_client(change_list);
		delete buf;
	}
}

void
write_event_handler(std::vector<port_info_t>& port_info, struct kevent* cur_event,
					std::vector<struct kevent>& change_list) {
	client_buf_t* buf = (client_buf_t*)cur_event->udata;
	res_field_t*  res = &buf->req_res_queue_.front().second;

	if (cur_event->ident == buf->client_fd_) {
		// write to client
		// if (res->flag_ & RES_CGI) {
		// 	// buf->write_cgi()
		// } else {
		// }
		buf->write_response(cur_event->ident, change_list);
		if ((buf->flag_ & RDBUF_CHECKED) == false) {
			while (buf->req_res_controller(change_list, cur_event) == true) {
				;
			}
		}
		if (buf->req_res_queue_.size() == 0 || (buf->req_res_queue_.front().second.flag_ & WRITE_READY) == false) {
			add_change_list(change_list, cur_event->ident, EVFILT_WRITE,
							EV_DISABLE, 0, 0, buf);
		}
	} else {
		// write to serv file descriptor or cgi
		// if (buf->req_res_queue_.front)
	}
}

bool
ClientBuffer::write_response(uintptr_t					 fd,
							 std::vector<struct kevent>& change_list) {
	res_field_t* res = &this->req_res_queue_.front().second;
	// no chunked case.
	int n_write = write(fd, &res->res_buffer_[res->sent_pos_],
						std::min((size_t)WRITE_BUFFER_MAX,
								 res->res_buffer_.size() - res->sent_pos_));
	if (n_write < 0) {
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
	if (res->buf_size_ == 0) {
		this->req_res_queue_.pop();
		this->flag_ &= ~(RDBUF_CHECKED);
	}
	return true;
}

void
proc_event_handler(struct kevent* cur_event, event_list_t& change_list) {
	// need to check exit status??
	client_buf_t* buf = (client_buf_t*)cur_event->udata;
	waitpid(cur_event->ident, NULL, 0);
	// close(buf->req_res_queue_.front().first.cgi_in_fd_);
	// close(buf->req_res_queue_.front().first.cgi_out_fd_);
	add_change_list(change_list, cur_event->ident, EVFILT_PROC, EV_DELETE, 0, 0, NULL);
}

void
timer_event_handler(struct kevent* cur_event, event_list_t& change_list) {
	// timeout(60s)
	close(cur_event->ident);
	// TODO: check file write to delete tmp file and check file descriptors to close.
	add_change_list(change_list, cur_event->ident, EVFILT_TIMER, EV_DELETE, 0, 0, NULL);
	add_change_list(change_list, cur_event->ident, EVFILT_READ, EV_DELETE, 0, 0, NULL);
	add_change_list(change_list, cur_event->ident, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
}

void
cgi_handler(struct kevent* cur_event, event_list_t& change_list) {
	client_buf_t& buf = (client_buf_t&)cur_event->udata;
	int			  write_to_cgi[2];
	int			  read_from_cgi[2];
	pid_t		  pid;

	if (pipe(write_to_cgi) == -1) {
		// pipe error
		std::cerr << "pipe error" << std::endl;
		return;
	}
	if (pipe(read_from_cgi) == -1) {
		// pipe error
		close(write_to_cgi[0]);
		close(write_to_cgi[1]);
		std::cerr << "pipe error" << std::endl;
		return;
	}
	pid = fork();
	if (pid < 0) {
		// fork error
		close(write_to_cgi[0]);
		close(write_to_cgi[1]);
		close(read_from_cgi[0]);
		close(read_from_cgi[1]);
		return;
	}
	if (pid == 0) {
		// child. run cgi
		dup2(write_to_cgi[0], STDIN_FILENO);
		close(write_to_cgi[1]);
		close(read_from_cgi[0]);
		dup2(read_from_cgi[1], STDOUT_FILENO);
		// set_cgi_envp()
		// execve();
		exit(EXIT_FAILURE);
	}
	// parent
	close(write_to_cgi[0]);
	fcntl(write_to_cgi[1], F_SETFL, O_NONBLOCK);
	fcntl(read_from_cgi[0], F_SETFL, O_NONBLOCK);
	close(read_from_cgi[1]);
	buf.req_res_queue_.back().first.cgi_in_fd_	= write_to_cgi[1];
	buf.req_res_queue_.back().first.cgi_out_fd_ = read_from_cgi[0];
	add_change_list(change_list, write_to_cgi[1], EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, cur_event->udata);
	add_change_list(change_list, read_from_cgi[0], EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, cur_event->udata);
	add_change_list(change_list, pid, EVFILT_PROC, EV_ADD, NOTE_EXIT, 0, cur_event->udata);
}

void
kqueue_main(std::vector<port_info_t>& port_info) {
	std::vector<struct kevent>		  change_list;
	std::map<uintptr_t, client_buf_t> clients;
	struct kevent					  event_list[8];
	int								  kq;

	kq = kqueue();
	if (kq == -1) {
		for (int i = 0; i < port_info.size(); i++) {
			if (port_info[i].listen_sd == i) {
				close(i);
			}
		}
		error_exit_msg("kqueue()");
	}

	for (int i = 0; i < port_info.size(); i++) {
		if (port_info[i].listen_sd == i) {
			add_change_list(change_list, i, EVFILT_READ,
							EV_ADD | EV_ENABLE, 0, 0, NULL);
		}
	}

	int			   event_len;
	struct kevent* cur_event;
	// int            l = 0;
	while (true) {
		event_len = kevent(kq, change_list.begin().base(), change_list.size(),
						   event_list, MAX_EVENT_LOOP, NULL);
		if (event_len == -1) {
			for (int i = 0; i < port_info.size(); i++) {
				if (port_info[i].listen_sd == i) {
					close(i);
				}
			}
			error_exit_msg("kevent()");
		}
		change_list.clear();

		// std::cout << "current loop: " << l++ << std::endl;

		for (int i = 0; i < event_len; ++i) {
			cur_event = &event_list[i];
			if (cur_event->flags & (EV_ERROR | EV_EOF)) {
				if (cur_event->flags & EV_ERROR) {
					kevnet_error_handler(port_info, cur_event, change_list);
				} else {
					// eof close fd.
				}
			}
			switch (cur_event->filter) {
			case EVFILT_READ:
				read_event_handler(port_info, cur_event, change_list);
				break;
			case EVFILT_WRITE:
				write_event_handler(port_info, cur_event, change_list);
				break;
			case EVFILT_PROC:
				// cgi end
				proc_event_handler(cur_event, change_list);
				break;
			case EVFILT_TIMER:
				timer_event_handler(cur_event, change_list);
				// TODO: timer
				break;
			}
		}
	}
	return;
}
