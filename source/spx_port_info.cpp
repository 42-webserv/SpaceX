#include "spx_port_info.hpp"
#include "spx_autoindex_generator.hpp"
#include "spx_core_type.hpp"
#include "spx_core_util_box.hpp"
#include "spx_req_res_field.hpp"

#include <cstddef>
#include <dirent.h>
#include <string>
#include <sys/socket.h>

namespace {

	inline void
	close_socket_and_exit__(int const prev_socket_size, port_info_vec& port_info) {
		spx_log_(COLOR_RED "close_socket_and_exit__" COLOR_RESET);
		for (int i = 0; i <= prev_socket_size; ++i) {
			if (i == port_info[i].listen_sd) {
				spx_log_("close port: ", port_info[i].my_port);
				close(port_info[i].listen_sd);
			}
		}
		exit(spx_error);
	}

} // namespace

server_info_t::server_info(server_info_for_copy_stage_t const& from)
	: ip(from.ip)
	, port(from.port)
	, default_server_flag(from.default_server_flag)
	, server_name(from.server_name)
	, root(from.root)
	, default_error_page(from.default_error_page) {
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
								   uri_resolved_t&	  uri_resolved_sets,
								   int				  request_method) const {

	uri_location_t*				return_location		 = NULL;
	uri_location_t*				return_location_root = NULL;
	std::string					temp;
	std::string					candidate_physical_path;
	std::string					candidate_phenomenon_path;
	std::string					candidate_surfix_index;
	uint32_t					flag_for_uri_status = 0;
	std::string::const_iterator it					= uri.begin();

	//  initialize
	uri_resolved_sets.is_cgi_  = false;
	uri_resolved_sets.cgi_loc_ = NULL;
	uri_resolved_sets.cgi_path_info_.clear();
	uri_resolved_sets.request_uri_.clear();
	uri_resolved_sets.resolved_request_uri_.clear();
	uri_resolved_sets.script_name_.clear();
	uri_resolved_sets.script_filename_.clear();
	uri_resolved_sets.path_info_.clear();
	uri_resolved_sets.path_translated_.clear();
	uri_resolved_sets.query_string_.clear();
	uri_resolved_sets.fragment_.clear();

	if (uri.empty()) {
		return NULL;
	}

	uri_resolved_sets.request_uri_ = uri;

	enum {
		uri_main, // 0
		uri_depth,
		uri_cgi,
		uri_query,
		uri_fragment,
		uri_find_delimeter_case, // 5
		uri_done,
		uri_end
	} state;

	state = uri_main;

	while (state != uri_end) {
		switch (state) {
		case uri_main: {
			while (it != uri.end() && *it == '/') {
				++it;
			}
			temp += "/";
			while (it != uri.end() && syntax_(usual_for_uri_parse_, static_cast<uint8_t>(*it))) {
				temp += *it;
				++it;
			}
			uri_location_map_p::iterator loc_it = uri_case.find(temp);
			if ((it != uri.end() && *it == '.') || loc_it == uri_case.end()) {
				flag_for_uri_status |= Kuri_notfound_uri;
				loc_it = uri_case.find("/");
				uri_resolved_sets.script_name_ += temp;
			}
			return_location			  = &loc_it->second;
			return_location_root	  = &loc_it->second;
			candidate_physical_path	  = loc_it->second.root;
			candidate_phenomenon_path = loc_it->second.uri;
			if (request_method & (REQ_POST | REQ_PUT)) {
				candidate_surfix_index = loc_it->second.saved_path;
			} else {
				candidate_surfix_index = loc_it->second.index;
			}
			if (!(return_location->cgi_pass.empty())) {
				std::string				 extension = loc_it->second.cgi_pass.substr(loc_it->second.cgi_pass.find_last_of('.'));
				cgi_list_map_p::iterator cgi_it	   = cgi_case.find(extension);
				if (cgi_it != cgi_case.end()) {
					uri_resolved_sets.cgi_path_info_   = cgi_it->second.cgi_path_info;
					uri_resolved_sets.script_filename_ = loc_it->second.cgi_pass;
				} else {
					uri_resolved_sets.cgi_path_info_ = loc_it->second.cgi_pass;
				}
				uri_resolved_sets.is_cgi_  = true;
				uri_resolved_sets.cgi_loc_ = &loc_it->second;
				flag_for_uri_status |= Kuri_cgi;
				flag_for_uri_status |= Kflag_cgi_pass;
			}
			temp.clear();
			state = uri_find_delimeter_case;
			break;
		}

		case uri_find_delimeter_case: {
			if (it == uri.end()) {
				state = uri_done;
				break;
			}
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
				state = uri_depth;
				break;
			}
			default: {
				state = uri_done;
				break;
			}
			}
			break;
		}

		case uri_depth: {
			while (it != uri.end() && *it == '/') {
				++it;
			}
			temp += "/";
			while (it != uri.end() && syntax_(usual_for_uri_parse_, static_cast<uint8_t>(*it))) {
				temp += *it;
				++it;
			}
			if (flag_for_uri_status & Kuri_cgi) {
				flag_for_uri_status |= Kuri_path_info;
				uri_resolved_sets.path_info_ += temp;
				while (it != uri.end() && syntax_(except_query_fragment_, static_cast<uint8_t>(*it))) {
					uri_resolved_sets.path_info_ += *it;
					++it;
				}
			} else {
				flag_for_uri_status |= Kuri_depth_uri;
				uri_resolved_sets.script_name_ += temp;
			}
			temp.clear();
			state = uri_find_delimeter_case;
			break;
		}

		case uri_cgi: {
			while (it != uri.end() && syntax_(except_slash_query_fragment_, static_cast<uint8_t>(*it))) {
				temp += *it;
				++it;
			}
			if (flag_for_uri_status & Kuri_cgi) {
				uri_resolved_sets.path_info_ += temp;
			} else {
				size_t pos_ = temp.find_last_of(".");
				if (pos_ != std::string::npos) {
					std::string				 check_extension = temp.substr(pos_);
					cgi_list_map_p::iterator cgi_it			 = cgi_case.find(check_extension);
					if (cgi_it != cgi_case.end()) {
						uri_resolved_sets.cgi_loc_ = &cgi_it->second;
						// if (uri_resolved_sets.cgi_loc_->accepted_methods_flag & request_method) {
						uri_resolved_sets.is_cgi_		 = true;
						uri_resolved_sets.cgi_path_info_ = cgi_it->second.cgi_path_info;
						return_location					 = &cgi_it->second;
						flag_for_uri_status |= Kuri_cgi;
						// }
					}
				}
				if (!(flag_for_uri_status & Kuri_cgi)) {
					flag_for_uri_status |= Kuri_check_extension;
				}
				uri_resolved_sets.script_name_ += temp;
			}
			temp.clear();
			state = uri_find_delimeter_case;
			break;
		}

		case uri_query: {
			while (it != uri.end() && *it != '#') {
				uri_resolved_sets.query_string_ += *it;
				++it;
			}
			state = uri_find_delimeter_case;
			break;
		}

		case uri_fragment: {
			while (it != uri.end()) {
				uri_resolved_sets.fragment_ += *it;
				++it;
			}
			state = uri_done;
			break;
		}

		case uri_done: {
			if (flag_for_uri_status & Kuri_depth_uri && flag_for_uri_status & Kuri_notfound_uri) {
				spx_log_("depth_uri and notfound_uri");
				return_location			   = NULL;
				uri_resolved_sets.cgi_loc_ = NULL;
				uri_resolved_sets.is_cgi_  = false;
			} else if (!(flag_for_uri_status & Kuri_cgi)
					   && return_location->accepted_methods_flag & request_method
					   && !(flag_for_uri_status & Kuri_notfound_uri)
					   && !(return_location->cgi_path_info.empty())) {
				uri_resolved_sets.cgi_loc_ = return_location;
				uri_resolved_sets.is_cgi_  = true;
			}
			uri_resolved_sets.script_filename_ = path_resolve_(candidate_physical_path + uri_resolved_sets.script_name_);
			uri_resolved_sets.script_name_	   = path_resolve_(candidate_phenomenon_path + uri_resolved_sets.script_name_);
			if (!(flag_for_uri_status & (Kuri_notfound_uri | Kuri_cgi | Kuri_check_extension))
				&& !(candidate_surfix_index.empty())) {
				DIR* dir = NULL;
				if (request_method & (REQ_GET | REQ_HEAD)
					|| ((request_method & (REQ_POST | REQ_PUT)) && (dir = opendir(uri_resolved_sets.script_filename_.c_str())))) {
					uri_resolved_sets.script_filename_ = path_resolve_(uri_resolved_sets.script_filename_ + "/" + candidate_surfix_index);
					uri_resolved_sets.script_name_	   = path_resolve_(uri_resolved_sets.script_name_ + "/" + candidate_surfix_index);
					if (dir) {
						closedir(dir);
					}
				}
			}
			if (flag_for_uri_status & Kflag_cgi_pass) {
				uri_resolved_sets.script_filename_ = uri_resolved_sets.cgi_loc_->cgi_pass;
			}
			if (return_location != NULL && flag_for_uri_status & Kuri_cgi) {
				std::ifstream file;
				file.open(uri_resolved_sets.script_filename_.c_str(), std::ios::in);
				if (file.good()) {
					if (!(flag_for_uri_status & Kuri_notfound_uri)) {
							return_location = return_location_root;
							}
					spx_log_("cgi file exist");
					file.close();
				} else {
					spx_log_("cgi file doesn't exist");
					uri_resolved_sets.script_filename_ = uri_resolved_sets.cgi_loc_->cgi_pass; 
//return_location			   = return_location_root;
//					uri_resolved_sets.cgi_loc_ = NULL;
//					uri_resolved_sets.is_cgi_  = false;
				}
			}
			uri_resolved_sets.path_info_			= path_resolve_(uri_resolved_sets.path_info_);
			uri_resolved_sets.resolved_request_uri_ = uri_resolved_sets.script_name_ + uri_resolved_sets.path_info_;
			if (uri_resolved_sets.path_info_.empty() == false) {
				uri_resolved_sets.path_translated_ = path_resolve_(candidate_physical_path + "/" + uri_resolved_sets.path_info_);
			}
			state = uri_end;
			break;
		}
		case uri_end: {
			break;
		}
		} // switch end
	} // while end
	if (return_location) {
		spx_log_(return_location->uri);
	}
	uri_resolved_sets.print_();
	return return_location;
}

std::string const
server_info_t::path_resolve_(std::string const& unvalid_path) {
	std::string resolved_path;

	std::string::const_iterator it = unvalid_path.begin();
	while (it != unvalid_path.end()) {
		if (syntax_(only_slash_, static_cast<uint8_t>(*it))) {
			while (syntax_(only_slash_, static_cast<uint8_t>(*it)) && it != unvalid_path.end()) {
				++it;
			}
			resolved_path += '/';
		} else if (syntax_(only_percent_, static_cast<uint8_t>(*it))
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
	spx_log_("\n-----resolved uri info-----");
	spx_log_("is_cgi: ", is_cgi_);
	if (cgi_loc_ == NULL) {
		spx_log_("cgi_location_t: NULL");
	} else {
		spx_log_("cgi_location_t: ON");
	}
	spx_log_("cgi_path_info: ", cgi_path_info_);
	spx_log_("request_uri: ", request_uri_);
	spx_log_("resolved_request_uri: ", resolved_request_uri_);
	spx_log_("script_name: ", script_name_);
	spx_log_("script_filename: ", script_filename_);
	spx_log_("path_info: ", path_info_);
	spx_log_("path_translated: ", path_translated_);
	spx_log_("query_string: ", query_string_);
	spx_log_("----------------------------\n");
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
}

void
server_info_t::print_(void) const {
	std::cout << COLOR_GREEN << " [ server_name ] " << server_name << COLOR_RESET;
	if (default_server_flag == Kdefault_server) {
		std::cout << COLOR_RED << " <---- default_server" << COLOR_RESET << std::endl;
	} else {
		std::cout << std::endl;
	}
	std::cout << "ip: " << ip << std::endl;
	std::cout << "port: " << port << std::endl;
	std::cout << "root: " << root << std::endl;

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

	std::cout << COLOR_RED << "uri_case: " << uri_case.size() << "\t---------------" << COLOR_RESET << std::endl;
	uri_location_map_p::iterator it = uri_case.begin();
	while (it != uri_case.end()) {
		std::cout << "\n[ uri ] " << it->first << std::endl;
		it->second.print_();
		it++;
	}
	std::cout << std::endl;

	if (cgi_case.size() != 0) {
		std::cout << COLOR_RED << "cgi_case: " << cgi_case.size() << "\t---------------" << COLOR_RESET << std::endl;
		cgi_list_map_p::const_iterator it_cgi = cgi_case.begin();
		while (it_cgi != cgi_case.end()) {
			std::cout << "\n[ cgi ] " << it_cgi->first << std::endl;
			it_cgi->second.print_();
			it_cgi++;
		}
		std::cout << std::endl;
	}
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
	client_max_body_size = -1;
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
	if (accepted_methods_flag & KHead)
		std::cout << "HEAD ";
	if (accepted_methods_flag & KPut)
		std::cout << "PUT ";
	if (accepted_methods_flag & KPost)
		std::cout << "POST ";
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
								int64_t&				 socket_size,
								int64_t&				 first_socket) {
	int64_t prev_socket_size;

	for (total_port_server_map_p::const_iterator it = config_info.begin(); it != config_info.end(); ++it) {

		server_map_p::const_iterator it2 = it->second.begin();
		while (it2 != it->second.end()) {
			if (it2->second.default_server_flag & Kdefault_server) {
				port_info_t temp_port_info(it2->second);
				temp_port_info.my_port	   = it->first;
				temp_port_info.my_port_map = it->second;

				temp_port_info.listen_sd = socket(AF_INET, SOCK_STREAM, 0);
				prev_socket_size		 = socket_size;
				socket_size				 = temp_port_info.listen_sd;
				if (prev_socket_size == -1) {
					first_socket = socket_size;
				}
				if (temp_port_info.listen_sd < 0) {
					error_str("socket error");
					close_socket_and_exit__(prev_socket_size, port_info);
				}
				int opt(1);
				if (setsockopt(temp_port_info.listen_sd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
					error_fn("setsockopt error", close, temp_port_info.listen_sd);
					close_socket_and_exit__(prev_socket_size, port_info);
				}
				if (fcntl(temp_port_info.listen_sd, F_SETFL, O_NONBLOCK) == -1) {
					error_fn("fcntl error", close, temp_port_info.listen_sd);
					close_socket_and_exit__(prev_socket_size, port_info);
				}
				bzero((char*)&temp_port_info.addr_server, sizeof(temp_port_info.addr_server));
				temp_port_info.addr_server.sin_family	   = AF_INET;
				temp_port_info.addr_server.sin_port		   = htons(temp_port_info.my_port);
				temp_port_info.addr_server.sin_addr.s_addr = htonl(INADDR_ANY);
				if (bind(temp_port_info.listen_sd, (struct sockaddr*)&temp_port_info.addr_server, sizeof(temp_port_info.addr_server)) == -1) {
					spx_log_("current listen_sd : ", temp_port_info.listen_sd);
					spx_log_("prev_socket_size", prev_socket_size);
					std::stringstream ss;
					ss << temp_port_info.my_port;
					std::string err = "bind at port " + ss.str();
					error_log_(err);
					error_fn("bind error", close, temp_port_info.listen_sd);
					close_socket_and_exit__(prev_socket_size, port_info);
				}
				if (listen(temp_port_info.listen_sd, LISTEN_BACKLOG_SIZE) < 0) {
					error_fn("listen error", close, temp_port_info.listen_sd);
					close_socket_and_exit__(prev_socket_size, port_info);
				}
				int64_t i = prev_socket_size;
				while (i < socket_size) {
					port_info.push_back(temp_port_info);
					++i;
				}
				break;
			}
			++it2;
		}
		if (it2 == it->second.end()) {
			std::cerr << "no default server in port " << it->first << std::endl;
			close_socket_and_exit__(socket_size, port_info);
		}
	}
	if (socket_size == 0) {
		error_exit("socket size error");
	}
	++socket_size;
	config_info.clear();
	return spx_ok;
}
