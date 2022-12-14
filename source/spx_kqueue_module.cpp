#include "spx_kqueue_module.hpp"
#include "spx_client.hpp"

void
add_change_list(event_list_t& change_list,
				uintptr_t ident, int64_t filter, uint16_t flags,
				uint32_t fflags, intptr_t data, void* udata) {
	struct kevent tmp_event;
	EV_SET(&tmp_event, ident, filter, flags, fflags, data, udata);
	change_list.push_back(tmp_event);
}

bool
create_client_event(uintptr_t serv_sd, struct kevent* cur_event,
					event_list_t& change_list, port_info_t& port_info) {

	uintptr_t client_fd = accept(serv_sd, NULL, NULL);

	if (client_fd == -1) {
		std::cerr << strerror(errno) << std::endl;
		return false;
	} else {
		// std::cout << "accept new client: " << client_fd << std::endl;
		fcntl(client_fd, F_SETFL, O_NONBLOCK);
		client_t* new_cl   = new client_t(&change_list);
		new_cl->_client_fd = client_fd;
		new_cl->_port_info = &port_info;
		add_change_list(change_list, client_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, new_cl);
		add_change_list(change_list, client_fd, EVFILT_WRITE, EV_ADD | EV_DISABLE, 0, 0, new_cl);
		// add_change_list(change_list, client_fd, EVFILT_TIMER,
		// 				EV_ADD, NOTE_SECONDS, 60, new_cl);
		return true;
	}
}

void
read_event_handler(std::vector<port_info_t>& port_info, struct kevent* cur_event,
				   event_list_t& change_list) {

	if (cur_event->ident < port_info.size()) {
		if (create_client_event(cur_event->ident, cur_event, change_list, port_info[cur_event->ident]) == false) {
			// TODO: error ???
		}
		return;
	}
	client_t* cl = static_cast<client_t*>(cur_event->udata);
	if (cur_event->ident == cl->_client_fd) {
		spx_log_("read_event_handler - client_fd");
		cl->read_to_client_buffer_(cur_event);
	} else if (cl->_req._uri_resolv.is_cgi_) {
		// read from cgi output.
		spx_log_("read_event_handler - cgi");
		cl->read_to_cgi_buffer_(cur_event);
	} else {
		spx_log_("read_event_handler - server_file");
		// server file read case for res_body.
		cl->read_to_res_buffer_(cur_event);
	}
}

void
kevent_error_handler(std::vector<port_info_t>& port_info, struct kevent* cur_event,
					 event_list_t& change_list) {
	if (cur_event->ident < port_info.size()) {
		for (int i = 0; i < port_info.size(); i++) {
			if (port_info[i].listen_sd == i) {
				close(i);
			}
		}
		error_exit_msg("kevent()");
	} else {
		client_t* cl = static_cast<client_t*>(cur_event->udata);
		if (cl != NULL && cl->_client_fd == cur_event->ident) {
			cl->disconnect_client_();
			delete cl;
		} else if (cl != NULL) {
			spx_log_("cgi_fd_error?");
		}
	}
}

void
write_event_handler(std::vector<port_info_t>& port_info, struct kevent* cur_event,
					event_list_t& change_list) {
	client_t& cl = *static_cast<client_t*>(cur_event->udata);

	if (cur_event->ident == cl._client_fd) {
		spx_log_("write event handler - cl state", cl._state);
		if (cl._res._res_header.size() || cl._res._header_sent) {
			if (cl.write_response_() == false) {
				return;
			}
			if (cl._res._header_sent) {
				if (cl._req._req_mthd & (REQ_POST | REQ_PUT | REQ_DELETE)) {
					spx_log_("write_event_handler - disable write");
					add_change_list(change_list, cur_event->ident, EVFILT_WRITE, EV_DISABLE, 0, 0, &cl);
				} else if (cl._res._write_ready == false) {
					add_change_list(change_list, cur_event->ident, EVFILT_WRITE, EV_DISABLE, 0, 0, &cl);
				}
			}
			if (cl._state != REQ_HOLD) {
				spx_log_("write_event_handler - not REQ_HOLD");
				spx_log_("write event handler - cl state", cl._state);
				cl.req_res_controller_(cur_event);
			}
		} else {
			spx_log_("empty!!!!!");
			add_change_list(change_list, cur_event->ident, EVFILT_WRITE, EV_DISABLE, 0, 0, &cl);
		}
	} else {
		if (cur_event->ident == cl._req._body_fd) {
			spx_log_("write_for_upload");
			if (cl.write_for_upload_(cur_event) == false) {
				spx_log_("too large file to upload");
				cl.error_response_keep_alive_(HTTP_STATUS_NOT_ACCEPTABLE);
				return;
			}
			// }
			// if (cl._req._flag & READ_BODY_END) {
			// 	spx_log_("write_for_upload - uploaded");
			// 	close(cur_event->ident);
			// 	add_change_list(change_list, cur_event->ident, EVFILT_WRITE, EV_DISABLE | EV_DELETE, 0, 0, NULL);
			// 	spx_log_("queue size", cl.req_res_queue_.size());
			// 	cl.make_response_header();
			// 	cl.state_ = REQ_LINE_PARSING;
			// 	if (cl.rdsaved_.size() == cl.rdchecked_) {
			// 		cl.rdsaved_.clear();
			// 		cl.rdchecked_ = 0;
			// 	}
			// 	add_change_list(change_list, cl._client_fd, EVFILT_WRITE, EV_ENABLE, 0, 0, cl);
			// 	// write(STDOUT_FILENO, &_res.res_buffer_[0], _res.res_buffer_.size());
			// 	// cl.state_ = REQ_LINE_PARSING;
			// }
		} else {
			spx_log_("write_to_cgi");
			// cgi logic
			cl.write_to_cgi_(cur_event);
		}
	}
}

void
proc_event_handler(struct kevent* cur_event, event_list_t& change_list) {
	// need to check exit status??
	client_t* cl = (client_t*)cur_event->udata;
	int		  status;
	waitpid(cur_event->ident, &status, 0);
	spx_log_("status: ", status);

	// close(cl->req_res_queue_.front().first.cgi_in_fd_);
	// close(cl->req_res_queue_.front().first.cgi_out_fd_);
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
	event_list_t  change_list;
	struct kevent event_list[MAX_EVENT_LIST];
	int			  kq;

	kq = kqueue();
	if (kq == -1) {
		for (int i = 0; i < port_info.size(); i++) {
			if (port_info[i].listen_sd == i) {
				close(i);
			}
		}
		error_exit_msg("kqueue() error");
	}

	for (int i = 0; i < port_info.size(); i++) {
		if (port_info[i].listen_sd == i) {
			add_change_list(change_list, i, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
		}
	}

	int			   event_len;
	struct kevent* cur_event;

	while (true) {
		event_len = kevent(kq, &change_list.front(), change_list.size(),
						   event_list, MAX_EVENT_LIST, NULL);
		if (event_len == -1) {
			for (int i = 0; i < port_info.size(); i++) {
				if (port_info[i].listen_sd == i) {
					close(i);
				}
			}
			error_exit_msg("kevent() error");
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
					client_t* cl = static_cast<client_t*>(cur_event->udata);
					if (cl == NULL) {
						continue;
					}
					if (cur_event->ident == cl->_client_fd) {
						main_log_("client socket eof", COLOR_PURPLE);
						cl->disconnect_client_();
						delete cl;
					} else {
						if (cur_event->filter == EVFILT_PROC) {
							spx_log_("event_proc");
							proc_event_handler(cur_event, change_list);
						} else {
							// cgi case. server file does not return EV_EOF.
							if (cur_event->filter == EVFILT_READ) {
								int n_read = cl->_cgi._from_cgi.read_(cur_event->ident);
								if (n_read < 0) {
									cl->disconnect_client_();
									delete cl;
									return;
								}
								spx_log_("cgi read close");
								cl->_cgi.cgi_controller_(*cl);
								// add_change_list(change_list, cl->_client_fd, EVFILT_WRITE, EV_ENABLE, 0, 0, cl);
								break;
							} else {
								spx_log_("cgi write close");
								close(cur_event->ident);
								add_change_list(change_list, cur_event->ident, cur_event->filter, EV_DISABLE | EV_DELETE, 0, 0, NULL);
							}
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
