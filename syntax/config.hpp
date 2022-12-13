#pragma once
#ifndef __CONFIG__HPP__
#define __CONFIG__HPP__

#include "spacex_type.hpp"
#include <iostream>
#include <map>
#include <string>

typedef enum {
	other_server = 0,
	default_server
} t_default_server_state;

typedef enum {
	autoindex_on = 0,
	autoindex_off
} t_autoindex_state;

typedef enum {
	module_none = 0,
	module_serve,
	module_upload,
	module_redirect,
	module_cgi
} t_module_case_state;

typedef enum {
	GET		= 1 << 1,
	POST	= 1 << 2,
	PUT		= 1 << 3,
	DELETE	= 1 << 4,
	HEAD	= 1 << 5,
	OPTIONS = 1 << 6
} t_accepted_method_flag;

typedef struct server_location {
	t_module_case_state module_state;
	uint8_t				accepted_method_flag;
	std::string			redirect;
	std::string			root;
	std::string			index;
	t_autoindex_state	autoindex_flag;
	std::string			saved_path;
	std::string			cgi_pass;
	std::string			cgi_path_info;
} t_location;

typedef struct server_info_copy {
	std::string									  ip;
	uint32_t									  port;
	t_default_server_state						  default_server_flag;
	std::string									  server_name;
	std::string									  error_page;
	uint32_t									  client_max_body_size;
	std::map<const std::string, const t_location> uri_case;

} t_server_info_copy;

typedef struct server_info {
private:
	const std::string							  ip;
	const uint32_t								  port;
	const t_default_server_state				  default_server_flag;
	const std::string							  server_name;
	const std::string							  error_page;
	const uint32_t								  client_max_body_size;
	std::map<const std::string, const t_location> uri_case;

public:
	server_info(t_server_info_copy const& from)
		: ip(from.ip)
		, port(from.port)
		, default_server_flag(from.default_server_flag)
		, server_name(from.server_name)
		, error_page(from.error_page)
		, client_max_body_size(from.client_max_body_size)
		, uri_case(from.uri_case) {
	}

	~server_info() {
	}

	t_default_server_state
	is_default_server(void) const {
		return default_server_flag;
	}

	const std::string&
	get_server_name(void) const {
		return server_name;
	}

	inline void
	print(void) {
#ifdef DEBUG
		std::cout
			<< "ip: " << ip << std::endl;
		std::cout << "port: " << port << std::endl;
		std::cout << "default_server_flag: " << default_server_flag << std::endl;
		std::cout << "server_name: " << server_name << std::endl;
		std::cout << "error_page: " << error_page << std::endl;
		std::cout << "client_max_body_size: " << client_max_body_size << std::endl;
		std::cout << "uri_case: " << uri_case.size() << std::endl;
		std::cout << std::endl;

		std::map<const std::string, const t_location>::iterator it = uri_case.begin();
		while (it != uri_case.end()) {
			std::cout << "uri: " << it->first << std::endl;
			std::cout << "t_location: " << &it->second << std::endl;
			std::cout << "module_state: " << it->second.module_state << std::endl;
			std::cout << "accepted_method_flag: " << it->second.accepted_method_flag << std::endl;
			std::cout << "redirect: " << it->second.redirect << std::endl;
			std::cout << "root: " << it->second.root << std::endl;
			std::cout << "index: " << it->second.index << std::endl;
			std::cout << "autoindex_flag: " << it->second.autoindex_flag << std::endl;
			std::cout << "saved_path: " << it->second.saved_path << std::endl;
			std::cout << "cgi_pass: " << it->second.cgi_pass << std::endl;
			std::cout << "cgi_path_info: " << it->second.cgi_path_info << std::endl;
			std::cout << std::endl;
			it++;
		}
#endif
	}

} t_server_info;

// std::map<std::string, t_server_info>		server_map;
// t_server_info* 							default_server_ptr; // default server ptr;

status
spx_config_syntax_server();

#endif
