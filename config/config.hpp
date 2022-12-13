#pragma once
#ifndef __CONFIG__HPP__
#define __CONFIG__HPP__

#include "port_info.hpp"
#include "spacex_type.hpp"
#include <map>
#include <string>

#include <vector> // need to check what data type is used in buffer
/*
 * --------------------
 */

struct uri_location_for_copy_stage {
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
	uint32_t			   client_max_body_size;
	uri_location_map_p	   uri_case;
};

typedef std::map<const std::uint32_t, const server_map_p> total_port_server_map_p;
// target_port_server_map_p								  my_config_map;

#endif
