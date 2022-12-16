#include "spx_socket_init.hpp"
#include "spx_port_info.hpp"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <map>
#include <string>
#include <sys/event.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include <vector>

#define SERVER_PORT 1234

void
error_exit(std::string err, int (*func)(int), int fd) {
	if (func != NULL) {
		func(fd);
	}
	std::cout << errno << std::endl;
	perror(err.c_str());
	exit(EXIT_FAILURE);
}

void
change_events(std::vector<struct kevent>& change_list, uintptr_t ident,
			  int16_t filter, uint16_t flags, uint32_t fflags,
			  intptr_t data, void* udata) {
	struct kevent temp_event;
	EV_SET(&temp_event, ident, filter, flags, fflags, data, udata);
	change_list.push_back(temp_event);
}

void
disconnect_client(int client_fd, std::map<int, std::string>& clients) {
	std::cout << "client disconnected: " << client_fd << std::endl;
	close(client_fd);
	clients.erase(client_fd);
}

status
socket_init_build_port_info(total_port_server_map_p& config_info,
							port_info_map&			 port_info) {

	for (total_port_server_map_p::const_iterator it = config_info.begin(); it != config_info.end(); ++it) {
		port_info_t temp_port_info;
		temp_port_info.my_port	   = it->first;
		temp_port_info.my_port_map = it->second;
		// for (server_map_p::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
		// 	if (it2->second.default_server_flag & Kdefault_server) {
		// 		break;
		// 	}
		// }

		temp_port_info.listen_sd = socket(AF_INET, SOCK_STREAM, 0);
		if (temp_port_info.listen_sd < 0) {
			error_exit("socket", NULL, 0);
		}

		int opt(1);
		// SO_REUSEPORT
		if (setsockopt(temp_port_info.listen_sd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
			error_exit("setsockopt", close, temp_port_info.listen_sd);
		}

		if (fcntl(temp_port_info.listen_sd, F_SETFL, O_NONBLOCK) == -1) {
			error_exit("fcntl", close, temp_port_info.listen_sd);
		}

		temp_port_info.addr_server.sin_family	   = AF_INET;
		temp_port_info.addr_server.sin_port		   = htons(temp_port_info.my_port);
		temp_port_info.addr_server.sin_addr.s_addr = htonl(INADDR_ANY);

		if (bind(temp_port_info.listen_sd, (struct sockaddr*)&temp_port_info.addr_server, sizeof(temp_port_info.addr_server)) == -1) {
			error_exit("bind", close, temp_port_info.listen_sd);
		}

		if (listen(temp_port_info.listen_sd, LISTEN_BACKLOG_SIZE) < 0) {
			error_exit("listen", close, temp_port_info.listen_sd);
		}

		port_info.insert(std::make_pair(temp_port_info.my_port, temp_port_info));
	}
	config_info.clear();
	return spx_ok;
}

int
socket_init(total_port_server_map_p const& config_info) {
	int listen_sd = 0;

	// sleep( 20 );
	// std::cout << "sleep" << std::endl;

	struct timespec timeout;
	timeout.tv_sec	= 10;
	timeout.tv_nsec = 0;

	/* init kqueue */
	int kq;
	if ((kq = kqueue()) == -1) {
		error_exit("kqueue", close, listen_sd);
	}

	std::map<int, std::string> clients; // map for client socket:data
	std::vector<struct kevent> change_list; // kevent vector for changelist
	struct kevent			   event_list[8]; // kevent array for eventlist

	/* add event for server socket */
	change_events(change_list, listen_sd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
	std::cout << "echo server started" << std::endl;

	/* main loop */
	int			   new_events;
	struct kevent* curr_event;
	while (1) {
		/*  apply changes and return new events(pending events) */
		new_events = kevent(kq, &change_list[0], change_list.size(),
							event_list, 8, &timeout);

		if (new_events == -1) {
			error_exit("kevent", NULL, 0);
		}

		change_list.clear(); // clear change_list for new changes

		for (int i = 0; i < new_events; ++i) {
			curr_event = &event_list[i];

			/* check error event return */
			if (curr_event->flags & EV_ERROR) {
				if (curr_event->ident == static_cast<uintptr_t>(listen_sd))
					error_exit("server socket error", NULL, 0);
				else {
					std::cerr << "client socket error" << std::endl;
					disconnect_client(curr_event->ident, clients);
				}
			} else if (curr_event->filter == EVFILT_READ) {
				if (curr_event->ident == static_cast<uintptr_t>(listen_sd)) {
					/* accept new client */
					int client_socket;
					if ((client_socket = accept(listen_sd, NULL, NULL)) == -1)
						error_exit("accept", NULL, 0);
					std::cout << "accept new client: " << client_socket
							  << std::endl;
					fcntl(client_socket, F_SETFL, O_NONBLOCK);

					/* add event for client socket - add read && write event */
					change_events(change_list, client_socket, EVFILT_READ,
								  EV_ADD | EV_ENABLE, 0, 0, NULL);
					change_events(change_list, client_socket, EVFILT_WRITE,
								  EV_ADD | EV_ENABLE, 0, 0, NULL);
					clients[client_socket] = "";
				} else if (clients.find(curr_event->ident) != clients.end()) {
					/* read data from client */
					char buf[10];
					int	 n = read(curr_event->ident, buf, sizeof(buf));

					if (n <= 0) {
						if (n < 0) {
							std::cerr << "client read error!" << std::endl;
						}
						disconnect_client(curr_event->ident, clients);
					} else {
						buf[n] = '\0';
						clients[curr_event->ident] += buf;
						std::cout << "received data from " << curr_event->ident
								  << ": " << clients[curr_event->ident]
								  << std::endl;
					}
				}
			} else if (curr_event->filter == EVFILT_WRITE) {
				/* send data to client */
				std::map<int, std::string>::iterator it = clients.find(curr_event->ident);
				std::cout << "write" << std::endl;
				usleep(500000);
				if (it != clients.end()) {
					if (clients[curr_event->ident] != "") {
						int n;
						if ((n = write(curr_event->ident,
									   clients[curr_event->ident].c_str(),
									   clients[curr_event->ident].size())
								 == -1)) {
							std::cerr << "client write error!" << std::endl;
							disconnect_client(curr_event->ident, clients);
						} else {
							// write( curr_event->ident, "ok", strlen( "ok" ) );
							clients[curr_event->ident].clear();
						}
					}
				}
			}
		}
	}
	(void)config_info;
	return (0);
}
