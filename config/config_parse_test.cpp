#include "config.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

/* HOW TO USE*/
// c++ -std=c++98 -Wall -Werror -Wextra -D CONFIG_STATE_DEBUG -D REAK -D CONFIG_DEBUG *.cpp && ./a.out

#ifdef REAK
void
ft_handler() {
	system("leaks a.out");
}
#endif

void
inner_main(void) {
	std::fstream file("test.conf", std::ios::in);

	if (file.is_open() == false) {
		std::cout << "test_main: file open error" << std::endl;
		return;
	}
	std::stringstream ss;
	ss << file.rdbuf();
	if (ss.fail()) {
		std::cout << "test_main: file read error" << std::endl;
		file.close();
		return;
	}
	file.close();

	total_port_server_map_p org_config;
	std::cout << ss.str() << std::endl;

	if (spx_config_syntax_checker(ss.str(), org_config) != spx_ok) {
		std::cout << "test_main: error" << std::endl;
	} else {
		std::cout << "test_main: success" << std::endl;
	}
}

int
main() {
#ifdef REAK
	atexit(ft_handler);
#endif

	inner_main();

	return 0;
}

// -- --
// #include "config.hpp"
// #include <cstdlib>
// #include <iostream>

// /* HOW TO USE*/
// // c++ -D DEBUG *.cpp && ./a.out
// // c++ -D DEBUG -D REAK *.cpp && ./a.out

// #ifdef REAK
// void
// ft_handler() {
// 	system("leaks a.out");
// }
// #endif

// server_info_for_copy_stage_t
// generator(std::string plus_name) {
// 	server_info_for_copy_stage_t server_info_copy;

// 	server_info_copy.ip					  = "127.0.0.1";
// 	server_info_copy.port				  = 80;
// 	server_info_copy.default_server_flag  = default_server;
// 	server_info_copy.server_name		  = plus_name;
// 	server_info_copy.error_page			  = "error_page.html";
// 	server_info_copy.client_max_body_size = 1024;

// 	uri_location_for_copy_stage_t location;
// 	location.module_state		   = module_serve;
// 	location.accepted_methods_flag = KGet | KPost;
// 	location.redirect			   = "github.com";
// 	location.root				   = "/home/username";
// 	location.index				   = "index.html";
// 	location.autoindex_flag		   = Kautoindex_on;
// 	location.saved_path			   = "/home/username/saved";
// 	location.cgi_pass			   = "/home/username/cgi";
// 	location.cgi_path_info		   = "/home/username/cgi_path_info";

// 	uri_location_for_copy_stage_t location2;
// 	location2.module_state			= module_upload;
// 	location2.accepted_methods_flag = KHead;
// 	location2.redirect				= "hi.com";
// 	location2.root					= "test";
// 	location2.index					= "test";
// 	location2.autoindex_flag		= Kautoindex_off;
// 	location2.saved_path			= "test";
// 	location2.cgi_pass				= "test";
// 	location2.cgi_path_info			= "test";

// 	server_info_copy.uri_case.insert(std::pair<const std::string, const uri_location_t>("/first", location));
// 	server_info_copy.uri_case.insert(std::pair<const std::string, const uri_location_t>("/second", location2));

// 	return server_info_copy;
// }

// int
// main() {
// #ifdef REAK
// 	atexit(ft_handler);
// #endif

// 	std::string	  name1("spaceX");
// 	std::string	  name2("spaceX2");
// 	server_info_t test(generator(name1));
// 	server_info_t test2(generator(name2));

// 	// test.print();

// 	std::map<std::string, server_info_t> test_map;
// 	test_map.insert(std::pair<std::string, server_info_t>(test.server_name, test));
// 	test_map.insert(std::pair<std::string, server_info_t>(test2.server_name, test2));

// 	std::map<std::string, server_info_t>::iterator it = test_map.find(name1);
// 	if (it != test_map.end()) {
// 		std::cout << "find " << name1 << "------------------" << std::endl;
// 		it->second.print();
// 	}

// 	it = test_map.find(name2);
// 	if (it != test_map.end()) {
// 		std::cout << "find " << name2 << "------------------" << std::endl;
// 		it->second.print();
// 	}

// 	it = test_map.find("third");
// 	if (it != test_map.end()) {
// 		std::cout << "find third ------------------" << std::endl;
// 		it->second.print();
// 	}

// 	server_info_t* default_server_ptr = NULL; // default server ptr;

// 	it = test_map.begin();
// 	while (it != test_map.end()) {
// 		if (it->second.default_server_flag & default_server) {
// 			default_server_ptr = &it->second;
// 			break;
// 		}
// 		it++;
// 	}

// 	if (default_server_ptr) {
// 		std::cout << "find default ------------------" << std::endl;
// 		default_server_ptr->print();
// 		default_server_ptr->uri_case.find("/first")->second.print();
// 	}

// 	return 0;
// }
