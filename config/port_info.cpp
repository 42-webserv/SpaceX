#include "port_info.hpp"
#include "config.hpp"
#include <iostream>

namespace {

}

void
uri_location_t::print(void) const {
#ifdef CONFIG_DEBUG
	// std::cout << "uri_location_t: " << this << std::endl;
	std::cout << "module_state: " << module_state << std::endl;
	std::cout << "accepted_method_flag: " << accepted_methods_flag << std::endl;
	if (redirect != "")
		std::cout << "redirect: " << redirect << std::endl;
	if (root != "")
		std::cout << "root: " << root << std::endl;
	if (index != "")
		std::cout << "index: " << index << std::endl;
	std::cout << "autoindex_flag: " << autoindex_flag << std::endl;
	if (saved_path != "")
		std::cout << "saved_path: " << saved_path << std::endl;
	if (cgi_pass != "")
		std::cout << "cgi_pass: " << cgi_pass << std::endl;
	if (cgi_path_info != "")
		std::cout << "cgi_path_info: " << cgi_path_info << std::endl;
	std::cout << std::endl;
#endif
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
	, cgi_path_info(from.cgi_path_info) {
#ifdef CONFIG_DEBUG
	std::cout << "uri_location copy construct" << std::endl;
#endif
}

uri_location_t::~uri_location() {
}

/*
 * --------------------
 */

void
server_info_t::print(void) const {
#ifdef CONFIG_DEBUG
	std::cout << "ip: " << ip << std::endl;
	std::cout << "port: " << port << std::endl;
	std::cout << "default_server_flag: " << default_server_flag << std::endl;
	std::cout << "server_name: " << server_name << std::endl;
	std::cout << "client_max_body_size: " << client_max_body_size << std::endl;
	std::cout << "error_page: " << error_page << std::endl;

	// error_page_map_p::iterator it_error_page = error_page_case.begin();
	// while (it_error_page != error_page_case.end()) {
	// 	std::cout << "error_page: " << it_error_page->first << " " << it_error_page->second << std::endl;
	// 	it_error_page++;
	// }

	std::cout << "uri_case: " << uri_case.size() << std::endl;
	uri_location_map_p::iterator it = uri_case.begin();
	while (it != uri_case.end()) {
		std::cout << "uri: " << it->first << std::endl;
		it->second.print();
		std::cout << std::endl;
		it++;
	}
#endif
}

server_info_t::server_info(server_info_for_copy_stage_t const& from)
	: ip(from.ip)
	, port(from.port)
	, default_server_flag(from.default_server_flag)
	, server_name(from.server_name)
	, error_page(from.error_page)
	, client_max_body_size(from.client_max_body_size) {
#ifdef CONFIG_DEBUG

	std::cout << "server_info copy construct" << std::endl;
#endif
	// error_page_case = from.error_page_case;
	if (from.uri_case.size() != 0) {
		uri_case.insert(from.uri_case.begin(), from.uri_case.end());
	}
}

server_info_t::~server_info() {
}
