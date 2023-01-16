#include "spacex.hpp"

#include "spx_client.hpp"
#include "spx_kqueue_module.hpp"

#include <csignal>
#include <cstdlib>

namespace {

#ifdef LEAK
	void
	ft_handler_leak_() {
		system("leaks spacex");
	}
#endif

	inline total_port_server_map_p
	config_file_open__(int argc, char const* argv[], std::string const& cur_path) {
		std::fstream file;
		switch (argc) {
		case 1:
			file.open("./config/default.conf", std::ios_base::in);
			break;
		case 2: {
			std::string conf_file = argv[1];
			if (conf_file.size() < 6 || conf_file.substr(conf_file.size() - 5, 5) != ".conf") {
				error_exit("config file must be end with '.conf' and config file name must be longer than 5");
			}
			file.open(argv[1], std::ios_base::in);
			break;
		}
		default:
			error_exit("usage: ./spacex [config_file]");
		}
		if (file.is_open() == false) {
			error_fn("file open error", exit, spx_error);
		}
		std::stringstream ss;
		ss << file.rdbuf();
		if (ss.fail()) {
			file.close();
			error_fn("file read error", exit, spx_error);
		}
		file.close();
		total_port_server_map_p temp_config;
		if (spx_config_syntax_checker(ss.str(), temp_config, cur_path) != spx_ok) {
			error_exit("config syntax error");
		}
		return temp_config;
	}

	inline void
	get_current_path__(std::string& cur_path) {
		char buf[8192];
		if (getcwd(buf, sizeof(buf)) == NULL) {
			error_fn("getcwd error", exit, spx_error);
		}
		cur_path = buf;
	}

	inline void
	port_info_print__(main_info_t const& spx) {
		int64_t i	   = spx.first_socket;
		int64_t prev_i = -1;

		std::cout << "\n--------------- [ " << COLOR_BLUE << "SpaceX Info" << COLOR_RESET
				  << " ] ---------------\n"
				  << std::endl;
		while (i < spx.socket_size) {
			if (prev_i == -1 || spx.port_info[prev_i].my_port != spx.port_info[i].my_port) {
				std::cout << "socket: " << COLOR_GREEN << i << COLOR_RESET;
				std::cout << "\t| port: " << COLOR_GREEN << spx.port_info[i].my_port_default_server.port << COLOR_RESET;
				std::cout << "\t| name: " << COLOR_GREEN << spx.port_info[i].my_port_default_server.server_name << COLOR_RESET;
				std::cout << COLOR_RED << "\t <---- default server" << COLOR_RESET << std::endl;
				for (server_map_p::const_iterator it2 = spx.port_info[i].my_port_map.begin();
					 it2 != spx.port_info[i].my_port_map.end(); ++it2) {
					if (!(it2->second.default_server_flag & Kdefault_server)) {
						std::cout << "socket: " << COLOR_GREEN << i << COLOR_RESET;
						std::cout << "\t| port: " << COLOR_GREEN << it2->second.port << COLOR_RESET;
						std::cout << "\t| name: " << COLOR_GREEN << it2->second.server_name << COLOR_RESET;
						std::cout << std::endl;
					}
				}
				std::cout << std::endl;
			}
			prev_i = i;
			++i;
		}
		std::cout << "-----------------------------------------------" << std::endl;
	}

} // namespace

int
main(int argc, char const* argv[]) {

#ifdef LEAK
	atexit(ft_handler_leak_);
#endif

	if (argc <= 2) {
		signal(SIGPIPE, SIG_IGN);
		signal(SIGCHLD, SIG_IGN);

		std::string cur_dir;
		get_current_path__(cur_dir);

		total_port_server_map_p config_info = config_file_open__(argc, argv, cur_dir);

		main_info_t spx;
		spx.socket_size = -1;

		socket_init_and_build_port_info(config_info, spx.port_info, spx.socket_size, spx.first_socket);
		port_info_print__(spx);

		kqueue_module(spx.port_info);

	} else {
		error_exit("usage: ./spacex [config_file]");
	}
	return spx_ok;
}
