#include "spx_kqueue_module.hpp"
#include "spx_client.hpp"

void
kqueue_module(port_list_t& port_info) {

	event_list_t		   change_list;
	struct kevent		   event_list[MAX_EVENT_LIST];
	int					   kq;
	rdbuf_t				   rdbuf(BUFFER_SIZE, IOV_VEC_SIZE);
	session_storage_t	   storage;
	std::vector<client_t*> for_close;

	kq = kqueue();
	if (kq == -1) {
		error_str("kqueue() error");
		for (int i = 0; static_cast<size_t>(i) < port_info.size(); i++) {
			if (port_info[i].listen_sd == i) {
				close(i);
			}
		}
		exit(spx_error);
	}

	for (int i = 0; static_cast<size_t>(i) < port_info.size(); i++) {
		if (port_info[i].listen_sd == i) {
			add_change_list(change_list, i, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
		}
	}
	// Session Checker
	add_change_list(change_list, -1, EVFILT_TIMER, EV_ADD, NOTE_SECONDS, EXPIRED_CLEANER_TIME,
					&storage);

	int			   event_len;
	struct kevent* cur_event;

	while (true) {
		event_len = kevent(kq, &change_list.front(), change_list.size(),
						   event_list, MAX_EVENT_LIST, NULL);
		if (event_len == -1) {
			error_str("kevent() error");
			for (int i = 0; static_cast<size_t>(i) < port_info.size(); i++) {
				if (port_info[i].listen_sd == i) {
					close(i);
				}
			}
			exit(spx_error);
		}
		change_list.clear();

		for (int i = 0; i < event_len; ++i) {
			cur_event = &event_list[i];
			if (cur_event->flags & (EV_ERROR | EV_EOF)) {
				if (cur_event->flags & EV_ERROR) {
					kevent_error_handler(port_info, cur_event, for_close);
				} else {
					kevent_eof_handler(cur_event, for_close);
				}
				continue;
			}
			switch (cur_event->filter) {
			case EVFILT_READ:
				read_event_handler(port_info, cur_event, change_list, &rdbuf, &storage);
				break;
			case EVFILT_WRITE:
				write_event_handler(cur_event);
				break;
			case EVFILT_TIMER:
				if (cur_event->ident == SIZE_T_MAX) {
					session_timer_event_handler(cur_event);
				}
				// else {
				// 	timer_event_handler(cur_event, change_list, for_close);
				// }
				break;
			}
		}
		// for delete client.
		if (for_close.size()) {
			while (for_close.size()) {
				delete for_close.back();
				for_close.pop_back();
			}
		}
	}
	return;
}

void
add_change_list(event_list_t& change_list, uintptr_t ident, int64_t filter,
				uint16_t flags, uint32_t fflags, intptr_t data, void* udata) {
	struct kevent tmp_event;
	EV_SET(&tmp_event, ident, filter, flags, fflags, data, udata);
	change_list.push_back(tmp_event);
}

bool
create_client_event(uintptr_t serv_sd, event_list_t& change_list, port_info_t& port_info,
					rdbuf_t* rdbuf, session_storage_t* storage) {

#ifdef CONSOLE_LOG
	struct sockaddr_in client_sockaddr;
	socklen_t		   client_sock_len = sizeof(sockaddr_in);
	uintptr_t		   client_fd	   = accept(serv_sd, (struct sockaddr*)&client_sockaddr, &client_sock_len);
#else
	uintptr_t client_fd = accept(serv_sd, NULL, NULL);
#endif

	if (client_fd == SIZE_T_MAX) {
		error_str("accept error");
		return false;
	} else {
		fcntl(client_fd, F_SETFL, O_NONBLOCK);

		struct linger opt = { 1, 0 };
		if (setsockopt(client_fd, SOL_SOCKET, SO_LINGER, &opt, sizeof(opt)) < 0) {
			error_str("setsockopt error");
		}

		client_t* new_cl = new client_t(&change_list);

		spx_log_(sizeof(client_t));
		new_cl->_client_fd = client_fd;
		new_cl->_rdbuf	   = rdbuf;
		new_cl->_port_info = &port_info;
		new_cl->_storage   = storage;
#ifdef CONSOLE_LOG
		new_cl->_sockaddr = client_sockaddr;
#endif

		add_change_list(change_list, client_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, new_cl);

		return true;
	}
}

void
read_event_handler(port_list_t& port_info, struct kevent* cur_event,
				   event_list_t& change_list, rdbuf_t* rdbuf, session_storage_t* storage) {

	if (cur_event->ident < port_info.size()) {
		if (create_client_event(cur_event->ident, change_list,
								port_info[cur_event->ident], rdbuf, storage)
			== false) {
			std::cerr << "create client error" << std::endl;
		}
		return;
	}
	client_t* cl = static_cast<client_t*>(cur_event->udata);
	if (cur_event->ident == cl->_client_fd) {
		cl->read_to_client_buffer_(cur_event);
	} else if (cl->_req._uri_resolv.is_cgi_) {
		// read from cgi output.
		cl->read_to_cgi_buffer_(cur_event);
	} else {
		cl->read_to_res_buffer_(cur_event);
	}
}

void
write_event_handler(struct kevent* cur_event) {
	client_t* cl = static_cast<client_t*>(cur_event->udata);

	if (cl == NULL) {
		return;
	}
	if (cur_event->ident == cl->_client_fd) {
		if (cl->write_response_() == false) {
			return;
		}
		if (cl->_state != REQ_HOLD) {
			cl->req_res_controller_(cur_event);
		}
	} else {
		if (cur_event->ident == static_cast<size_t>(cl->_req._body_fd)) {
			if (cl->write_for_upload_(cur_event) == false) {
				return;
			}
		} else {
			cl->write_to_cgi_(cur_event);
		}
	}
}

void
proc_event_wait_pid(struct kevent* cur_event) {
	client_t* cl = static_cast<client_t*>(cur_event->udata);
	int		  status;
	int		  pid;

	pid = waitpid(cl->_cgi._pid, &status, 0);
	if (cur_event->data) {
		// cgi_process error exit case
		cl->error_response_keep_alive_(HTTP_STATUS_INTERNAL_SERVER_ERROR);
	}
	spx_log_("pid ", pid);
	spx_log_("status ", cur_event->data);
	(void)pid;
}

void
kevent_error_handler(port_list_t& port_info, struct kevent* cur_event, close_vec_t& for_close) {
	if (cur_event->ident < port_info.size()) {
		error_str("kevent() error!");
		for (int i = 0; static_cast<size_t>(i) < port_info.size(); i++) {
			if (port_info[i].listen_sd == i) {
				close(i);
			}
		}
		exit(spx_error);
	} else {
		client_t* cl = static_cast<client_t*>(cur_event->udata);
		if (cur_event->filter == EVFILT_READ) {
			if (cur_event->ident == cl->_client_fd) {
				cl->disconnect_client_();
				close(cur_event->ident);
				for_close.push_back(cl);
				return;
			}
		}
	}
}

void
kevent_eof_handler(struct kevent* cur_event, close_vec_t& for_close) {
	// eof close fd.
	client_t* cl = static_cast<client_t*>(cur_event->udata);
	if (cl == NULL) {
		return;
	}
	if (cur_event->filter == EVFILT_READ) {
		if (cur_event->ident == cl->_client_fd) {
			cl->disconnect_client_();
			close(cur_event->ident);
			for_close.push_back(cl);
			return;
		}
		int n_read = cl->_rdbuf->read_(cur_event->ident, cl->_cgi._from_cgi);
		if (cur_event->data && n_read <= 0) {
			close(cur_event->ident);
			// proc_event_wait_pid(cur_event);
			return;
		}
		if (n_read == cur_event->data) {
			cl->_cgi._cgi_done	= true;
			cl->_cgi._cgi_state = CGI_HEADER;

			cl->_cgi.cgi_controller_(*cl);

			close(cur_event->ident);
			// proc_event_wait_pid(cur_event);
		}
	} else if (cur_event->filter == EVFILT_WRITE) {
		close(cur_event->ident);
	} else {
		proc_event_wait_pid(cur_event);
	}
}

// void
// timer_event_handler(struct kevent* cur_event, event_list_t& change_list, close_vec_t& for_close) {
// 	// timeout(60s)
// 	client_t* cl = static_cast<client_t*>(cur_event->udata);
// 	cl->disconnect_client_();
// 	close(cur_event->ident);
// 	for_close.push_back(static_cast<client_t*>(cur_event->udata));
// 	add_change_list(change_list, cur_event->ident, EVFILT_TIMER, EV_DELETE, 0, 0, NULL);
// 	add_change_list(change_list, cur_event->ident, EVFILT_READ, EV_DELETE, 0, 0, NULL);
// 	add_change_list(change_list, cur_event->ident, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
// }

void
session_timer_event_handler(struct kevent* cur_event) {
	// execute every 1hour (3600s)
	session_storage_t* storage;
	storage = (session_storage_t*)(cur_event->udata);
	storage->session_cleaner();
}
