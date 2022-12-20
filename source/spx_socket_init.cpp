#include "spx_socket_init.hpp"
#include "spx_config_port_info.hpp"
#include "spx_core_type.hpp"
#include "spx_core_util_box.hpp"
#include <vector>

namespace {

} // namespace

port_info_t::port_info(server_info_t const& from)
	: my_port_default_server(from) {
}

status
socket_init_and_build_port_info(total_port_server_map_p&  config_info,
								std::vector<port_info_t>& port_info,
								uint32_t&				  socket_size) {

	for (total_port_server_map_p::const_iterator it = config_info.begin(); it != config_info.end(); ++it) {

		server_map_p::const_iterator it2 = it->second.begin();
		while (it2 != it->second.end()) {
			if (it2->second.default_server_flag & Kdefault_server) {
				port_info_t temp_port_info(it2->second);
				temp_port_info.my_port	   = it->first;
				temp_port_info.my_port_map = it->second;

				temp_port_info.listen_sd = socket(AF_INET, SOCK_STREAM, 0); // TODO :: key
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
				if (socket_size == 0) {
					socket_size = temp_port_info.listen_sd;
					{
						uint32_t i = 0;
						while (i < socket_size) {
							port_info.push_back(temp_port_info);
							++i;
						}
					}
				}
				port_info.push_back(temp_port_info);
				++socket_size;
				break;
			}
			++it2;
		}
		if (it2 == it->second.end()) {
			std::cerr << "no default server in port " << it->first << std::endl;
			error_exit_msg("");
		}
	}
	if (socket_size == 0 || socket_size > 65535) {
		error_exit_msg("socket size error");
	}
	config_info.clear();
	return spx_ok;
}
