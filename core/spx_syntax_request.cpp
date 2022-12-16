#include "spx_syntax_request.hpp"
#include "spx_core_type.hpp"
#include "spx_util_box.hpp"
#include <iostream>
#include <string>

namespace {

	inline status
	error_(const char* msg) {

#ifdef SYNTAX_DEBUG
		std::cout << "\033[1;31m" << msg << "\033[0m"
				  << " : "
				  << "\033[1;33m"
				  << ""
				  << "\033[0m" << std::endl;
#else
		(void)msg;
#endif
		return spx_error;
	}

} // namespace

status
spx_http_syntax_start_line(std::string const& line) {
	// spx_http_syntax_start_line(std::string const& line, t_client_buf& buf) {
	std::string::const_iterator it		   = line.begin();
	uint32_t					size_count = 0;
	std::string					temp_str;

	enum {
		start_line__start = 0,
		start_line__method,
		start_line__sp_before_uri,
		start_line__uri_start,
		start_line__uri,
		start_line__http_check,
		start_line__h,
		start_line__ht,
		start_line__htt,
		start_line__http,
		start_line__http_slash,
		start_line__http_major,
		start_line__http_dot,
		start_line__http_minor,
		start_line__almost_done,
		start_line__done
	} state;

	state = start_line__start;

	while (state != start_line__done) {
		switch (state) {
		case start_line__start: {
			if (syntax_(alpha_upper_case_, static_cast<uint8_t>(*it))) {
				state = start_line__method;
				break;
			}
			return error_("invalid line start : request line");
		}

		case start_line__method: {
			while (syntax_(alpha_upper_case_, static_cast<uint8_t>(*it))) {
				temp_str.push_back(*it);
				++it;
				++size_count;
			}
			switch (size_count) {
			case 3: {
				if (temp_str == "GET" || temp_str == "PUT") {
					break;
				}
				return error_("invalid method : 3 - request line");
			}
			case 4: {
				if (temp_str == "POST" || temp_str == "HEAD") {
					break;
				}
				return error_("invalid method : 4 - request line");
			}
			case 6: {
				if (temp_str == "DELETE") {
					break;
				}
				return error_("invalid method : 6 - request line");
			}
			case 7: {
				if (temp_str == "OPTIONS") {
					break;
				}
				return error_("invalid method : 7 - request line");
			}
			default: {
				return error_("invalid method : request line");
			}
			}
			state = start_line__sp_before_uri;
			break;
		}

		case start_line__sp_before_uri: {
			if (*it == ' ') {
				++it;
				state = start_line__uri_start;
				break;
			}
			return error_("invalid uri or method : request line");
		}

		case start_line__uri_start: {
			if (*it == '/') {
				++it;
				state = start_line__uri;
				break;
			}
			return error_("invalid uri start : request line");
		}

		case start_line__uri: {
			while (syntax_(usual_, *it)) {
				++it;
			}
			if (*it == ' ') {
				++it;
				state = start_line__h;
				break;
			}
			return error_("invalid uri : request line");
		}

		case start_line__h: {
			if (*it == 'H') {
				++it;
				state = start_line__ht;
				break;
			}
			return error_("invalid http version or end line : request line");
		}

		case start_line__ht: {
			if (*it == 'T') {
				++it;
				state = start_line__htt;
				break;
			}
			return error_("invalid http version or end line : request line");
		}

		case start_line__htt: {
			if (*it == 'T') {
				++it;
				state = start_line__http;
				break;
			}
			return error_("invalid http version or end line : request line");
		}

		case start_line__http: {
			if (*it == 'P') {
				++it;
				state = start_line__http_slash;
				break;
			}
			return error_("invalid http version or end line : request line");
		}

		case start_line__http_slash: {
			if (*it == '/') {
				++it;
				state = start_line__http_major;
				break;
			}
			return error_("invalid http version or end line : request line");
		}

		case start_line__http_major: {
			if (syntax_(digit_, static_cast<uint8_t>(*it))) {
				if (*it == '1') {
					++it;
					state = start_line__http_dot;
					break;
				}
				return error_("invalid http major version : request line");
			}
			return error_("invalid http major version : not number : request line");
		}

		case start_line__http_dot: {
			if (*it == '.') {
				++it;
				state = start_line__http_minor;
				break;
			}
			return error_("invalid http version or end line : request line");
		}

		case start_line__http_minor: {
			if (syntax_(digit_, static_cast<uint8_t>(*it))) {
				if (*it == '0' || *it == '1') {
					++it;
					state = start_line__almost_done;
					break;
				}
				return error_("invalid http minor version : request line");
			}
			return error_("invalid http minor version : not number : request line");
		}

		case start_line__almost_done: {
			if (it == line.end()) {
				state = start_line__done;
				break;
			}
			return error_("invalid request end line : request line");
		}

		default: {
			return error_("invalid request : request line");
		}
		}
	}
	return spx_ok;
}

status
spx_http_syntax_header_line(std::string const& line) {
	std::string::const_iterator it = line.begin();
	enum {
		spx_start = 0,
		spx_key,
		spx_OWS_before_value,
		spx_value,
		spx_OWS_after_value,
		spx_almost_done,
		spx_done
	} state;

	state = spx_start;

	while (state != spx_done) {
		switch (state) {
		case spx_start: {
			switch (*it) {
			case '\r':
				++it;
				state = spx_almost_done;
				break;
			default:
				state = spx_key;
			}
			break;
		}

		case spx_key: {
			while (syntax_(name_token_, static_cast<uint8_t>(*it))) {
				++it;
			}
			if (*it == ':') {
				state = spx_OWS_before_value;
				++it;
				break;
			}
			return error_("invalid key : header");
		}

		case spx_OWS_before_value: {
			while (*it == ' ') {
				++it;
			}
			if (syntax_(vchar_, static_cast<uint8_t>(*it))) {
				state = spx_value;
				break;
			}
			return error_("invalid value : header");
		}

		case spx_value: {
			while (syntax_(vchar_, static_cast<uint8_t>(*it))) {
				++it;
			}
			if (it == line.end()) {
				state = spx_almost_done;
				break;
			}
			if (*it == ' ') {
				state = spx_OWS_after_value;
				++it;
				break;
			}
			return error_("invalid value : header");
		}

		case spx_OWS_after_value: {
			while (*it == ' ') {
				++it;
			}
			state = spx_value;
			break;
		}

		case spx_almost_done: {
			if (it == line.end()) {
				state = spx_done;
				break;
			}
			return error_("invalid header end line : header");
		}

		default:
			return error_("invalid key or value : header");
		}
	}
	return spx_ok;
}
