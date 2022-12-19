#include "spx_socket_init.hpp"
#include "spx_config_port_info.hpp"
#include "spx_core_util_box.hpp"

namespace {

} // namespace

port_info_t::port_info(void) {
}

status
socket_init_and_build_port_info(total_port_server_map_p& config_info,
								port_info_map&			 port_info) {

	for (total_port_server_map_p::const_iterator it = config_info.begin(); it != config_info.end(); ++it) {
		port_info_t temp_port_info;

		temp_port_info.my_port	   = it->first;
		temp_port_info.my_port_map = it->second;

		for (server_map_p::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
			if (it2->second.default_server_flag & Kdefault_server) {
				std::cout << "check default_server info print" << std::endl;
				// it2->second.print(); // default_server_info
				// NOTE:  default_server copy is not properly work.print trash value... TT break;
			}
		}

		temp_port_info.listen_sd = socket(AF_INET, SOCK_STREAM, 0);
		if (temp_port_info.listen_sd < 0) {
			error_exit("socket", NULL, 0);
		}
		int opt(1);
		if (setsockopt(temp_port_info.listen_sd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) { // NOTE:: SO_REUSEPORT
			error_exit("setsockopt", close, temp_port_info.listen_sd);
		}
		if (fcntl(temp_port_info.listen_sd, F_SETFL, O_NONBLOCK) == -1) {
			error_exit("fcntl", close, temp_port_info.listen_sd);
		}
		temp_port_info.addr_server.sin_family	   = AF_INET;
		temp_port_info.addr_server.sin_port		   = htons(temp_port_info.my_port);
		temp_port_info.addr_server.sin_addr.s_addr = htonl(INADDR_ANY);
		if (bind(temp_port_info.listen_sd, (struct sockaddr*)&temp_port_info.addr_server, sizeof(temp_port_info.addr_server)) == -1) {
			std::stringstream ss;
			ss << temp_port_info.my_port;
			std::string err = "bind port " + ss.str() + " ";
			error_exit(err, close, temp_port_info.listen_sd);
		}
		if (listen(temp_port_info.listen_sd, LISTEN_BACKLOG_SIZE) < 0) {
			error_exit("listen", close, temp_port_info.listen_sd);
		}
		port_info.insert(std::make_pair(temp_port_info.my_port, temp_port_info));
	}
	config_info.clear();
	return spx_ok;
}
