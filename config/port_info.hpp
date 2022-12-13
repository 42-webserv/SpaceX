#pragma once
#ifndef __PORT__INFO__HPP__
#define __PORT__INFO__HPP__

#include "spacex_type.hpp"
#include <map>
#include <string>

typedef enum {
	other_server = 0,
	default_server
} default_server_state_e;

typedef enum {
	autoindex_on = 0,
	autoindex_off
} autoindex_state_e;

typedef enum {
	module_none = 0,
	module_serve,
	module_upload,
	module_redirect,
	module_cgi
} module_case_state_e;

typedef enum {
	GET		= 1 << 1,
	POST	= 1 << 2,
	PUT		= 1 << 3,
	DELETE	= 1 << 4,
	HEAD	= 1 << 5,
	OPTIONS = 1 << 6
} accepted_method_flag_e;

typedef struct uri_location_for_copy_stage uri_location_for_copy_stage_t;
typedef struct server_info_for_copy_stage  server_info_for_copy_stage_t;

typedef struct uri_location {
	const module_case_state_e module_state;
	const uint8_t			  accepted_method_flag;
	const std::string		  redirect;
	const std::string		  root;
	const std::string		  index;
	const autoindex_state_e	  autoindex_flag;
	const std::string		  saved_path;
	const std::string		  cgi_pass;
	const std::string		  cgi_path_info;
	//
	uri_location(uri_location_for_copy_stage_t const& from);
	~uri_location();
	//
	void print(void) const;

} uri_location_t;

typedef std::map<const std::string, const uri_location_t> uri_location_map_p;
typedef struct server_info {
	const std::string			 ip;
	const uint32_t				 port;
	const default_server_state_e default_server_flag;
	const std::string			 server_name;
	const std::string			 error_page;
	const uint32_t				 client_max_body_size;
	mutable uri_location_map_p	 uri_case;
	//
	server_info(server_info_for_copy_stage_t const& from);
	~server_info();
	// add uri value get function
	void print(void) const;

} server_info_t;

/*
 * --------------------
 */
typedef std::map<const std::string, const server_info_t> server_map_p;
typedef struct target_port_server_info {
	server_info_t* my_port_default_server_ptr;
	server_map_p   my_port_map;
	// add handler function

} target_port_server_info_t;

#endif
