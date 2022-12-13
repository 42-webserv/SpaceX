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
	flag_accepted_method = 1 << 0,
	flag_root			 = 1 << 1,
	flag_index			 = 1 << 2,
	flag_autoindex		 = 1 << 3,
	flag_redirect		 = 1 << 4,
	flag_saved_path		 = 1 << 5,
	flag_cgi_pass		 = 1 << 6,
	flag_cgi_path_info	 = 1 << 7
} flag_config_parse_location_part_e;

/*
 * --------------------
 */

struct uri_location_for_copy_stage {
	std::string			uri;
	module_case_state_e module_state;
	uint8_t				accepted_method_flag;
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
	uri_location_map_p	   uri_case;
};

typedef std::map<const std::uint32_t, const server_map_p> total_port_server_map_p;
// target_port_server_map_p								  my_config_map;
// < port_number , < server_name, server_info_t> > my_config_map;

#endif
