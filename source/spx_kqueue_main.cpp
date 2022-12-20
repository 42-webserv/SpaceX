#include "spx_client_buffer.hpp"

void
error_exit(std::string err, int (*func)(int), int fd) {
	std::cerr << strerror(errno) << std::endl;
	if (func != NULL) {
		func(fd);
	}
	exit(EXIT_FAILURE);
}

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
	, client_fd()
	, rdchecked_(0)
	, flag_(0)
	, state_(REQ_LINE_PARSING)
	, rdbuf_() {
}

ClientBuffer::~ClientBuffer() { }

bool
ClientBuffer::request_line_check(std::string& req_line) {
	return true;
}

bool
ClientBuffer::request_line_parser() {
	std::string		   req_line;
	t_buffer::iterator crlf_pos = this->rdsaved_.begin();

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
				std::pair<t_req_field, t_res_field>());
			if (this->request_line_check(req_line) == false) {
				this->flag_ |= E_BAD_REQ;
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
ClientBuffer::header_valid_check(std::string& key_val, size_t col_pos) {
	// check logic
	return true;
}

bool
ClientBuffer::header_field_parser() {
	std::string		   header_field_line;
	t_buffer::iterator crlf_pos = this->rdsaved_.begin() + this->rdchecked_;
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
			idx = header_field_line.find(':');
			if (idx != std::string::npos) {
				size_t tmp = idx + 1;
				// to do
				if (header_valid_check(header_field_line, idx)) {
					while (header_field_line[tmp] == ' ' || header_field_line[tmp] == '\t') {
						++tmp;
					}
					this->req_res_queue_.back()
						.first.field_[header_field_line.substr(0, idx)]
						= header_field_line.substr(
							tmp, header_field_line.size() - tmp);
				} else {
					this->flag_ |= E_BAD_REQ;
					return false;
				}
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
	return true;
}

bool
ClientBuffer::write_res_body(uintptr_t					 fd,
							 std::vector<struct kevent>& change_list) {
	t_res_field* res = &this->req_res_queue_.front().second;
	// no chunked case.
	int n_write = write(fd, &res->body_buffer_[res->sent_pos_],
						std::min((size_t)WRITE_BUFFER_MAX,
								 res->body_buffer_.size() - res->sent_pos_));
	res->sent_pos_ += n_write;
	res->content_length_ -= n_write;
	if (res->body_buffer_.size() == res->sent_pos_) {
		res->body_buffer_.clear();
		res->sent_pos_ = 0;
	}
	if (res->content_length_ == 0) {
		this->state_ &= ~(RES_BODY);
	}
}

bool
ClientBuffer::write_res_header(uintptr_t				   fd,
							   std::vector<struct kevent>& change_list) {
	t_res_field* res = &this->req_res_queue_.front().second;
	int			 n	 = write(fd, &res->res_header_.c_str()[res->sent_pos_],
							 res->res_header_.size() - res->sent_pos_);
	if (n < 0) {
		// client fd error. maybe disconnected.
		// error handle code
		return n;
	}
	if (n != res->res_header_.size() - res->sent_pos_) {
		// partial write
		res->sent_pos_ += n;
	} else {
		// header sent
		if (res->body_flag_) {
			// int fd = open( res->file_path_.c_str(), O_RDONLY, 0644 );
			// if ( fd < 0 ) {
			// 	// file open error. incorrect direction ??
			// }
			// add_change_list( change_list, buf->client_fd, EVFILT_READ,
			// 					   EV_DISABLE, 0, 0, buf );
			// add_change_list( change_list, fd, EVFILT_READ,
			// 					   EV_ADD | EV_ENABLE, 0, 0, buf );
			this->flag_ |= RES_BODY;
			this->flag_ &= ~SOCK_WRITE;
		} else {
			this->req_res_queue_.pop();
		}
	}
	return n;
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
		} else {
			this->req_res_queue_.back().first.body_recieved_ += n;
		}
	}
}

bool
ClientBuffer::req_res_controller(std::vector<struct kevent>& change_list,
								 struct kevent*				 cur_event) {
	switch (this->state_) {
	case REQ_LINE_PARSING:
		if (this->request_line_parser() == false) {
			this->flag_ |= RDBUF_CHECKED;
			break;
		}
	case REQ_HEADER_PARSING:
		if (this->header_field_parser() == false) {
			// need to read more from the client socket.
			this->flag_ |= RDBUF_CHECKED;
			break;
		}
		switch (this->req_res_queue_.back().first.req_type_) {
		case REQ_GET:
			if (this->req_res_queue_.back().second.header_ready_ == 0) {
				// set_get_res();
			}
			// server file descriptor will be added to kqueue
			// if it has a body, status will be changed to RES_BODY
			// plus, client fd EV_READ will be disabled.

			if (this->req_res_queue_.back().first.content_length_ > 0) {
				this->skip_body(this->req_res_queue_.back().first.content_length_);
			} else if (this->req_res_queue_.back().first.transfer_encoding_ & TE_CHUNKED) {
				this->skip_body(-1);
			}
			return false;
		case REQ_HEAD:
			// same with REQ_GET without body.
			if (this->req_res_queue_.back().second.header_ready_ == 0) {
				// set_head_res();
			}
			if (this->req_res_queue_.back().first.content_length_ > 0) {
				this->skip_body(this->req_res_queue_.back().first.content_length_);
			} else if (this->req_res_queue_.back().first.transfer_encoding_ & TE_CHUNKED) {
				this->skip_body(-1);
			}
			break;
		case REQ_POST:
			// set_post_res();
			if (this->req_res_queue_.back().first.body_flag_ & FILE_OPEN == false) {
				uintptr_t fd = open(
					this->req_res_queue_.back().first.file_path_.c_str(),
					O_RDONLY | O_CREAT | O_NONBLOCK | O_APPEND, 0644);
				if (fd < 0) {
					// open error
				}
				add_change_list(change_list, cur_event->ident,
								EVFILT_READ, EV_DISABLE, 0, 0,
								this);
				add_change_list(change_list, fd, EVFILT_READ,
								EV_ADD | EV_ENABLE, 0, 0, this);
				this->req_res_queue_.back().first.body_flag_ |= FILE_OPEN;
			}
			break;
		case REQ_PUT:
			if (this->req_res_queue_.back().first.body_flag_ & FILE_OPEN == false) {
				uintptr_t fd = open(
					this->req_res_queue_.back().first.file_path_.c_str(),
					O_RDONLY | O_CREAT | O_NONBLOCK | O_TRUNC, 0644);
				if (fd < 0) {
					// open error
				}
				add_change_list(change_list, cur_event->ident,
								EVFILT_READ, EV_DISABLE, 0, 0,
								this);
				add_change_list(change_list, fd, EVFILT_READ,
								EV_ADD | EV_ENABLE, 0, 0, this);
				this->req_res_queue_.back().first.body_flag_ |= FILE_OPEN;
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
	case REQ_BODY:
		// TODO: Body length check
		// if body length is larger than client body limit, disconnect??
		// if transfer encoding is chunked, keep checking it for the limit.
		if (this->req_res_queue_.back().first.body_recieved_ < this->req_res_queue_.back().first.body_limit_) {
		}
	case RES_BODY:
		// file end check.
		if (lseek(cur_event->ident, 0, SEEK_CUR)
			== this->req_res_queue_.back().second.content_length_ - 1) {
			add_change_list(change_list, this->client_fd, EVFILT_READ,
							EV_ENABLE, 0, 0, this);
			close(cur_event->ident);
			add_change_list(change_list, cur_event->ident,
							EVFILT_READ, EV_DELETE, 0, 0, this);
			// need to check _rdsaved buffer before read.
			this->state_ = REQ_LINE_PARSING;
		}
		// keep reading form server file descriptor.
		break;
	}
	return true;
}

// time out case?
void
ClientBuffer::disconnect_client(std::vector<struct kevent>& change_list) {
	// client status, tmp file...? check.
	add_change_list(change_list, this->client_fd, EVFILT_READ, EV_DELETE, 0, 0,
					NULL);
	add_change_list(change_list, this->client_fd, EVFILT_WRITE, EV_DELETE, 0,
					0, NULL);
	close(client_fd);
}

uintptr_t
server_init() {
	int				   serv_sd;
	struct sockaddr_in serv_addr;

	serv_sd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (serv_sd == -1) {
		error_exit("socket()", NULL, 0);
	}

	int opt = 1;
	if (setsockopt(serv_sd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
		error_exit("setsockopt()", close, serv_sd);
	}

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family	  = AF_INET;
	serv_addr.sin_port		  = htons(SERV_PORT);
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(serv_sd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
		error_exit("bind", close, serv_sd);
	}

	if (listen(serv_sd, SERV_SOCK_BACKLOG) == -1) {
		error_exit("bind", close, serv_sd);
	}
}

bool
create_client_event(uintptr_t serv_sd, struct kevent* cur_event,
					std::vector<struct kevent>& change_list, port_info_t& serv_info) {
	uintptr_t client_fd;
	if ((client_fd = accept(serv_sd, NULL, NULL)) == -1) {
		std::cerr << strerror(errno) << std::endl;
		return false;
	} else {
		std::cout << "accept new client: " << client_fd << std::endl;
		fcntl(client_fd, F_SETFL, O_NONBLOCK);
		client_buf_t* new_buf = new client_buf_t();
		new_buf->client_fd	  = client_fd;
		new_buf->port_info	  = &serv_info;
		// port info will be added.
		add_change_list(change_list, client_fd, EVFILT_READ,
						EV_ADD | EV_ENABLE, 0, 0, new_buf);
		add_change_list(change_list, client_fd, EVFILT_WRITE,
						EV_ADD | EV_DISABLE, 0, 0, new_buf);
		return true;
	}
}

void
ClientBuffer::client_buffer_read(struct kevent*				 cur_event,
								 std::vector<struct kevent>& change_list) {
	int n_read = read(this->client_fd, this->rdbuf_, BUFFER_SIZE);
	if (n_read < 0) {
		// TODO: error handle
		return;
	}
	this->flag_ &= ~READ_READY;
	if (this->state_ != RES_BODY) {
		this->rdsaved_.insert(this->rdsaved_.end(), this->rdbuf_,
							  this->rdbuf_ + n_read);
	} else {
		this->req_res_queue_.back().second.body_buffer_.insert(
			this->req_res_queue_.back().second.body_buffer_.end(),
			this->rdbuf_, this->rdbuf_ + n_read);
	}
	while (this->flag_ & READ_READY == false) {
		this->req_res_controller(change_list, cur_event);
	}
	if (this->req_res_queue_.size() != 0
		&& this->req_res_queue_.front().second.body_flag_ & WRITE_READY == true) {
		add_change_list(change_list, cur_event->ident, EVFILT_WRITE,
						EV_ENABLE, 0, 0, this);
	}
}

void
read_event_handler(std::vector<port_info_t>& serv_info, struct kevent* cur_event,
				   std::vector<struct kevent>& change_list) {
	// server port will be updated.
	if (cur_event->ident < serv_info.size()) {
		if (create_client_event(cur_event->ident, cur_event, change_list, serv_info[cur_event->ident]) == false) {
			// TODO: error ???
		}
	} else {
		client_buf_t* buf = static_cast<client_buf_t*>(cur_event->udata);

		buf->client_buffer_read(cur_event, change_list);
	}
}

void
kevnet_error_handler(std::vector<port_info_t>& serv_info, struct kevent* cur_event,
					 std::vector<struct kevent>& change_list) {
	if (cur_event->ident < serv_info.size()) {
		for (int i = 0; i < serv_info.size(); i++) {
			if (serv_info[i].listen_sd == i) {
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
write_event_handler(std::vector<port_info_t>& serv_info, struct kevent* cur_event,
					std::vector<struct kevent>& change_list) {
	client_buf_t* buf = (client_buf_t*)cur_event->udata;

	if (buf->flag_ & RES_BODY) {
		buf->write_res_body(cur_event->ident, change_list);
	} else {
		if (buf->write_res_header(cur_event->ident, change_list) == false) {
			// error
		}
	}
	if (buf->req_res_queue_.size() == 0 || buf->req_res_queue_.front().second.body_flag_ & WRITE_READY == false) {
		add_change_list(change_list, cur_event->ident, EVFILT_WRITE,
						EV_DISABLE, 0, 0, buf);
	}
}

void
kqueue_main(std::vector<port_info_t>& serv_info) {
	std::map<uintptr_t, client_buf_t> clients;
	std::vector<struct kevent>		  change_list;
	struct kevent					  event_list[8];
	int								  kq;

	kq = kqueue();
	if (kq == -1) {
		for (int i = 0; i < serv_info.size(); i++) {
			if (serv_info[i].listen_sd == i) {
				close(i);
			}
		}
		error_exit_msg("kqueue()");
	}

	for (int i = 0; i < serv_info.size(); i++) {
		if (serv_info[i].listen_sd == i) {
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
			for (int i = 0; i < serv_info.size(); i++) {
				if (serv_info[i].listen_sd == i) {
					close(i);
				}
			}
			error_exit_msg("kevent()");
		}
		change_list.clear();

		// std::cout << "current loop: " << l++ << std::endl;

		for (int i = 0; i < event_len; ++i) {
			cur_event = &event_list[i++];
			if (cur_event->flags & EV_ERROR) {
				kevnet_error_handler(serv_info, cur_event, change_list);
			} else if (cur_event->filter == EVFILT_READ) {
				read_event_handler(serv_info, cur_event, change_list);
			} else if (cur_event->filter == EVFILT_WRITE) {
				write_event_handler(serv_info, cur_event, change_list);
			} else if (cur_event->filter == EVFILT_PROC) {
				// TODO: waitpid
			}
		}
	}
	return;
}
