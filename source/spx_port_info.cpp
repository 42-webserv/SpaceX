#include "spx_port_info.hpp"
#include "spx_core_type.hpp"
#include "spx_core_util_box.hpp"
#include "spx_parse_config.hpp"
#include <cstddef>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace {

} // namespace

server_info_t::server_info(server_info_for_copy_stage_t const& from)
	: ip(from.ip)
	, port(from.port)
	, default_server_flag(from.default_server_flag)
	, server_name(from.server_name)
	, root(from.root)
	, default_error_page(from.default_error_page) {
#ifdef CONFIG_DEBUG

	std::cout << "server_info copy construct" << std::endl;
#endif
	if (from.error_page_case.size() != 0) {
		error_page_case.insert(from.error_page_case.begin(), from.error_page_case.end());
	}
	if (from.uri_case.size() != 0) {
		uri_case.insert(from.uri_case.begin(), from.uri_case.end());
	}
	if (from.cgi_case.size() != 0) {
		cgi_case.insert(from.cgi_case.begin(), from.cgi_case.end());
	}
}

server_info_t::server_info(server_info_t const& from)
	: ip(from.ip)
	, port(from.port)
	, default_server_flag(from.default_server_flag)
	, server_name(from.server_name)
	, root(from.root)
	, default_error_page(from.default_error_page) {
#ifdef CONFIG_DEBUG

	std::cout << "server_info copy construct" << std::endl;
#endif
	if (from.error_page_case.size() != 0) {
		error_page_case.insert(from.error_page_case.begin(), from.error_page_case.end());
	}
	if (from.uri_case.size() != 0) {
		uri_case.insert(from.uri_case.begin(), from.uri_case.end());
	}
	if (from.cgi_case.size() != 0) {
		cgi_case.insert(from.cgi_case.begin(), from.cgi_case.end());
	}
}

server_info_t::~server_info() {
}

std::string const
server_info_t::get_error_page_path_(uint32_t const& error_code) const {
	error_page_map_p::const_iterator it = error_page_case.find(error_code);

	if (it != error_page_case.end()) {
		return path_resolve_(it->second);
	}
	return path_resolve_(this->default_error_page);
}

uri_location_t const*
server_info_t::get_uri_location_t_(std::string const& uri,
								   uri_resolved_t&	  uri_resolved_sets) const {
	uri_location_t*				return_location = NULL;
	std::string					temp;
	std::string					temp_root;
	std::string					temp_location;
	std::string					temp_extension;
	std::string					temp_index;
	uint8_t						flag_check_dup = 0;
	uint8_t						root_uri_flag  = 0;
	std::string::const_iterator it			   = uri.begin();

	uri_resolved_sets.is_cgi_			= false;
	uri_resolved_sets.is_same_location_ = false;
	uri_resolved_sets.cgi_loc_			= NULL;
	uri_resolved_sets.request_uri_		= uri;

	enum {
		uri_main, // 0
		uri_remain,
		uri_cgi,
		uri_query,
		uri_fragment,
		uri_find_delimeter_case, // 5
		uri_done,
		uri_end
	} state,
		next_state;

	state	   = uri_main;
	next_state = uri_find_delimeter_case;

	while (state != uri_end) {
		switch (state) {
		case uri_main: {
			while (*it == '/') {
				++it;
			}
			temp += "/";
			while (syntax_(usual_for_uri_parse_, static_cast<uint8_t>(*it))) {
				temp += *it;
				++it;
			}
			uri_location_map_p::iterator it_ = uri_case.find(temp);
			if (it_ != uri_case.end()) {
				return_location = &it_->second;
				temp_index		= it_->second.index;
				temp_root		= it_->second.root;
				temp_location	= it_->second.uri;
				flag_check_dup |= Kuri_same_uri;
			} else {
				it_ = uri_case.find("/");
				if (it_ != uri_case.end()) {
					return_location = &it_->second;
					root_uri_flag |= 1;
				}
				// 	temp_index		= it_->second.index;
				// 	temp_root		= it_->second.root;
				// 	temp_location	= it_->second.uri;
				temp_root = this->root;
				uri_resolved_sets.script_name_ += temp;
				// }
			}
			// uri_resolved_sets.script_name_ += temp; // NOTE: different from nginx
			temp.clear();
			state = next_state;
			break;
		}

		case uri_find_delimeter_case: {
			switch (*it) {
			case '?': {
				++it;
				state = uri_query;
				break;
			}
			case '#': {
				++it;
				state = uri_fragment;
				break;
			}
			case '.': {
				state = uri_cgi;
				break;
			}
			case '/': {
				while (*it == '/') {
					++it;
				}
				temp += "/";
				state = uri_remain;
				break;
			}
			default: {
				state = uri_done;
				break;
			}
			}
			break;
		}

		case uri_remain: {
			while (syntax_(usual_for_uri_parse_, static_cast<uint8_t>(*it))) {
				temp += *it;
				++it;
			}
			if (!(flag_check_dup & Kuri_cgi)) {
				flag_check_dup &= ~Kuri_same_uri;
				uri_resolved_sets.script_name_ += temp;
			} else {
				flag_check_dup |= Kuri_path_info;
				uri_resolved_sets.path_info_ += temp;
				while (syntax_(except_query_fragment_, static_cast<uint8_t>(*it))) {
					temp_extension += *it;
					++it;
				}
				uri_resolved_sets.path_info_ += temp_extension;
				temp_extension.clear();
			}
			temp.clear();
			state = uri_find_delimeter_case;
			break;
		}

		case uri_cgi: {
			while (syntax_(except_slash_query_fragment_, static_cast<uint8_t>(*it))) {
				temp_extension += *it;
				++it;
			}
			if (flag_check_dup & Kuri_cgi) {
				flag_check_dup |= Kuri_path_info;
				uri_resolved_sets.path_info_ += temp_extension;
			} else {
				std::string				 check_ext = temp_extension.substr(temp_extension.find_last_of("."));
				cgi_list_map_p::iterator it_	   = cgi_case.find(check_ext);
				if (it_ != cgi_case.end()) {
					uri_resolved_sets.is_cgi_  = true;
					uri_resolved_sets.cgi_loc_ = &it_->second;
					flag_check_dup |= Kuri_cgi;
				}
				uri_resolved_sets.script_name_ += temp_extension;
			}
			temp_extension.clear();
			state = uri_find_delimeter_case;
			break;
		}

		case uri_query: {
			while (it != uri.end() && *it != '#') {
				uri_resolved_sets.query_string_ += *it;
				++it;
			}
			if (*it == '#') {
				state = uri_fragment;
			} else {
				state = uri_done;
			}
			break;
		}

		case uri_fragment: { // NOTE : fragment is not used in this project. just passing
			while (it != uri.end()) {
				// uri_resolved_sets.fragment_ += *it;
				++it;
			}
			state = uri_done;
			break;
		}

		case uri_done: {
			// XXX : if set saved_path, then use saved_path // not recommend this time
			// TODO: cgi's saved_path isn't defined yet
			if (flag_check_dup & Kuri_path_info) {
				if (root_uri_flag & 1) {
					return_location = NULL;
				}
				uri_resolved_sets.script_filename_ = path_resolve_(temp_root + "/" + uri_resolved_sets.script_name_);
				uri_resolved_sets.script_name_	   = path_resolve_(temp_location + uri_resolved_sets.script_name_);
			} else if (temp_index.empty()) {
				uri_resolved_sets.script_filename_ = path_resolve_(temp_root + "/" + uri_resolved_sets.script_name_);
				uri_resolved_sets.script_name_	   = path_resolve_(temp_location + uri_resolved_sets.script_name_);
			} else {
				uri_resolved_sets.script_filename_ = path_resolve_(temp_root + "/" + uri_resolved_sets.script_name_ + "/" + temp_index);
				uri_resolved_sets.script_name_	   = path_resolve_(temp_location + uri_resolved_sets.script_name_ + "/" + temp_index);
			}
			uri_resolved_sets.path_info_			= path_resolve_(uri_resolved_sets.path_info_);
			uri_resolved_sets.resolved_request_uri_ = uri_resolved_sets.script_name_ + uri_resolved_sets.path_info_;
			if (uri_resolved_sets.path_info_.empty() == false) {
				uri_resolved_sets.path_translated_ = path_resolve_(temp_root + "/" + uri_resolved_sets.path_info_);
			}
			if (flag_check_dup & Kuri_same_uri && !(flag_check_dup & (Kuri_cgi | Kuri_path_info))) {
				uri_resolved_sets.is_same_location_ = true;
			}
			state = uri_end;
			break;
		}
		case uri_end: {
			break;
		}
		} // switch end
	} // while end
	return return_location;
}

// uri_location_t const*
// server_info_t::get_uri_location_t_(std::string const& uri,
// 								   std::string&		  output_resolved_uri) const {
// 	std::string basic_location, remain_uri, final_uri, first_resolved_uri;
// 	first_resolved_uri = path_resolve_(uri);
// 	find_slash_then_divide_(first_resolved_uri, basic_location, remain_uri);

// 	uri_location_map_p::const_iterator it = uri_case.find(basic_location);
// 	if (it != uri_case.end()) {
// 		final_uri += it->second.root;
// 		if (remain_uri.empty()) {
// 			if (!it->second.index.empty()) {
// 				final_uri += '/' + it->second.index;
// 			}
// 		} else {
// 			final_uri += '/' + remain_uri;
// 		}
// 		output_resolved_uri = path_resolve_(final_uri);
// 		return &it->second;
// 	} else {
// 		final_uri += this->root + '/' + uri;
// 	}
// 	output_resolved_uri = path_resolve_(final_uri);
// 	return NULL;
// }

std::string const
server_info_t::path_resolve_(std::string const& unvalid_path) {
	std::string resolved_path;

	std::string::const_iterator it = unvalid_path.begin();
	while (it != unvalid_path.end()) {
		if (*it == '/') {
			while (*it == '/' && it != unvalid_path.end()) {
				++it;
			}
			resolved_path += '/';
		} else if (*it == '%'
				   && syntax_(hexdigit_, static_cast<uint8_t>(*(it + 1)))
				   && syntax_(hexdigit_, static_cast<uint8_t>(*(it + 2)))) {
			std::string		  temp_hex;
			std::stringstream ss;
			uint16_t		  hex_value;
			temp_hex += *(it + 1);
			temp_hex += *(it + 2);
			ss << std::hex << temp_hex;
			ss >> hex_value;
			resolved_path += static_cast<char>(hex_value);
			it += 3;
		} else {
			resolved_path += *it;
			++it;
		}
	}
	return resolved_path;
}

void
uri_resolved_t::print_(void) const {
	std::cout << "\n-----resolved uri info-----" << std::endl;
	std::cout << "is_cgi: " << is_cgi_ << std::endl;
	std::cout << "is_same_location: " << is_same_location_ << std::endl;
	std::cout << "cgi_location_t: ";
	if (cgi_loc_ == NULL) {
		std::cout << "NULL" << std::endl;
	} else {
		std::cout << "ON" << std::endl;
	}
	std::cout << "request_uri: " << request_uri_ << std::endl;
	std::cout << "resolved_request_uri: " << resolved_request_uri_ << std::endl;
	std::cout << "script_name: " << script_name_ << std::endl;
	std::cout << "script_filename: " << script_filename_ << std::endl;
	std::cout << "path_info: " << path_info_ << std::endl;
	std::cout << "path_translated: " << path_translated_ << std::endl;
	std::cout << "query_string: " << query_string_ << std::endl;
	// std::cout << "fragment : " << fragment_ << std::endl;
	std::cout << "----------------------------" << std::endl;
}

void
server_info_for_copy_stage_t::clear_(void) {
	ip.clear();
	port				= 0;
	default_server_flag = Kother_server;
	server_name.clear();
	root.clear();
	default_error_page.clear();
	error_page_case.clear();
	uri_case.clear();
	cgi_case.clear();
}

void
server_info_for_copy_stage_t::print_() const {
#ifdef CONFIG_DEBUG
	std::cout << "\n[ server_name ] " << server_name << std::endl;
	std::cout << "ip: " << ip << std::endl;
	std::cout << "port: " << port << std::endl;
	if (default_server_flag == Kdefault_server)
		std::cout << "default_server_flag: on" << std::endl;
	else
		std::cout << "default_server_flag: off" << std::endl;

	if (default_error_page != "")
		std::cout << "default_error_page: " << default_error_page << std::endl;
	else
		std::cout << "default_error_page: none" << std::endl;
	std::cout << std::endl;
#endif
}

void
server_info_t::print_(void) const {
	std::cout << "\033[1;32m [ server_name ] " << server_name << "\033[0m" << std::endl;
	std::cout << "ip: " << ip << std::endl;
	std::cout << "port: " << port << std::endl;
	std::cout << "root: " << root << std::endl;
	if (default_server_flag == Kdefault_server)
		std::cout << "\033[1;31m default_server \033[0m" << std::endl;

	if (default_error_page != "")
		std::cout << "\ndefault_error_page: " << default_error_page << std::endl;
	else
		std::cout << "\ndefault_error_page: none" << std::endl;
	std::cout << "error_page_case: " << error_page_case.size() << std::endl;
	error_page_map_p::iterator it_error_page = error_page_case.begin();
	while (it_error_page != error_page_case.end()) {
		std::cout << it_error_page->first << " : " << it_error_page->second << std::endl;
		it_error_page++;
	}
	std::cout << std::endl;

	std::cout << "uri_case: " << uri_case.size() << std::endl;
	uri_location_map_p::iterator it = uri_case.begin();
	while (it != uri_case.end()) {
		std::cout << "\n[ uri ] " << it->first << std::endl;
		it->second.print_();
		it++;
	}
	std::cout << std::endl;

	std::cout << "cgi_case: " << cgi_case.size() << std::endl;
	cgi_list_map_p::const_iterator it_cgi = cgi_case.begin();
	while (it_cgi != cgi_case.end()) {
		std::cout << "\n[ cgi ] " << it_cgi->first << std::endl;
		it_cgi->second.print_();
		it_cgi++;
	}
	std::cout << std::endl;
}

/* NOTE: uri_location_t
********************************************************************************
*/

void
uri_location_for_copy_stage_t::clear_(void) {
	uri.clear();
	module_state		  = Kmodule_none;
	accepted_methods_flag = 0;
	redirect.clear();
	root.clear();
	index.clear();
	autoindex_flag = Kautoindex_off;
	saved_path.clear();
	cgi_pass.clear();
	cgi_path_info.clear();
	client_max_body_size = 0;
}

uri_location_t::uri_location(const uri_location_for_copy_stage_t from)
	: uri(from.uri)
	, module_state(from.module_state)
	, accepted_methods_flag(from.accepted_methods_flag)
	, redirect(from.redirect)
	, root(from.root)
	, index(from.index)
	, autoindex_flag(from.autoindex_flag)
	, saved_path(from.saved_path)
	, cgi_pass(from.cgi_pass)
	, cgi_path_info(from.cgi_path_info)
	, client_max_body_size(from.client_max_body_size) {
#ifdef CONFIG_DEBUG
	std::cout << "uri_location copy construct" << std::endl;
#endif
}

uri_location_t::~uri_location() {
}

void
uri_location_t::print_(void) const {
	// std::cout << "uri_location_t: " << this << std::endl;
	std::cout << "module_state: " << module_state << std::endl;
	std::cout << "accepted_methods_flag: ";
	if (accepted_methods_flag & KGet)
		std::cout << "GET ";
	if (accepted_methods_flag & KPost)
		std::cout << "POST ";
	if (accepted_methods_flag & KHead)
		std::cout << "HEAD ";
	if (accepted_methods_flag & KPut)
		std::cout << "PUT ";
	if (accepted_methods_flag & KDelete)
		std::cout << "DELETE ";
	std::cout << std::endl;

	if (redirect != "")
		std::cout << "redirect: " << redirect << std::endl;
	if (root != "")
		std::cout << "root: " << root << std::endl;
	if (index != "")
		std::cout << "index: " << index << std::endl;

	if (autoindex_flag == Kautoindex_on)
		std::cout << "autoindex: \033[1;32mon\033[0m" << std::endl;
	else
		std::cout << "autoindex: \033[1;31moff\033[0m" << std::endl;

	if (saved_path != "")
		std::cout << "saved_path: " << saved_path << std::endl;
	if (cgi_pass != "")
		std::cout << "cgi_pass: " << cgi_pass << std::endl;
	if (cgi_path_info != "")
		std::cout << "cgi_path_info: " << cgi_path_info << std::endl;

	std::cout << "client_max_body_size: " << client_max_body_size << std::endl;
}

/* NOTE: port_info_t
********************************************************************************
*/

port_info_t::port_info(server_info_t const& from)
	: my_port_default_server(from) {
}

server_info_t const&
port_info_t::search_server_config_(std::string const& host_name) {
	server_map_p::const_iterator it = my_port_map.find(host_name);
	if (it != my_port_map.end()) {
		return it->second;
	}
	return this->my_port_default_server;
}

status
socket_init_and_build_port_info(total_port_server_map_p& config_info,
								port_info_vec&			 port_info,
								uint32_t&				 socket_size) {
	uint32_t prev_socket_size;

	for (total_port_server_map_p::const_iterator it = config_info.begin(); it != config_info.end(); ++it) {

		server_map_p::const_iterator it2 = it->second.begin();
		while (it2 != it->second.end()) {
			if (it2->second.default_server_flag & Kdefault_server) {
				port_info_t temp_port_info(it2->second);
				temp_port_info.my_port	   = it->first;
				temp_port_info.my_port_map = it->second;

				temp_port_info.listen_sd = socket(AF_INET, SOCK_STREAM, 0); // TODO :: key
				prev_socket_size		 = socket_size;
				socket_size				 = temp_port_info.listen_sd;
				spx_log_(temp_port_info.listen_sd);
				if (temp_port_info.listen_sd < 0) {
					error_exit("socket", NULL, 0);
				}
				int opt(1);
				if (setsockopt(temp_port_info.listen_sd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) { // NOTE:: SO_REUSEPORT
					error_exit("setsockopt", close, temp_port_info.listen_sd);
				}
				if (fcntl(temp_port_info.listen_sd, F_SETFL, O_NONBLOCK) == -1) {
					error_exit("fcntl", close, temp_port_info.listen_sd);
				}
				temp_port_info.addr_server.sin_family	   = AF_INET;
				temp_port_info.addr_server.sin_port		   = htons(temp_port_info.my_port);
				temp_port_info.addr_server.sin_addr.s_addr = htonl(INADDR_ANY);
				if (bind(temp_port_info.listen_sd, (struct sockaddr*)&temp_port_info.addr_server, sizeof(temp_port_info.addr_server)) == -1) {
					std::stringstream ss;
					ss << temp_port_info.my_port;
					std::string err = "bind port " + ss.str() + " ";
					error_exit(err, close, temp_port_info.listen_sd);
				}
				if (listen(temp_port_info.listen_sd, LISTEN_BACKLOG_SIZE) < 0) {
					error_exit("listen", close, temp_port_info.listen_sd);
				}
				if (prev_socket_size == 0) {
					uint32_t i = 0;
					while (i < socket_size) {
						port_info.push_back(temp_port_info);
						++i;
					}
				} else {
					uint32_t i = prev_socket_size + 1;
					while (i < socket_size) {
						port_info.push_back(temp_port_info);
						++i;
					}
				}
				port_info.push_back(temp_port_info);
				break;
			}
			++it2;
		}
		if (it2 == it->second.end()) {
			std::cerr << "no default server in port " << it->first << std::endl;
			error_exit_msg("");
		}
	}
	if (socket_size == 0) {
		error_exit_msg("socket size error");
	}
	++socket_size;
	config_info.clear();
	return spx_ok;
}
