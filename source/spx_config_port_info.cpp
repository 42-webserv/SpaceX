#include "spx_config_port_info.hpp"
#include "spx_config_parse.hpp"
#include "spx_core_util_box.hpp"
#include <cstddef>
#include <fstream>
#include <sstream>
#include <string>

namespace {

	inline void
	find_slash_then_divide_(std::string const& origin_uri,
							std::string&	   basic_location,
							std::string&	   remain_uri) {
		std::string::size_type slash_pos = origin_uri.find_first_of('/', 1);
		if (slash_pos == std::string::npos) {
			basic_location = origin_uri;
			remain_uri	   = "";
		} else {
			basic_location = origin_uri.substr(0, slash_pos);
			remain_uri	   = origin_uri.substr(slash_pos + 1);
		}
	}

} // namespace

server_info_t::server_info(server_info_for_copy_stage_t const& from)
	: ip(from.ip)
	, port(from.port)
	, default_server_flag(from.default_server_flag)
	, server_name(from.server_name)
	, root(from.root)
	, client_max_body_size(from.client_max_body_size)
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
}

server_info_t::server_info(server_info_t const& from)
	: ip(from.ip)
	, port(from.port)
	, default_server_flag(from.default_server_flag)
	, server_name(from.server_name)
	, root(from.root)
	, client_max_body_size(from.client_max_body_size)
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
								   std::string&		  output_resolved_uri) const {
	std::string basic_location, remain_uri, final_uri, first_resolved_uri;
	first_resolved_uri = path_resolve_(uri);
	find_slash_then_divide_(first_resolved_uri, basic_location, remain_uri);

	uri_location_map_p::const_iterator it = uri_case.find(basic_location);
	if (it != uri_case.end()) {
		final_uri += '/' + it->second.root;
		if (remain_uri.empty()) {
			if (it->second.index.empty()) {
				final_uri += "/index.html";
			} else {
				final_uri += '/' + it->second.index;
			}
		} else {
			final_uri += '/' + remain_uri;
		}
		output_resolved_uri = path_resolve_(final_uri);
		return &it->second;
	} else {
		final_uri += '/' + this->root + '/' + uri;
	}
	output_resolved_uri = path_resolve_(final_uri);
	return NULL;
}

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
#ifdef YOMA_SEARCH_DEBUG
	std::cout << "resolved_path : " << resolved_path << std::endl;
#endif
	return resolved_path;
}

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
}

void
server_info_for_copy_stage_t::clear_(void) {
	ip.clear();
	port				= 0;
	default_server_flag = Kother_server;
	server_name.clear();
	root.clear();
	client_max_body_size = 0;
	default_error_page.clear();
	error_page_case.clear();
	uri_case.clear();
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
	std::cout << "client_max_body_size: " << client_max_body_size << std::endl;

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
	std::cout << "client_max_body_size: " << client_max_body_size << std::endl;

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
}

/*
********************************************************************************
*/

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
	, cgi_path_info(from.cgi_path_info) {
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
}
