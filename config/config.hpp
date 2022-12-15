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

typedef struct uri_location_for_copy_stage {
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

} uri_location_for_copy_stage_t;

typedef struct server_info_for_copy_stage {
	std::string			   ip;
	uint32_t			   port;
	default_server_state_e default_server_flag;
	std::string			   server_name;
	uint64_t			   client_max_body_size;
	std::string			   default_error_page;
	error_page_map_p	   error_page_case;
	uri_location_map_p	   uri_case;
} server_info_for_copy_stage_t;

typedef std::map<const uint32_t, server_map_p> total_port_server_map_p;
// < port_number , < server_name, server_info_t> > my_config_map;
// < port_number , < server_name, < uri_location, uri_location_t> > > my_config_map;

status
spx_config_syntax_checker(std::string const&	   buf,
						  total_port_server_map_p& config_map);

#endif
