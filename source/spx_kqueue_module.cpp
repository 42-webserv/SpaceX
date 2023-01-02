#include "spx_kqueue_module.hpp"
#include "spx_client_buffer.hpp"

void
add_change_list(std::vector<struct kevent>& change_list,
				uintptr_t ident, int64_t filter, uint16_t flags,
				uint32_t fflags, intptr_t data, void* udata) {
	struct kevent tmp_event;
	EV_SET(&tmp_event, ident, filter, flags, fflags, data, udata);
	change_list.push_back(tmp_event);
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
read_event_handler(std::vector<port_info_t>& port_info, struct kevent* cur_event,
				   std::vector<struct kevent>& change_list) {
	// debug
	static long cgi_read = 0;

	// server port will be updated.
	if (cur_event->ident < port_info.size()) {
		if (create_client_event(cur_event->ident, cur_event, change_list, port_info[cur_event->ident]) == false) {
			// TODO: error ???
		}
		return;
	}
	client_buf_t* buf = static_cast<client_buf_t*>(cur_event->udata);
	if (cur_event->ident == buf->client_fd_) {
		spx_log_("read_event_handler - client_fd");
		buf->read_to_client_buffer(change_list, cur_event);
	} else if (buf->req_res_queue_.back().second.uri_resolv_.is_cgi_) {
		// read from cgi output.
		cgi_read++;
		spx_log_("read_event_handler - cgi", cgi_read);
		buf->read_to_cgi_buffer(change_list, cur_event);
	} else {
		spx_log_("read_event_handler - server_file");
		// server file read case for res_body.
		buf->read_to_res_buffer(change_list, cur_event);
		// if ( )
	}
}

void
kevent_error_handler(std::vector<port_info_t>& port_info, struct kevent* cur_event,
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
		if (buf != NULL && buf->client_fd_ == cur_event->ident) {
			buf->disconnect_client(change_list);
			delete buf;
		} else if (buf != NULL) {
			spx_log_("cgi_fd_error?");
		}
	}
}

void
write_event_handler(std::vector<port_info_t>& port_info, struct kevent* cur_event,
					std::vector<struct kevent>& change_list) {
	client_buf_t* buf = (client_buf_t*)cur_event->udata;
	res_field_t*  res = &buf->req_res_queue_.front().second;

	// debug
	static long cgi_write = 0;

	if (cur_event->ident == buf->client_fd_) {
		spx_log_("write event handler - buf state", buf->state_);
		if (buf->req_res_queue_.front().second.res_buffer_.size() || buf->req_res_queue_.front().second.cgi_buffer_.size() - buf->req_res_queue_.front().second.cgi_checked_) {
			if (buf->write_response(change_list) == false) {
				return;
			}
			if (buf->req_res_queue_.size() == 0 || (res->flag_ & WRITE_READY) == false) {
				spx_log_("write_event_handler - disable write");
				add_change_list(change_list, cur_event->ident, EVFILT_WRITE, EV_DISABLE, 0, 0, buf);
				// if (buf->req_res_queue_.size() == 0) {
				// 	buf->state_ = REQ_LINE_PARSING;
				// }
			}
			if (buf->state_ != REQ_HOLD) {
				spx_log_("write_event_handler - not REQ_HOLD");
				spx_log_("write event handler - buf state", buf->state_);
				buf->req_res_controller(change_list, cur_event);
			}
		} else {
			spx_log_("empty!!!!!");
			add_change_list(change_list, cur_event->ident, EVFILT_WRITE, EV_DISABLE, 0, 0, buf);
		}
	} else {
		if (cur_event->ident == buf->req_res_queue_.back().first.body_fd_) {
			// file upload case
			// if (buf->req_res_queue_.back().second.uri_resolv_.is_cgi_) {
			// 	spx_log_("write_for_upload - chunked");
			// 	// chunked logic

			// } else {
			spx_log_("write_for_upload");
			if (buf->write_for_upload(change_list, cur_event) == false) {
				spx_log_("too large file to upload");
				buf->make_error_response(HTTP_STATUS_NOT_ACCEPTABLE);
				if (buf->req_res_queue_.back().second.body_fd_ != -1) {
					add_change_list(change_list, buf->req_res_queue_.back().second.body_fd_, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, buf);
				}
				return;
			}
			// }
			if (buf->req_res_queue_.back().first.flag_ & READ_BODY_END) {
				spx_log_("write_for_upload - uploaded");
				close(cur_event->ident);
				add_change_list(change_list, cur_event->ident, EVFILT_WRITE, EV_DISABLE | EV_DELETE, 0, 0, NULL);
				spx_log_("queue size", buf->req_res_queue_.size());
				buf->make_response_header();
				buf->state_ = REQ_LINE_PARSING;
				if (buf->rdsaved_.size() == buf->rdchecked_) {
					buf->rdsaved_.clear();
					buf->rdchecked_ = 0;
				}
				add_change_list(change_list, buf->client_fd_, EVFILT_WRITE, EV_ENABLE, 0, 0, buf);
				// write(STDOUT_FILENO, &buf->req_res_queue_.front().second.res_buffer_[0], buf->req_res_queue_.front().second.res_buffer_.size());
				// buf->state_ = REQ_LINE_PARSING;
			}
		} else {
			cgi_write++;
			spx_log_("write_to_cgi", cgi_write);
			// cgi logic
			buf->write_to_cgi(cur_event, change_list);

			// if (buf->req_res_queue_.back().first.transfer_encoding_ & TE_CHUNKED) {
			// 	// write from req->chunked body buffer
			// 	cgi_
			// }
		}
	}
}

void
proc_event_handler(struct kevent* cur_event, event_list_t& change_list) {
	// need to check exit status??
	client_buf_t* buf = (client_buf_t*)cur_event->udata;
	int			  status;
	waitpid(cur_event->ident, &status, 0);
	std::cout << "status: " << status << std::endl;

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
kqueue_module(std::vector<port_info_t>& port_info) {
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
			// spx_log_("event_len:", event_len);
			// spx_log_("cur->ident:", cur_event->ident);
			// spx_log_("cur->flags:", cur_event->flags);
			cur_event = &event_list[i];
			if (cur_event->flags & (EV_ERROR | EV_EOF)) {
				if (cur_event->flags & EV_ERROR) {
					kevent_error_handler(port_info, cur_event, change_list);
				} else {
					// eof close fd.
					client_buf_t* buf = static_cast<client_buf_t*>(cur_event->udata);
					if (cur_event->ident == buf->client_fd_) {
						std::cerr << "client socket eof" << std::endl;
						buf->disconnect_client(change_list);
						delete buf;
					} else {
						if (cur_event->filter == EVFILT_PROC) {
							// proc
							spx_log_("event_proc");
							proc_event_handler(cur_event, change_list);
						} else {
							// cgi case. server file does not return EV_EOF.
							if (cur_event->filter == EVFILT_READ) {
								int n_read = read(cur_event->ident, buf->rdbuf_, BUFFER_SIZE);
								if (n_read < 0) {
									// TODO: error handle
									// buf->disconnect_client(change_list);
									return;
								} else if (n_read == 0) {
									spx_log_("cgi read close");
									buf->req_res_queue_.back().second.cgi_checked_ = 0;
									buf->cgi_controller();
									add_change_list(change_list, buf->client_fd_, EVFILT_WRITE, EV_ENABLE, 0, 0, buf);
								} else {
									buf->req_res_queue_.back().second.cgi_buffer_.insert(
										buf->req_res_queue_.back().second.cgi_buffer_.end(), buf->rdbuf_, buf->rdbuf_ + n_read);
									break;
								}
							} else {
								spx_log_("cgi write close");
							}
							close(cur_event->ident);
							add_change_list(change_list, cur_event->ident, cur_event->filter, EV_DISABLE | EV_DELETE, 0, 0, NULL);
						}
					}
				}
				continue;
			}
			switch (cur_event->filter) {
			case EVFILT_READ:
				spx_log_("event_read");
				read_event_handler(port_info, cur_event, change_list);
				break;
			case EVFILT_WRITE:
				// if (cur_event->flags & EV_DISABLE) {
				// 	spx_log_("event_write disabled!!");
				// 	break;
				// }
				spx_log_("event_write");
				write_event_handler(port_info, cur_event, change_list);
				break;
			case EVFILT_PROC:
				// cgi end
				spx_log_("event_cgi");
				proc_event_handler(cur_event, change_list);
				break;
			case EVFILT_TIMER:
				spx_log_("event_timer");
				timer_event_handler(cur_event, change_list);
				// TODO: timer
				break;
			}
		}
	}
	return;
}
