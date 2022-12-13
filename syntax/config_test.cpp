#include "config.hpp"
#include <cstdlib>
#include <iostream>

/* HOW TO USE*/
// c++ -D DEBUG config_test.cpp && ./a.out
// c++ -D DEBUG -D REAK config_test.cpp && ./a.out

#ifdef REAK
void
ft_handler() {
	system("leaks a.out");
}
#endif

t_server_info_copy
generator(std::string plus_name) {
	t_server_info_copy server_info_copy;

	server_info_copy.ip					  = "127.0.0.1";
	server_info_copy.port				  = 80;
	server_info_copy.default_server_flag  = default_server;
	server_info_copy.server_name		  = plus_name;
	server_info_copy.error_page			  = "error_page.html";
	server_info_copy.client_max_body_size = 1024;

	t_location location;
	location.module_state		  = module_serve;
	location.accepted_method_flag = GET | POST;
	location.redirect			  = "github.com";
	location.root				  = "/home/username";
	location.index				  = "index.html";
	location.autoindex_flag		  = autoindex_on;
	location.saved_path			  = "/home/username/saved";
	location.cgi_pass			  = "/home/username/cgi";
	location.cgi_path_info		  = "/home/username/cgi_path_info";

	t_location location2;
	location2.module_state		   = module_upload;
	location2.accepted_method_flag = HEAD;
	location2.redirect			   = "hi.com";
	location2.root				   = "test";
	location2.index				   = "test";
	location2.autoindex_flag	   = autoindex_off;
	location2.saved_path		   = "test";
	location2.cgi_pass			   = "test";
	location2.cgi_path_info		   = "test";

	server_info_copy.uri_case.insert(std::pair<const std::string, const t_location>("/first", location));
	server_info_copy.uri_case.insert(std::pair<const std::string, const t_location>("/second", location2));

	return server_info_copy;
}

int
main() {
#ifdef REAK
	atexit(ft_handler);
#endif

	std::string	  name1("spaceX");
	std::string	  name2("spaceX2");
	t_server_info test(generator(name1));
	t_server_info test2(generator(name2));

	// test.print();

	std::map<std::string, t_server_info> test_map;
	test_map.insert(std::pair<std::string, t_server_info>(test.get_server_name(), test));
	test_map.insert(std::pair<std::string, t_server_info>(test2.get_server_name(), test2));

	std::map<std::string, t_server_info>::iterator it = test_map.find(name1);
	if (it != test_map.end()) {
		std::cout << "find " << name1 << "------------------" << std::endl;
		it->second.print();
	}

	it = test_map.find(name2);
	if (it != test_map.end()) {
		std::cout << "find " << name2 << "------------------" << std::endl;
		it->second.print();
	}

	it = test_map.find("third");
	if (it != test_map.end()) {
		std::cout << "find third ------------------" << std::endl;
		it->second.print();
	}

	t_server_info* default_server_ptr; // default server ptr;

	it = test_map.begin();
	while (it != test_map.end()) {
		if (it->second.is_default_server() == default_server) {
			default_server_ptr = &it->second;
			break;
		}
		it++;
	}

	if (default_server_ptr) {
		std::cout << "find default ------------------" << std::endl;
		default_server_ptr->print();
	}

	return 0;
}
