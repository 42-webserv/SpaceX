#include "spx_port_info.hpp"
#include "spx_config_parse.hpp"
#include <iostream>

namespace {

}

void
uri_location_for_copy_stage_t::clear(void) {
	uri.clear();
	module_state		  = module_none;
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
server_info_for_copy_stage_t::clear(void) {
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
server_info_for_copy_stage_t::print() const {

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
uri_location_t::print(void) const {
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
	if (accepted_methods_flag & KOptions)
		std::cout << "OPTIONS ";
	std::cout << std::endl;

	if (redirect != "")
		std::cout << "redirect: " << redirect << std::endl;
	if (root != "")
		std::cout << "root: " << root << std::endl;
	if (index != "")
		std::cout << "index: " << index << std::endl;

	if (autoindex_flag == Kautoindex_on)
		std::cout << "autoindex: on" << std::endl;
	else
		std::cout << "autoindex: off" << std::endl;

	if (saved_path != "")
		std::cout << "saved_path: " << saved_path << std::endl;
	if (cgi_pass != "")
		std::cout << "cgi_pass: " << cgi_pass << std::endl;
	if (cgi_path_info != "")
		std::cout << "cgi_path_info: " << cgi_path_info << std::endl;
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
	std::cout << "[ server_name ] " << server_name << std::endl;
	std::cout << "ip: " << ip << std::endl;
	std::cout << "port: " << port << std::endl;
	std::cout << "root: " << root << std::endl;
	if (default_server_flag == Kdefault_server)
		std::cout << "default_server_flag: on" << std::endl;
	else
		std::cout << "default_server_flag: off" << std::endl;
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
		it->second.print();
		it++;
	}
	std::cout << std::endl;
}

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

// server_info&
// server_info::operator=(server_info_t const& from) {
// 	server_info* tmp = new server_info_t(from);
// 	return *tmp;
// }

server_info_t::~server_info() {
}
