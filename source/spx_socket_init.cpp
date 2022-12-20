#include "spx_socket_init.hpp"
#include "spx_config_port_info.hpp"
#include "spx_core_type.hpp"
#include "spx_core_util_box.hpp"
#include <_types/_uint32_t.h>
#include <vector>

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

	std::string
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

} // namespace

port_info_t::port_info(server_info_t const& from)
	: my_port_default_server(from) {
}

std::string const
port_info_t::get_error_page_(uint32_t const& error_code) {
	// check actual server -> if not default server_info
	// search error page in error_page_map
	// if not found -> return default error page
	// if not exit default error page -> return default error page

	return error_page_str_(static_cast<error_page_e>(error_code));
}
std::string const&
get_uri_location_(std::string const& uri) { // NOTE :: before stage need to check specific method (CGI? or else?)
	// 1. find uri in uri server(host) name
	// 2 (from 1) if exit uri -> location.path substitution
	//  -> path_resolve(root + (location -> lo.root substitution) + remain uri);
	// 3 (from 1) if not exit uri -> return path_resolve(root + uri);
}

status
socket_init_and_build_port_info(total_port_server_map_p&  config_info,
								std::vector<port_info_t>& port_info,
								uint32_t&				  socket_size) {
	uint32_t prev_socket_size;

	for (total_port_server_map_p::const_iterator it = config_info.begin(); it != config_info.end(); ++it) {

		server_map_p::const_iterator it2 = it->second.begin();
		while (it2 != it->second.end()) {
			prev_socket_size = socket_size;
			if (it2->second.default_server_flag & Kdefault_server) {
				port_info_t temp_port_info(it2->second);
				temp_port_info.my_port	   = it->first;
				temp_port_info.my_port_map = it->second;

				temp_port_info.listen_sd = socket(AF_INET, SOCK_STREAM, 0); // TODO :: key
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
				if (socket_size == 0) {
					socket_size = temp_port_info.listen_sd;
					{
						uint32_t i = 0;
						while (i < socket_size) {
							port_info.push_back(temp_port_info);
							++i;
						}
					}
				}
				{
					uint32_t i = prev_socket_size;
					while (i < socket_size) {
						port_info.push_back(temp_port_info);
						++i;
					}
				}
				++socket_size;
				break;
			}
			++it2;
		}
		if (it2 == it->second.end()) {
			std::cerr << "no default server in port " << it->first << std::endl;
			error_exit_msg("");
		}
	}
	if (socket_size == 0 || socket_size > 65535) {
		error_exit_msg("socket size error");
	}
	config_info.clear();
	return spx_ok;
}
