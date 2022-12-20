#include "spx_config_port_info.hpp"
#include "spx_config_parse.hpp"
#include "spx_core_util_box.hpp"
#include <fstream>
#include <sstream>
#include <string>

namespace {

#define ERROR_PAGE_MAP(XX)                                                        \
	XX(100, Continue, 100 Continue)                                               \
	XX(101, Switching_Protocols, 101 Switching Protocols)                         \
	XX(102, Processing, 102 Processing)                                           \
	XX(103, Early_Hints, 103 Early Hints)                                         \
	XX(200, OK, 200 OK)                                                           \
	XX(201, Created, 201 Created)                                                 \
	XX(202, Accepted, 202 Accepted)                                               \
	XX(203, Non_Authoritative_Information, 203 Non Authoritative Information)     \
	XX(204, No_Content, 204 No Content)                                           \
	XX(205, Reset_Content, 205 Reset Content)                                     \
	XX(206, Partial_Content, 206 Partial Content)                                 \
	XX(207, Multi_Status, 207 Multi Status)                                       \
	XX(208, Already_Reported, 208 Already Reported)                               \
	XX(226, IM_Used, 226 IM Used)                                                 \
	XX(300, Multiple_Choices, 300 Multiple Choices)                               \
	XX(301, Moved_Permanently, 301 Moved Permanently)                             \
	XX(302, Found, 302 Found)                                                     \
	XX(303, See_Other, 303 See Other)                                             \
	XX(304, Not_Modified, 304 Not Modified)                                       \
	XX(305, Use_Proxy, 305 Use Proxy)                                             \
	XX(306, Switch_Proxy, 306 Switch Proxy)                                       \
	XX(307, Temporary_Redirect, 307 Temporary Redirect)                           \
	XX(308, Permanent_Redirect, 308 Permanent Redirect)                           \
	XX(400, Bad_Request, 400 Bad Request)                                         \
	XX(401, Unauthorized, 401 Unauthorized)                                       \
	XX(402, Payment_Required, 402 Payment Required)                               \
	XX(403, Forbidden, 403 Forbidden)                                             \
	XX(404, Not_Found, 404 Not Found)                                             \
	XX(405, Method_Not_Allowed, 405 Method Not Allowed)                           \
	XX(406, Not_Acceptable, 406 Not Acceptable)                                   \
	XX(407, Proxy_Authentication_Required, 407 Proxy Authentication Required)     \
	XX(408, Request_Timeout, 408 Request Timeout)                                 \
	XX(409, Conflict, 409 Conflict)                                               \
	XX(410, Gone, 410 Gone)                                                       \
	XX(411, Length_Required, 411 Length Required)                                 \
	XX(412, Precondition_Failed, 412 Precondition Failed)                         \
	XX(413, Entity_Too_Large, 413 Entity Too Large)                               \
	XX(414, URI_Too_Long, 414 URI Too Long)                                       \
	XX(415, Unsupported_Media_Type, 415 Unsupported Media Type)                   \
	XX(416, Range_Not_Satisfiable, 416 Range Not Satisfiable)                     \
	XX(417, Expectation_Failed, 417 Expectation Failed)                           \
	XX(418, I_m_a_teapot, 418 I am a teapot)                                      \
	XX(421, Misdirected_Request, 421 Misdirected Request)                         \
	XX(422, Unprocessable_Entity, 422 Unprocessable Entity)                       \
	XX(423, Locked, 423 Locked)                                                   \
	XX(424, Failed_Dependency, 424 Failed Dependency)                             \
	XX(425, Too_Early, 425 Too Early)                                             \
	XX(426, Upgrade_Required, 426 Upgrade Required)                               \
	XX(428, Precondition_Required, 428 Precondition Required)                     \
	XX(429, Too_Many_Requests, 429 Too Many Requests)                             \
	XX(431, Request_Header_Fields_Too_Large, 431 Request Header Fields Too Large) \
	XX(451, Unavailable_For_Legal_Reasons, 451 Unavailable For Legal Reasons)     \
	XX(494, Request_Header_Too_Large, 494 Request Header Or Cookie Too Large)     \
	XX(500, Internal_Server_Error, 500 Internal Server Error)                     \
	XX(501, Not_Implemented, 501 Not Implemented)                                 \
	XX(502, Bad_Gateway, 502 Bad Gateway)                                         \
	XX(503, Service_Unavailable, 503 Service Unavailable)                         \
	XX(504, Gateway_Timeout, 504 Gateway Timeout)                                 \
	XX(505, HTTP_Version_Not_Supported, 505 HTTP Version Not Supported)           \
	XX(506, Variant_Also_Negotiates, 506 Variant Also Negotiates)                 \
	XX(507, Insufficient_Storage, 507 Insufficient Storage)                       \
	XX(508, Loop_Detected, 508 Loop Detected)                                     \
	XX(510, Not_Extended, 510 Not Extended)                                       \
	XX(511, Network_Authentication_Required, 511 Network Authentication Required)

	typedef enum error_page {
#define XX(num, name, string) ERROR_PAGE_##name = num,
		ERROR_PAGE_MAP(XX)
#undef XX
	} error_page_e;

	inline std::string
	error_page_str_(error_page_e s) {
		switch (s) {
#define XX(num, name, string) \
	case ERROR_PAGE_##name:   \
		return #string;
			ERROR_PAGE_MAP(XX)
#undef XX
		default:
			return "< XXX unknown cause >";
		}
	};

	inline bool
	try_open_file_(std::string const candidate_error_path,
				   std::string&		 error_page_context) {
		std::fstream try_open_file(candidate_error_path.c_str(), std::ios::in);

		if (try_open_file.is_open()) {
			std::stringstream ss;
			ss << try_open_file.rdbuf();
			if (ss.str().size() != 0) {
				try_open_file.close();
				error_page_context = ss.str();
				return true;
			}
		}
		return false;
	}

	inline void
	handmade_error_page_(uint32_t const& error_code,
						 std::string&	 error_page) {
		error_page += "<html>" CRLF "<head><title>";
		error_page += error_page_str_(static_cast<error_page_e>(error_code));
		error_page += "</title></head>" CRLF "<body>" CRLF "<center><h1>";
		error_page += error_page_str_(static_cast<error_page_e>(error_code));
		error_page += "</h1></center>" CRLF "</body>" CRLF "</html>";
	}

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
server_info_t::get_error_page_(uint32_t const& error_code) const {
	error_page_map_p::const_iterator it = error_page_case.find(error_code);
	std::string						 temp_error_page;

	if (it != error_page_case.end()) {
		if (try_open_file_(path_resolve_(this->root + it->second), temp_error_page)) {
			return temp_error_page;
		}
	}
	if (try_open_file_(path_resolve_(this->root + this->default_error_page), temp_error_page)) {
		return temp_error_page;
	}
	handmade_error_page_(error_code, temp_error_page);
	return temp_error_page;
}

std::string const
server_info_t::get_uri_location_(std::string const& uri) const { // TODO :: check end slash value. is it dir? or file?
	std::string basic_location, remain_uri, final_uri;
	find_slash_then_divide_(uri, basic_location, remain_uri);

	uri_location_map_p::const_iterator it = uri_case.find(basic_location);
	if (it != uri_case.end()) {
		final_uri = path_resolve_(this->root + it->second.root + remain_uri);
		// final_uri += "/";
		// final_uri += it->second.index; // TODO: check to index is setted default value in config parse stage.
	} else {
		final_uri = path_resolve_(this->root + uri);
		// final_uri += "/";
		// final_uri += "index.html";
	}
	return final_uri;
}

std::string const
server_info_t::path_resolve_(std::string const unvalid_path) {
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
uri_location_for_copy_stage_t::clear_(void) {
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
