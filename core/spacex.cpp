#include "spx_config_parse.hpp"
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#ifdef LEAK
void
ft_handler_leak() {
	system("leaks spacex");
}
#endif

namespace {

	template <typename T>
	inline void
	spx_log_(T msg) {
#ifdef DEBUG
		std::cout << "\033[1;32m" << msg << ";"
				  << "\033[0m" << std::endl;
#else
		(void)msg;
#endif
	}

	inline status
	error_(const char* msg, const char* msg2) {
		std::cout << "\033[1;31m[ " << msg << " ]\033[0m"
				  << " : "
				  << "\033[1;33m" << msg2 << "\033[0m" << std::endl;
		exit(spx_error);
	}

	inline status
	perror_(const char* msg) {
		perror(msg);
		exit(spx_error);
	}

}

total_port_server_map_p
config_file_open_(int argc, char const* argv[]) {

	std::fstream file;
	switch (argc) {
	case 1: {
		file.open("./config/default.conf", std::ios_base::in);
		break;
	}
	case 2: {
		file.open(argv[1], std::ios_base::in);
		break;
	}
	default: {
		error_("usage", "./spacex [config_file]");
	}
	}

	if (file.is_open() == false) {
		perror_("file open error");
	}

	std::stringstream ss;
	ss << file.rdbuf();
	if (ss.fail()) {
		error_("stringstream", "file read error");
		file.close();
		perror_("file read error");
	}
	file.close();

#ifdef MAIN_DEBUG
	std::cout << ss.str() << std::endl;
#endif

	total_port_server_map_p temp_config;
	if (spx_config_syntax_checker(ss.str(), temp_config) != spx_ok) {
		error_("config", "syntax error");
	}

	return temp_config;
}

int
main(int argc, char const* argv[]) {
#ifdef LEAK
	atexit(ft_handler_leak);
#endif
	if (argc <= 2) {
		total_port_server_map_p config_info = config_file_open_(argc, argv);
		spx_log_("config file open success");
		// server_map_p&			test		= config_info.find(443)->second;

#ifdef MAIN_DEBUG
		for (total_port_server_map_p::iterator print = config_info.begin(); print != config_info.end(); ++print) {
			std::cout << "port: " << print->first << std::endl;
			for (server_map_p::iterator print2 = print->second.begin(); print2 != print->second.end(); ++print2) {
				print2->second.print();
			}
		}
#endif

		// socket start

	} else {
		error_("usage", "./spacex [config_file]");
	}
	return spx_ok;
}
