#include "spx_core_util_box.hpp"

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
			ERROR_PAGE_LAST
	} error_page_e;

	inline std::string
	error_page_str__(error_page_e s) {
		switch (s) {
#define XX(num, name, string) \
	case ERROR_PAGE_##name:   \
		return #string;
			ERROR_PAGE_MAP(XX)
#undef XX
		default:
			return "< XXX unknown cause >";
		}
	}

} // namespace end

std::string const
generator_error_page_(uint32_t const& error_code) {
	std::string error_page;
	error_page += "<html>" CRLF "<head><title>";
	error_page += error_page_str__(static_cast<error_page_e>(error_code));
	error_page += "</title></head>" CRLF "<body>" CRLF "<center><h1>";
	error_page += error_page_str__(static_cast<error_page_e>(error_code));
	error_page += "</h1><p>SpaceX Default error page.</p></center>" CRLF "</body>" CRLF "</html>";
	return error_page;
}
