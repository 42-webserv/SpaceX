#include "config.hpp"

namespace {

}

status
spx_config_syntax_checker(std::vector<char> const& buf,
						  total_port_server_map_p& config_map) {
	total_port_server_map_p::iterator total_it = config_map.begin();
	server_map_p::iterator			  per_port_server_it;
	uri_location_map_p::iterator	  per_uri_location_it;

	enum {
		start,
		server,
		location,
		root,
		index,
		autoindex,
		error_page,
		client_max_body_size,
		cgi_pass,
		cgi_path_info,
		redirect,
		accepted_method,
		saved_path,
		module
	} state;

	return spx_ok;
}
