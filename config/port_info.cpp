#include "port_info.hpp"
#include "config.hpp"
#include <iostream>

namespace {

}

void
uri_location_t::print(void) const {
#ifdef DEBUG
	std::cout << "uri_location_t: " << this << std::endl;
	std::cout << "module_state: " << module_state << std::endl;
	std::cout << "accepted_method_flag: " << accepted_method_flag << std::endl;
	std::cout << "redirect: " << redirect << std::endl;
	std::cout << "root: " << root << std::endl;
	std::cout << "index: " << index << std::endl;
	std::cout << "autoindex_flag: " << autoindex_flag << std::endl;
	std::cout << "saved_path: " << saved_path << std::endl;
	std::cout << "cgi_pass: " << cgi_pass << std::endl;
	std::cout << "cgi_path_info: " << cgi_path_info << std::endl;
#endif
}

uri_location_t::uri_location(uri_location_for_copy_stage_t const& from)
	: module_state(from.module_state)
	, accepted_method_flag(from.accepted_method_flag)
	, redirect(from.redirect)
	, root(from.root)
	, index(from.index)
	, autoindex_flag(from.autoindex_flag)
	, saved_path(from.saved_path)
	, cgi_pass(from.cgi_pass)
	, cgi_path_info(from.cgi_path_info) {
#ifdef DEBUG
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
#ifdef DEBUG
	std::cout << "ip: " << ip << std::endl;
	std::cout << "port: " << port << std::endl;
	std::cout << "default_server_flag: " << default_server_flag << std::endl;
	std::cout << "server_name: " << server_name << std::endl;
	std::cout << "error_page: " << error_page << std::endl;
	std::cout << "client_max_body_size: " << client_max_body_size << std::endl;
	std::cout << "uri_case: " << uri_case.size() << std::endl;
	std::cout << std::endl;

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
	, client_max_body_size(from.client_max_body_size)
	, uri_case(from.uri_case) {
#ifdef DEBUG
	std::cout << "server_info copy construct" << std::endl;
#endif
}

server_info_t::~server_info() {
}
