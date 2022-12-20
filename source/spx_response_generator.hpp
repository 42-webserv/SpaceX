#ifndef __SPX_RESPONSE_GENERATOR_HPP__
#define __SPX_RESPONSE_GENERATOR_HPP__
#pragma once

#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>

#include "spacex.hpp"
#include "spx_client_buffer.hpp"
#include "spx_core_type.hpp"

// This must change to file that loaded from CONFIG file
#define ERR_PAGE_URL "html/404error.html"

/* Status Codes */
namespace {
#define SERVER_HEADER_KEY "Server"
#define SERVER_HEADER_VALUE "SpaceX"
#define CONTENT_LENGTH "Content-Legnth"
#define CONTENT_TYPE "Content-Type";

#define MIME_TYPE_HTML "text/html"
#define MIME_TYPE_JPG "image/jpg"
#define MIME_TYPE_JPEG "image/jpeg"
#define MIME_TYPE_PNG "image/png"
#define MIME_TYPE_BMP "image/bmp"
#define MIME_TYPE_TEXT "text"
#define MIME_TYPE_DEFUALT "application/octet-stream"

#define HTTP_STATUS_MAP(XX)                                                   \
	XX(100, CONTINUE, Continue)                                               \
	XX(101, SWITCHING_PROTOCOLS, Switching Protocols)                         \
	XX(102, PROCESSING, Processing)                                           \
	XX(200, OK, OK)                                                           \
	XX(201, CREATED, Created)                                                 \
	XX(202, ACCEPTED, Accepted)                                               \
	XX(203, NON_AUTHORITATIVE_INFORMATION, Non - Authoritative Information)   \
	XX(204, NO_CONTENT, No Content)                                           \
	XX(205, RESET_CONTENT, Reset Content)                                     \
	XX(206, PARTIAL_CONTENT, Partial Content)                                 \
	XX(207, MULTI_STATUS, Multi - Status)                                     \
	XX(208, ALREADY_REPORTED, Already Reported)                               \
	XX(226, IM_USED, IM Used)                                                 \
	XX(300, MULTIPLE_CHOICES, Multiple Choices)                               \
	XX(301, MOVED_PERMANENTLY, Moved Permanently)                             \
	XX(302, FOUND, Found)                                                     \
	XX(303, SEE_OTHER, See Other)                                             \
	XX(304, NOT_MODIFIED, Not Modified)                                       \
	XX(305, USE_PROXY, Use Proxy)                                             \
	XX(307, TEMPORARY_REDIRECT, Temporary Redirect)                           \
	XX(308, PERMANENT_REDIRECT, Permanent Redirect)                           \
	XX(400, BAD_REQUEST, Bad Request)                                         \
	XX(401, UNAUTHORIZED, Unauthorized)                                       \
	XX(402, PAYMENT_REQUIRED, Payment Required)                               \
	XX(403, FORBIDDEN, Forbidden)                                             \
	XX(404, NOT_FOUND, Not Found)                                             \
	XX(405, METHOD_NOT_ALLOWED, Method Not Allowed)                           \
	XX(406, NOT_ACCEPTABLE, Not Acceptable)                                   \
	XX(407, PROXY_AUTHENTICATION_REQUIRED, Proxy Authentication Required)     \
	XX(408, REQUEST_TIMEOUT, Request Timeout)                                 \
	XX(409, CONFLICT, Conflict)                                               \
	XX(410, GONE, Gone)                                                       \
	XX(411, LENGTH_REQUIRED, Length Required)                                 \
	XX(412, PRECONDITION_FAILED, Precondition Failed)                         \
	XX(413, PAYLOAD_TOO_LARGE, Payload Too Large)                             \
	XX(414, URI_TOO_LONG, URI Too Long)                                       \
	XX(415, UNSUPPORTED_MEDIA_TYPE, Unsupported Media Type)                   \
	XX(416, RANGE_NOT_SATISFIABLE, Range Not Satisfiable)                     \
	XX(417, EXPECTATION_FAILED, Expectation Failed)                           \
	XX(421, MISDIRECTED_REQUEST, Misdirected Request)                         \
	XX(422, UNPROCESSABLE_ENTITY, Unprocessable Entity)                       \
	XX(423, LOCKED, Locked)                                                   \
	XX(424, FAILED_DEPENDENCY, Failed Dependency)                             \
	XX(426, UPGRADE_REQUIRED, Upgrade Required)                               \
	XX(428, PRECONDITION_REQUIRED, Precondition Required)                     \
	XX(429, TOO_MANY_REQUESTS, Too Many Requests)                             \
	XX(431, REQUEST_HEADER_FIELDS_TOO_LARGE, Request Header Fields Too Large) \
	XX(451, UNAVAILABLE_FOR_LEGAL_REASONS, Unavailable For Legal Reasons)     \
	XX(500, INTERNAL_SERVER_ERROR, Internal Server Error)                     \
	XX(501, NOT_IMPLEMENTED, Not Implemented)                                 \
	XX(502, BAD_GATEWAY, Bad Gateway)                                         \
	XX(503, SERVICE_UNAVAILABLE, Service Unavailable)                         \
	XX(504, GATEWAY_TIMEOUT, Gateway Timeout)                                 \
	XX(505, HTTP_VERSION_NOT_SUPPORTED, HTTP Version Not Supported)           \
	XX(506, VARIANT_ALSO_NEGOTIATES, Variant Also Negotiates)                 \
	XX(507, INSUFFICIENT_STORAGE, Insufficient Storage)                       \
	XX(508, LOOP_DETECTED, Loop Detected)                                     \
	XX(510, NOT_EXTENDED, Not Extended)                                       \
	XX(511, NETWORK_AUTHENTICATION_REQUIRED, Network Authentication Required)

	enum http_status {
#define XX(num, name, string) HTTP_STATUS_##name = num,
		HTTP_STATUS_MAP(XX)
#undef XX
	};

	std::string
	http_status_str(enum http_status s) {
		switch (s) {
#define XX(num, name, string) \
	case HTTP_STATUS_##name:  \
		return #string;
			HTTP_STATUS_MAP(XX)
#undef XX
		default:
			return "<unknown>";
		}
	};
}
struct Response {
private:
	typedef std::pair<std::string, std::string> header;
	std::vector<header>							headers_;
	int											version_minor_;
	int											version_major_;
	unsigned int								status_code_;
	std::string									status_;
	bool										keep_alive_ = true;

	std::string
	make_to_string() const;
	int
	file_open(const char* dir);
	void
	setContentLength(int fd);
	void
	setContentType(std::string uri);
	std::string
	handle_static_error_page();

public:
	std::string
	make_error_response(std::vector<struct kevent>& change_list, ClientBuffer& client_buffer, http_status error_code);
	std::string
	setting_response_header(std::vector<struct kevent>& change_list, ClientBuffer& client_buffer);
};

#endif