#include "spacex.hpp"

#include "spx_client_buffer.hpp"
#include "spx_kqueue_module.hpp"
#include "spx_port_info.hpp"

namespace {

#ifdef LEAK
	void
	ft_handler_leak_() {
		system("leaks spacex");
	}
#endif

	inline total_port_server_map_p
	config_file_open_(int argc, char const* argv[], std::string const& cur_path) {
		std::fstream file;
		switch (argc) {
		case 1:
			file.open("./config/default.conf", std::ios_base::in);
			break;
		case 2:
			file.open(argv[1], std::ios_base::in);
			break;
		default:
			error_exit_msg("usage: ./spacex [config_file]");
		}
		if (file.is_open() == false) {
			error_exit_msg_perror("file open error");
		}
		std::stringstream ss;
		ss << file.rdbuf();
		if (ss.fail()) {
			file.close();
			error_exit_msg_perror("file read error");
		}
		file.close();
		total_port_server_map_p temp_config;
		if (spx_config_syntax_checker(ss.str(), temp_config, cur_path) != spx_ok) {
			error_exit_msg("config syntax error");
		}
		return temp_config;
	}

	inline void
	get_current_path_(std::string& cur_path) {
		char buf[8192];
		if (getcwd(buf, sizeof(buf)) == NULL) {
			error_exit_msg_perror("getcwd error");
		}
		cur_path = buf;
	}

	inline void
	port_info_print_(main_info_t const& spx) {
		uint32_t i = 3;
		std::cout << "\n-------------- [ " << COLOR_BLUE << "SpaceX Info" << COLOR_RESET
				  << " ] -------------\n"
				  << std::endl;
		while (i < spx.socket_size) {
			for (server_map_p::const_iterator it2 = spx.port_info[i].my_port_map.begin();
				 it2 != spx.port_info[i].my_port_map.end(); ++it2) {
				std::cout << "port: " << COLOR_GREEN << it2->second.port << COLOR_RESET;
				std::cout << "\t| name: " << COLOR_GREEN << it2->second.server_name << COLOR_RESET;
				if (it2->second.default_server_flag == Kdefault_server) {
					std::cout << COLOR_RED << "\t <---- default server" << COLOR_RESET << std::endl;
				} else {
					std::cout << std::endl;
				}
			}
			std::cout << std::endl;
			++i;
		}
		std::cout << "--------------------------------------------" << std::endl;
	}

} // namespace

int
main(int argc, char const* argv[]) {

#ifdef LEAK
	atexit(ft_handler_leak_);
#endif

	if (argc <= 2) {
		std::string cur_dir;
		get_current_path_(cur_dir);

		total_port_server_map_p config_info = config_file_open_(argc, argv, cur_dir);

		main_info_t spx;
		spx.socket_size = 0;

		socket_init_and_build_port_info(config_info, spx.port_info, spx.socket_size);
		port_info_print_(spx);

		kqueue_module(spx.port_info);

	} else {
		error_exit_msg("usage: ./spacex [config_file]");
	}
	return spx_ok;
}
