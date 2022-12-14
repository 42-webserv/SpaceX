#pragma once
#ifndef __CONFIG__HPP__
#define __CONFIG__HPP__

#include "port_info.hpp"
#include "spacex_type.hpp"
#include <map>
#include <string>

/*
 * --------------------
 */

#define CONFIG_STATE_MAP(XX)                               \
	XX(0, ZERO, Continue)                                  \
	XX(1, START, skip space)                               \
	XX(2, ENDLINE, check endline)                          \
	XX(3, WAITING_DEFAULT_VALUE, waiting default value)    \
	XX(4, SERVER, server position found)                   \
	XX(5, SERVER_OPEN, server curly bracket open)          \
	XX(6, SERVER_CLOSE, server curly bracket close)        \
	XX(7, LISTEN, listen found)                            \
	XX(8, LISTEN_DEFAULT, default_server)                  \
	XX(9, SERVER_NAME, server_name)                        \
	XX(10, ERROR_PAGE, error_page)                         \
	XX(11, CLIENT_MAX_BODY_SIZE, client_max_body_size)     \
	XX(12, WAITING_LOCATION_VALUE, waiting location value) \
	XX(13, LOCATION_ZERO, location flush)                  \
	XX(14, LOCATION_URI, location uri)                     \
	XX(15, LOCATION_OPEN, location curly bracket open)     \
	XX(16, LOCATION_CLOSE, location curly bracket close)   \
	XX(17, ACCEPTED_METHODS, accepted_methods)             \
	XX(18, ROOT, root)                                     \
	XX(19, INDEX, index)                                   \
	XX(20, AUTOINDEX, autoindex)                           \
	XX(21, REDIRECT, redirect)                             \
	XX(22, SAVED_PATH, saved_path)                         \
	XX(23, CGI_PASS, cgi_pass)                             \
	XX(24, CGI_PATH_INFO, cgi_path_info)                   \
	XX(25, ALMOST_DONE, almost done)                       \
	XX(26, DONE, done)

typedef enum config_state {
#define XX(num, name, string) CONFIG_STATE_##name = num,
	CONFIG_STATE_MAP(XX)
#undef XX
} config_state_e;

/*
 * --------------------
 */

enum {
	KSame = 0,
	KDiff
};

typedef enum {
	flag_listen				  = 1 << 1,
	flag_server_name		  = 1 << 2,
	flag_error_page			  = 1 << 3,
	flag_client_max_body_size = 1 << 4
} flag_config_parse_basic_part_e;

typedef enum {
	flag_accepted_methods = 1 << 0,
	flag_root			  = 1 << 1,
	flag_index			  = 1 << 2,
	flag_autoindex		  = 1 << 3,
	flag_redirect		  = 1 << 4,
	flag_saved_path		  = 1 << 5,
	flag_cgi_pass		  = 1 << 6,
	flag_cgi_path_info	  = 1 << 7
} flag_config_parse_location_part_e;

/*
 * --------------------
 */

struct uri_location_for_copy_stage {
	std::string			uri;
	module_case_state_e module_state;
	uint8_t				accepted_methods_flag;
	std::string			redirect;
	std::string			root;
	std::string			index;
	autoindex_state_e	autoindex_flag;
	std::string			saved_path;
	std::string			cgi_pass;
	std::string			cgi_path_info;
};

struct server_info_for_copy_stage {
	std::string			   ip;
	uint32_t			   port;
	default_server_state_e default_server_flag;
	std::string			   server_name;
	std::string			   error_page;
	uint64_t			   client_max_body_size;
	error_page_map_p	   error_page_case;
	uri_location_map_p	   uri_case;
};

typedef std::map<const uint32_t, server_map_p> total_port_server_map_p;
// < port_number , < server_name, server_info_t> > my_config_map;
// < port_number , < server_name, < uri_location, uri_location_t> > > my_config_map;

status
spx_config_syntax_checker(std::string const&	   buf,
						  total_port_server_map_p& config_map);

#endif
