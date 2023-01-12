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
					event_list_t& change_list, port_info_t& port_info,
					rdbuf_t* rdbuf, session_storage_t* storage) {

	uintptr_t client_fd = accept(serv_sd, NULL, NULL);

	if (client_fd == -1) {
		error_str("accept error");
		return false;
	} else {
		// std::cout << "accept new client: " << client_fd << std::endl;
		fcntl(client_fd, F_SETFL, O_NONBLOCK);

		struct linger opt = { 1, 0 };
		if (setsockopt(client_fd, SOL_SOCKET, SO_LINGER, &opt, sizeof(opt)) < 0) {
			error_str("setsockopt error");
		}

		client_t* new_cl   = new client_t(&change_list);
		new_cl->_client_fd = client_fd;
		new_cl->_rdbuf	   = rdbuf;
		new_cl->_port_info = &port_info;
		new_cl->_storage   = storage;

		add_change_list(change_list, client_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, new_cl);
		add_change_list(change_list, client_fd, EVFILT_WRITE, EV_ADD | EV_DISABLE, 0, 0, new_cl);
		// add_change_list(change_list, client_fd, EVFILT_TIMER,
		// 				EV_ADD, NOTE_SECONDS, 60, new_cl);
		return true;
	}
}

void
read_event_handler(port_list_t& port_info, struct kevent* cur_event,
				   event_list_t& change_list, rdbuf_t* rdbuf, session_storage_t* storage) {

	if (cur_event->ident < port_info.size()) {
		if (create_client_event(cur_event->ident, cur_event, change_list,
								port_info[cur_event->ident], rdbuf, storage)
			== false) {
			// TODO: error ???
		}
		return;
	}
	client_t* cl = static_cast<client_t*>(cur_event->udata);
	if (cur_event->ident == cl->_client_fd) {
		spx_log_("read_event_handler - client_fd. data size", cur_event->data);
		cl->read_to_client_buffer_(cur_event);
	} else if (cl->_req._uri_resolv.is_cgi_) {
		// read from cgi output.
		spx_log_("read_event_handler - cgi");
		cl->read_to_cgi_buffer_(cur_event);
	} else {
		spx_log_("read_event_handler - server_file");
		// server file read case for res_body.
		// if (cl->_res._body_read == 0) {
		// 	add_change_list(change_list, cl->_client_fd, EVFILT_WRITE, EV_ENABLE, 0, 0, cl);
		// }
		cl->read_to_res_buffer_(cur_event);
	}
}

void
kevent_error_handler(port_list_t& port_info, struct kevent* cur_event,
					 event_list_t& change_list) {
	if (cur_event->ident < port_info.size()) {
		error_str("kevent() error");
		for (int i = 0; i < port_info.size(); i++) {
			if (port_info[i].listen_sd == i) {
				close(i);
			}
		}
		exit(spx_error);
	} else if (cur_event->filter == EVFILT_PROC) {
		// spx_log_("PROC ERROR?", strerror(cur_event->data));
		// exit(1);
		proc_event_wait_pid_(cur_event, change_list);
	} else {
		client_t* cl = static_cast<client_t*>(cur_event->udata);
		if (cl != NULL && cl->_client_fd == cur_event->ident) {
			cl->disconnect_client_();
			delete cl;
		}
		// else if (cl != NULL) {
		// 	spx_log_("cgi_fd_error?");
		// }
	}
}

void
write_event_handler(port_list_t& port_info, struct kevent* cur_event,
					event_list_t& change_list) {
	client_t* cl = static_cast<client_t*>(cur_event->udata);

	if (cl == NULL) {
		return;
	}
	if (cur_event->ident == cl->_client_fd) {
		// spx_log_("write event handler - cl state", cl->_state);
		if (cl->write_response_() == false) {
			return;
		}
		if (cl->_state != REQ_HOLD) {
			// spx_log_("write_event_handler - not REQ_HOLD");
			// spx_log_("write event handler - cl state", cl->_state);
			cl->req_res_controller_(cur_event);
		}
	} else {
		if (cur_event->ident == cl->_req._body_fd) {
			// spx_log_("write_for_upload");
			if (cl->write_for_upload_(cur_event) == false) {
				// spx_log_("too large file to upload");
				cl->error_response_keep_alive_(HTTP_STATUS_NOT_ACCEPTABLE);
				return;
			}
		} else {
			// spx_log_("write_to_cgi");
			cl->write_to_cgi_(cur_event);
		}
	}
}

void
proc_event_wait_pid_(struct kevent* cur_event, event_list_t& change_list) {
	client_t* cl = static_cast<client_t*>(cur_event->udata);
	int		  status;
	id_t	  pid;

	pid = waitpid(cur_event->ident, &status, 0);
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
kqueue_module(port_list_t& port_info) {

	event_list_t	  change_list;
	struct kevent	  event_list[MAX_EVENT_LIST];
	int				  kq;
	rdbuf_t			  rdbuf(BUFFER_SIZE, IOV_VEC_SIZE);
	session_storage_t storage;

	kq = kqueue();
	if (kq == -1) {
		error_str("kqueue() error");
		for (int i = 0; i < port_info.size(); i++) {
			if (port_info[i].listen_sd == i) {
				close(i);
			}
		}
		exit(spx_error);
	}

	for (int i = 0; i < port_info.size(); i++) {
		if (port_info[i].listen_sd == i) {
			add_change_list(change_list, i, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
		}
	}

	int			   event_len;
	struct kevent* cur_event;
	// int			   l = 0;

	while (true) {
		event_len = kevent(kq, &change_list.front(), change_list.size(),
						   event_list, MAX_EVENT_LIST, NULL);
		if (event_len == -1) {
			error_str("kevent() error");
			for (int i = 0; i < port_info.size(); i++) {
				if (port_info[i].listen_sd == i) {
					close(i);
				}
			}
			exit(spx_error);
		}
		change_list.clear();
		// std::cout << "current loop: " << l++ << std::endl;

		for (int i = 0; i < event_len; ++i) {
			cur_event = &event_list[i];
			spx_log_("event_len:", event_len);
			spx_log_("cur->ident:", cur_event->ident);
			spx_log_("cur->flags:", cur_event->flags);
			if (cur_event->udata != NULL) {
				spx_log_("state", ((Client*)cur_event->udata)->_state);
			}
			if (cur_event->flags & (EV_ERROR | EV_EOF)) {
				if (cur_event->flags & EV_ERROR) {
					if (cur_event->filter == EVFILT_PROC) {
						spx_log_("PROC1 flags", cur_event->flags);
						spx_log_("PROC1 fflags", cur_event->fflags);
						exit(1);
					}
					// kevent_error_handler(port_info, cur_event, change_list);
					// switch (cur_event->data) {
					// case ENOENT:
					// 	break;
					// }
					// continue;
				} else {
					// eof close fd.
					client_t* cl = static_cast<client_t*>(cur_event->udata);
					if (cl == NULL) {
						continue;
					}
					if (cur_event->ident == cl->_client_fd) {
						spx_log_("client socket closed", cur_event->ident);
						cl->disconnect_client_();
						delete cl;
					} else {
						if (cur_event->filter == EVFILT_PROC) {
							if (cur_event->filter == EVFILT_PROC) {
								spx_log_("PROC2 flags", cur_event->flags);
								spx_log_("PROC2 fflags", cur_event->fflags);
								// exit(1);
							}
							proc_event_wait_pid_(cur_event, change_list);
						} else {
							// cgi case. server file does not return EV_EOF.
							if (cur_event->filter == EVFILT_READ) {
								int n_read = cl->_rdbuf->read_(cur_event->ident, cl->_cgi._from_cgi);
								if (n_read < 0) {
									cl->disconnect_client_();
									delete cl;
									return;
								}
								if (cl->_cgi._cgi_state == CGI_HEADER) {
									cl->_cgi.cgi_controller_(*cl);
								}
								spx_log_("cgi read close");
								close(cur_event->ident);
								add_change_list(change_list, cur_event->ident, EVFILT_READ, EV_DELETE, 0, 0, cl);
								break;
							} else {
								spx_log_("cgi write close");
								close(cur_event->ident);
								add_change_list(change_list, cur_event->ident, cur_event->filter, EV_DELETE, 0, 0, NULL);
							}
						}
					}
				}
				continue;
			}
			switch (cur_event->filter) {
			case EVFILT_READ:
				spx_log_("event_read");
				// usleep(1000);
				read_event_handler(port_info, cur_event, change_list, &rdbuf, &storage);
				break;
			case EVFILT_WRITE:
				// usleep(1000);
				write_event_handler(port_info, cur_event, change_list);
				break;
			case EVFILT_PROC:
				if (cur_event->filter == EVFILT_PROC) {
					spx_log_("PROC3 flags", cur_event->flags);
					spx_log_("PROC3 fflags", cur_event->fflags);
					exit(1);
				}
				proc_event_wait_pid_(cur_event, change_list);
				break;
				// case EVFILT_TIMER:
				// 	// spx_log_("event_timer");
				// 	timer_event_handler(cur_event, change_list);
				// 	// TODO: timer
				// 	break;
			}
		}
	}
	return;
}
