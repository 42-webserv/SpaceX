#include "config.hpp"
#include "spacex_type.hpp"

#include <vector>

namespace {

}

// std::map<std::string, t_server_info>		server_map;
// t_server_info* 							default_server_ptr; // default server ptr;
status
spx_config_syntax_checker(std::vector<char> const&							buf,
						  std::map<const std::string, const t_server_info>& server_map,
						  t_server_info*									default_server_ptr) {

	t_server_info_copy server_info_temp_copy;
	t_location		   location_temp;
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
