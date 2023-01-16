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

#ifdef CONSOLE_LOG
	struct sockaddr_in client_sockaddr;
	socklen_t		   client_sock_len = sizeof(sockaddr_in);
	uintptr_t		   client_fd	   = accept(serv_sd, (struct sockaddr*)&client_sockaddr, &client_sock_len);
#else
	uintptr_t client_fd = accept(serv_sd, NULL, NULL);
#endif

	if (client_fd == -1) {
		error_str("accept error");
		return false;
	} else {
		// std::cerr << "accept new client: " << client_fd << std::endl;
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
#ifdef CONSOLE_LOG
		new_cl->_sockaddr = client_sockaddr;
#endif

		add_change_list(change_list, client_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, new_cl);
		// add_change_list(change_list, client_fd, EVFILT_WRITE, EV_ADD | EV_DISABLE, 0, 0, new_cl);
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
			std::cerr << "create client error" << std::endl;
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
		error_str("kevent() error!");
		for (int i = 0; i < port_info.size(); i++) {
			if (port_info[i].listen_sd == i) {
				close(i);
			}
		}
		exit(spx_error);
	}
	// else {
	// 	// client_t* cl = static_cast<client_t*>(cur_event->udata);
	// 	// if (cl != NULL && cl->_client_fd == cur_event->ident) {
	// 	// 	cl->disconnect_client_();
	// 	// 	delete cl;
	// 	// }
	// }
}

void
write_event_handler(port_list_t& port_info, struct kevent* cur_event,
					event_list_t& change_list) {
	client_t* cl = static_cast<client_t*>(cur_event->udata);

	if (cl == NULL) {
		return;
	}
	if (cur_event->ident == cl->_client_fd) {
		spx_log_("write event handler - cl state", cl->_state);
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
			spx_log_("write_for_upload");
			if (cl->write_for_upload_(cur_event) == false) {
				// spx_log_("too large file to upload");
				// cl->error_response_keep_alive_(HTTP_STATUS_NOT_ACCEPTABLE);
				return;
			}
		} else {
			// exit(1);
			spx_log_("write_to_cgi");
			cl->write_to_cgi_(cur_event);
		}
	}
}

void
proc_event_wait_pid(struct kevent* cur_event, event_list_t& change_list) {
	client_t* cl = static_cast<client_t*>(cur_event->udata);
	int		  status;
	int		  pid;

	pid = waitpid(cl->_cgi._pid, &status, 0);
	spx_log_("waitpid stat_loc", status);
	if (WEXITSTATUS(status) == EXIT_FAILURE) { // NOTE: cgi_process error exit case
		// spx_log_(COLOR_RED "not detected by EXIT_FAILURE");
		// if (cl->_req._is_chnkd) {
		// 	cl->_state = REQ_SKIP_BODY_CHUNKED;
		// } else {
		// 	cl->_state = REQ_SKIP_BODY;
		// }
		close(cl->_cgi._write_to_cgi_fd);
		// add_change_list(change_list, cl->_cgi._write_to_cgi_fd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
		close(cl->_cgi._read_from_cgi_fd);
		// add_change_list(change_list, cl->_cgi._read_from_cgi_fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
		cl->error_response_keep_alive_(HTTP_STATUS_INTERNAL_SERVER_ERROR);
	}
	spx_log_("pid", pid);
	spx_log_("cl->_client_fd", cl->_client_fd);
	spx_log_("cur_event->ident", cur_event->ident);
	// if (l == 2) {
	// 	sleep(10);
	// 	exit(1);
	// }
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

// added by space 23.1.14
void
session_timer_event_handler(struct kevent* cur_event, event_list_t& change_list) {
	// execute every 1hour (3600s)
	session_storage_t* storage;
	storage = (session_storage_t*)(cur_event->udata);
	storage->session_cleaner();
}

bool
eof_case_handler(struct kevent* cur_event, event_list_t& change_list) {
	// eof close fd.
	client_t* cl = static_cast<client_t*>(cur_event->udata);
	if (cl == NULL) {
		return false;
	}
	// if (cur_event->ident == cl->_client_fd) {
	// 	if (cur_event->filter == EVFILT_READ) {
	// 		spx_log_("client socket closed", cur_event->ident);
	// 		// std::cerr << "client socket closed : " << cur_event->ident << std::endl;
	// 	}
	// } else {
	// cgi case. server file does not return EV_EOF.
	if (cur_event->filter == EVFILT_READ) {
		if (cur_event->ident == cl->_client_fd) {
			// cl->disconnect_client_();
			// delete cl;
			cl->disconnect_client_();
			close(cur_event->ident);
			delete cl;
			// add_change_list(change_list, cur_event->ident, EVFILT_TIMER, EV_ADD | EV_UDATA_SPECIFIC, NOTE_USECONDS, 10000, cl);
			return false;
		}
		int n_read = cl->_rdbuf->read_(cur_event->ident, cl->_cgi._from_cgi);
		if (cur_event->data && n_read <= 0) {
			// cl->disconnect_client_();
			// delete cl;
			close(cur_event->ident);
			proc_event_wait_pid(cur_event, change_list);
			return true;
		}
		spx_log_("n_read", n_read);
		if (n_read == cur_event->data) {
			cl->_cgi._cgi_done	= true;
			cl->_cgi._cgi_state = CGI_HEADER;

			cl->_cgi.cgi_controller_(*cl);

			spx_log_("cgi read close");
			close(cur_event->ident);
			// add_change_list(change_list, cur_event->ident, EVFILT_READ, EV_DELETE, 0, 0, cl);
			proc_event_wait_pid(cur_event, change_list);
		}
	} else {
		spx_log_("cgi write close");
		close(cur_event->ident);
		// add_change_list(change_list, cur_event->ident, cur_event->filter, EV_DELETE, 0, 0, NULL);
	}
	return true;
	// }
}

void
kqueue_module(port_list_t& port_info) {

	event_list_t	  change_list;
	struct kevent	  event_list[MAX_EVENT_LIST];
	int				  kq;
	rdbuf_t			  rdbuf(BUFFER_SIZE, IOV_VEC_SIZE);
	session_storage_t storage;

	signal(SIGPIPE, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);

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
	// Session Checker
	add_change_list(change_list, -1, EVFILT_TIMER, EV_ADD, NOTE_SECONDS, EXPIRED_CLEANER_TIME,
					&storage);

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
			spx_log_("event_filter:", cur_event->filter);
			spx_log_("cur->ident:", cur_event->ident);
			spx_log_("cur->flags:", cur_event->flags);
			spx_log_("cur->data:", cur_event->data);
			// if (cur_event->udata != NULL) {
			// 	spx_log_("state", ((Client*)cur_event->udata)->_state);
			// }
			if (cur_event->flags & (EV_ERROR | EV_EOF)) {
				if (cur_event->flags & EV_ERROR) {
					// if (cur_event->filter == EVFILT_PROC) {
					// 	spx_log_("PROC1 flags", cur_event->flags);
					// 	spx_log_("PROC1 fflags", cur_event->fflags);
					// 	// exit(1);
					// 	// proc_event_wait_pid(cur_event, change_list);
					// }
					kevent_error_handler(port_info, cur_event, change_list);
					// switch (cur_event->data) {
					// case ENOENT:
					// 	break;
					// }
					// continue;
				} else {
					if (eof_case_handler(cur_event, change_list) == false) {
						break;
					}
				}
				continue;
			}
			switch (cur_event->filter) {
			case EVFILT_READ:
				spx_log_("event_read");
				read_event_handler(port_info, cur_event, change_list, &rdbuf, &storage);
				break;
			case EVFILT_WRITE:
				write_event_handler(port_info, cur_event, change_list);
				break;
			case EVFILT_TIMER:
				spx_log_("event_timer");
				if (cur_event->ident == -1) {
					// added by space 23.1.14
					session_timer_event_handler(cur_event, change_list);
				} else {
					// TODO:  	timer
					// client_t* cl = static_cast<client_t*>(cur_event->udata);
					// delete cl;
					// add_change_list(change_list, cur_event->ident, EVFILT_TIMER, EV_DELETE | EV_UDATA_SPECIFIC, 0, 0, cl);
				}
				break;
			}
		}
	}
	return;
}
