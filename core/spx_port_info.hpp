#pragma once
#ifndef __SPX__PORT__INFO__HPP__
#define __SPX__PORT__INFO__HPP__

#include "spx_core_type.hpp"
#include <map>
#include <string>

typedef enum {
	Kother_server = 0,
	Kdefault_server
} default_server_state_e;

typedef enum {
	Kautoindex_off = 0,
	Kautoindex_on
} autoindex_state_e;

typedef enum {
	module_none = 0,
	module_serve,
	module_upload,
	module_redirect,
	module_cgi
} module_case_state_e;

typedef enum {
	KGet	 = 1 << 1,
	KPost	 = 1 << 2,
	KPut	 = 1 << 3,
	KDelete	 = 1 << 4,
	KHead	 = 1 << 5,
	KOptions = 1 << 6
} accepted_method_flag_e;

enum {
	KSame = 0,
	KDiff
};

typedef enum {
	flag_listen				  = 1 << 1,
	flag_server_name		  = 1 << 2,
	flag_error_page			  = 1 << 3,
	flag_client_max_body_size = 1 << 4,
	flag_root				  = 1 << 5
} flag_config_parse_basic_part_e;

typedef enum {
	flag_accepted_methods = 1 << 1,
	flag_index			  = 1 << 2,
	flag_autoindex		  = 1 << 3,
	flag_redirect		  = 1 << 4,
	flag_saved_path		  = 1 << 6,
	flag_cgi_pass		  = 1 << 7,
	flag_cgi_path_info	  = 1 << 8,
} flag_config_parse_location_part_e;

/*
 * --------------------
 */
typedef struct uri_location_for_copy_stage			uri_location_for_copy_stage_t;
typedef struct server_info_for_copy_stage			server_info_for_copy_stage_t;
typedef struct server_info							server_info_t;
typedef struct uri_location							uri_location_t;
typedef std::map<const std::string, uri_location_t> uri_location_map_p;
typedef std::map<const uint32_t, const std::string> error_page_map_p;
typedef std::map<const std::string, server_info_t>	server_map_p;
typedef std::map<const uint32_t, server_map_p>		total_port_server_map_p;

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
	void				clear();
} uri_location_for_copy_stage_t;

typedef struct server_info_for_copy_stage {
	std::string			   ip;
	uint32_t			   port;
	default_server_state_e default_server_flag;
	std::string			   server_name;
	std::string			   root;
	uint64_t			   client_max_body_size;
	std::string			   default_error_page;
	error_page_map_p	   error_page_case;
	uri_location_map_p	   uri_case;
	void				   clear();
	void				   print() const;
} server_info_for_copy_stage_t;

typedef struct uri_location {
	const std::string		  uri;
	const module_case_state_e module_state;
	const uint8_t			  accepted_methods_flag;
	const std::string		  redirect;
	const std::string		  root;
	const std::string		  index;
	const autoindex_state_e	  autoindex_flag;
	const std::string		  saved_path;
	const std::string		  cgi_pass;
	const std::string		  cgi_path_info;
	//
	uri_location(const uri_location_for_copy_stage_t from);
	~uri_location();
	//
	void print(void) const;

} uri_location_t;

typedef struct server_info {
	const std::string			 ip;
	const uint32_t				 port;
	const default_server_state_e default_server_flag;
	const std::string			 server_name;
	const std::string			 root;
	const uint64_t				 client_max_body_size;
	const std::string			 default_error_page;
	mutable error_page_map_p	 error_page_case;
	mutable uri_location_map_p	 uri_case;
	//
	server_info(server_info_for_copy_stage_t const& from);
	server_info(server_info_t const& from);
	// server_info& operator=(server_info_t const& from);
	~server_info();
	// add uri value get function
	void print(void) const;

} server_info_t;

/*
 * --------------------
 */

// typedef std::map<const std::string, const uri_location_t> uri_location_map_p;
// typedef std::map<const std::string, server_info_t>  server_map_p;
// typedef std::map<const std::uint32_t, server_map_p> total_port_server_map_p;

// < port_number , < server_name, serve_map_p> > my_config_map;
// < port_number , < server_name, < uri_location, uri_location_t> > > my_config_map;

#endif
