#pragma once
#ifndef __SPX__PORT__INFO__HPP__
#define __SPX__PORT__INFO__HPP__

#include "spx_core_type.hpp"
#include "spx_core_util_box.hpp"

#include <arpa/inet.h>
#include <sys/event.h>
#include <sys/socket.h>

#include <fcntl.h>
#include <unistd.h>

typedef enum {
	Kother_server = 0,
	Kdefault_server
} default_server_state_e;

typedef enum {
	Kautoindex_off = 0,
	Kautoindex_on
} autoindex_state_e;

typedef enum {
	Kmodule_none	 = 1 << 0,
	Kmodule_serve	 = 1 << 1,
	Kmodule_upload	 = 1 << 2,
	Kmodule_redirect = 1 << 3,
	Kmodule_cgi		 = 1 << 4
} module_case_state_e;

typedef enum {
	KGet	= 1 << 1,
	KPost	= 1 << 2,
	KPut	= 1 << 3,
	KDelete = 1 << 4,
	KHead	= 1 << 5
} accepted_method_flag_e;

enum {
	KSame = 0,
	KDiff
};

typedef enum {
	Kflag_root		  = 1 << 0,
	Kflag_listen	  = 1 << 1,
	Kflag_server_name = 1 << 2,
	Kflag_error_page  = 1 << 3,
	Kflag_root_slash  = 1 << 4
} flag_config_parse_basic_part_e;

typedef enum {
	Kflag_accepted_methods	   = 1 << 1,
	Kflag_index				   = 1 << 2,
	Kflag_autoindex			   = 1 << 3,
	Kflag_redirect			   = 1 << 4,
	Kflag_saved_path		   = 1 << 5,
	Kflag_cgi_pass			   = 1 << 6,
	Kflag_cgi_path_info		   = 1 << 7,
	Kflag_client_max_body_size = 1 << 8
} flag_config_parse_location_part_e;

/* NOTE : typedef
 * --------------------
 */
typedef struct uri_location_for_copy_stage			uri_location_for_copy_stage_t;
typedef struct server_info_for_copy_stage			server_info_for_copy_stage_t;
typedef struct server_info							server_info_t;
typedef struct uri_location							uri_location_t;
typedef std::map<const std::string, uri_location_t> cgi_list_map_p;
typedef std::map<const std::string, uri_location_t> uri_location_map_p;
typedef std::map<const uint32_t, const std::string> error_page_map_p;
typedef std::map<const std::string, server_info_t>	server_map_p;
typedef std::map<const uint32_t, server_map_p>		total_port_server_map_p;

typedef struct uri_location_for_copy_stage {
	std::string			uri;
	module_case_state_e module_state;
	uint16_t			accepted_methods_flag;
	std::string			redirect;
	std::string			root;
	std::string			index;
	autoindex_state_e	autoindex_flag;
	std::string			saved_path;
	std::string			cgi_pass;
	std::string			cgi_path_info;
	uint64_t			client_max_body_size;
	void				clear_();
} uri_location_for_copy_stage_t;

typedef struct server_info_for_copy_stage {
	std::string			   ip;
	uint32_t			   port;
	default_server_state_e default_server_flag;
	std::string			   server_name;
	std::string			   root;
	std::string			   default_error_page;
	error_page_map_p	   error_page_case;
	uri_location_map_p	   uri_case;
	cgi_list_map_p		   cgi_case;
	void				   clear_();
	void				   print_() const;
} server_info_for_copy_stage_t;

/* NOTE: location_info_t
 * --------------------
 */

typedef struct uri_location {
	const std::string		  uri;
	const module_case_state_e module_state;
	const uint16_t			  accepted_methods_flag;
	const std::string		  redirect;
	const std::string		  root;
	const std::string		  index;
	const autoindex_state_e	  autoindex_flag;
	const std::string		  saved_path;
	const std::string		  cgi_pass;
	const std::string		  cgi_path_info;
	const uint64_t			  client_max_body_size;

	uri_location(const uri_location_for_copy_stage_t from);
	~uri_location();
	void print_(void) const;

} uri_location_t;

/* NOTE: server_info_t
 * --------------------
 */

typedef enum {
	Kuri_notfound_uri	 = 1 << 0,
	Kuri_depth_uri		 = 1 << 1,
	Kuri_check_extension = 1 << 2,
	Kuri_cgi			 = 1 << 3,
	Kuri_path_info		 = 1 << 4,
	Kuri_cgi_pass		 = 1 << 5
} uri_flag_e;

typedef struct uri_resolved {
	bool			is_cgi_;
	uri_location_t* cgi_loc_;
	std::string		cgi_path_info_;
	std::string		request_uri_;
	std::string		resolved_request_uri_;
	std::string		script_name_;
	std::string		script_filename_;
	std::string		path_info_;
	std::string		path_translated_;
	std::string		query_string_;
	std::string		fragment_;
	void			print_(void) const;
} uri_resolved_t;

typedef struct server_info {
	const std::string			 ip;
	const uint32_t				 port;
	const default_server_state_e default_server_flag;
	const std::string			 server_name;
	const std::string			 root;
	const std::string			 default_error_page;
	mutable error_page_map_p	 error_page_case;
	mutable uri_location_map_p	 uri_case;
	mutable cgi_list_map_p		 cgi_case;
	//
	server_info(server_info_for_copy_stage_t const& from);
	server_info(server_info_t const& from);
	~server_info();

	std::string const		 get_error_page_path_(uint32_t const& error_code) const;
	uri_location_t const*	 get_uri_location_t_(std::string const& uri, uri_resolved_t& uri_resolved_sets, int request_method) const;
	static std::string const path_resolve_(std::string const& unvalid_path);
	void					 print_(void) const;

} server_info_t;

/* NOTE: port_info_t
 * --------------------
 */

#ifndef LISTEN_BACKLOG_SIZE
#define LISTEN_BACKLOG_SIZE 1024
#endif

typedef struct port_info {
	int				   listen_sd;
	struct sockaddr_in addr_server;
	uint32_t		   my_port;
	server_info_t	   my_port_default_server;
	server_map_p	   my_port_map;
	//---------------------
	port_info(server_info_t const& from);
	server_info_t const& search_server_config_(std::string const& host_name);

} port_info_t;

typedef std::vector<port_info_t> port_info_vec;

status
socket_init_and_build_port_info(total_port_server_map_p& config_info,
								port_info_vec&			 port_info,
								int64_t&				 socket_size,
								int64_t&				 first_socket);

int socket_init(total_port_server_map_p const& config_info);

// e.g. std::map<const std::string, const uri_location_t> uri_location_map_p;
// e.g. std::map<const std::string, server_info_t>  server_map_p;
// e.g. std::map<const std::uint32_t, server_map_p> total_port_server_map_p;

// < port_number , < server_name, serve_map_p> > my_config_map;
// < port_number , < server_name, < uri_location, uri_location_t> > > my_config_map;

#endif
