#include "syntax.hpp"
#include <_types/_uint8_t.h>
#include <iostream>

namespace {

	uint32_t usual_[] = {
		0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
		/* ?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
		0x7fffb7d6, /* 0111 1111 1111 1111  1011 0111 1101 0110 */
		/* _^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
		0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
		/*  ~}| {zyx wvut srqp  onml kjih gfed cba` */
		0x7fffffff, /* 0111 1111 1111 1111  1111 1111 1111 1111 */
		0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
		0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
		0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
		0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
	};

	uint32_t start_line_[] = {
		0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
		/* ?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
		0x00002000, /* 0000 0000 0000 0000  0010 0000 0000 0000 */
		/* _^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
		0x87fffffe, /* 1000 0111 1111 1111  1111 1111 1111 1110 */
		/*  ~}| {zyx wvut srqp  onml kjih gfed cba` */
		0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
		0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
		0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
		0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
		0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
	};

	uint8_t lowcase_[] = {
		"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
		"\0\0\0\0\0\0\0\0\0\0\0\0\0-\0\0"
		"0123456789\0\0\0\0\0\0"
		"\0abcdefghijklmnopqrstuvwxyz\0\0\0\0\0"
		"\0abcdefghijklmnopqrstuvwxyz\0\0\0\0\0"
		"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
		"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
		"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
		"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
	};

	inline status
	error_(const char* msg) {
// throw(msg);
#ifdef DEBUG
		std::cout << msg << std::endl;
#endif
		return spx_error;
	}

	inline int
	syntax_(const uint32_t table[8], int c) {
		return (table[((unsigned char)c >> 5)] & (1U << (((unsigned char)c) & 0x1f)));
	}

} // namespace

status
spx_http_syntax_start_line_request(std::string const& line) {
	std::string::const_iterator it = line.begin();
	size_t						temp_len;
	enum {
		spx_start = 0,
		spx_method,
		spx_sp_before_uri,
		spx_uri_start,
		spx_uri,
		spx_proto,
		spx_almost_done,
		spx_done
	} state;

	state = spx_start;

	while (state != spx_done) {
		switch (state) {
		case spx_start: {
			if (syntax_(start_line_, *it)) {
				state = spx_method;
				// add request start point to struct
				break;
			}
			return error_("invalid line start : request line");
		}

		case spx_method: { // check method
			temp_len = line.find(" ", 0);
			switch (temp_len) {
			case 3:
				if (line.find("GET", 0, 3) != std::string::npos) {
					// add method to struct or class_member
					state = spx_sp_before_uri;
					break;
				}
				if (line.find("PUT", 0, 3) != std::string::npos) {
					// add method to struct or class_member
					state = spx_sp_before_uri;
				}
				break;

			case 4:
				if (line.find("POST", 0, 4) != std::string::npos) {
					// add method to struct or class_member
					state = spx_sp_before_uri;
					break;
				}
				if (line.find("HEAD", 0, 4) != std::string::npos) {
					// add method to struct or class_member
					state = spx_sp_before_uri;
				}
				break;

			case 6:
				if (line.find("DELETE", 0, 5) != std::string::npos) {
					// add method to struct or class_member
					state = spx_sp_before_uri;
				}
				break;

			case 7:
				if (line.find("OPTIONS", 0, 5) != std::string::npos) {
					// add method to struct or class_member
					state = spx_sp_before_uri;
				}
				break;
			}
		}
			if (state != spx_sp_before_uri) {
				return error_("invalid method : request line");
			}
			it += temp_len;
			break;

		case spx_sp_before_uri: { // check uri
			if (*it == ' ') {
				++it;
				state = spx_uri_start;
				break;
			}
			return error_("invalid uri or method : request line");
		}

		case spx_uri_start: {
			if (*it == '/') {
				++it;
				state = spx_uri;
				break;
			}
			return error_("invalid uri start : request line");
		}

		case spx_uri: { // start uri check
			if (syntax_(usual_, *it)) {
				++it;
				break;
			}
			if (*it == ' ') {
				++it;
				state = spx_proto;
				break;
			}
			return error_("invalid uri ' ' or syntax_usual_error : request line");
		}

		case spx_proto: {
			if (line.compare(it - line.begin(), 8, "HTTP/1.1") == 0) {
				// add proto to struct or class_member
				it += 8;
				state = spx_almost_done;
				break;
			}
			return error_("invalid http version or end line : request line");
		}

		case spx_almost_done: {
			if (line.find("\r\n", it - line.begin(), 2) != std::string::npos) {
				state = spx_done;
				break;
			}
			return error_("invalid request end line : request line");
		}

		default:
			return error_("invalid request : request line");
		}
	}
	return spx_ok;
}

status
spx_http_syntax_header_line(std::string const& line) {
	std::string::const_iterator it = line.begin();
	uint8_t						c;
	enum {
		spx_start = 0,
		spx_key,
		spx_sp_before_value,
		spx_value,
		spx_sp_after_value,
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
			c = lowcase_[*it];
			if (c) {
				// add key to struct or class_member
				++it;
				break;
			}
			switch (*it) {
			case ':':
				state = spx_sp_before_value;
				++it;
				break;

			case '\r':
				++it;
				state = spx_almost_done;
				break;
			}
			if (state != spx_sp_before_value) {
				return error_("invalid key : header");
			}
			break;
		}

		case spx_sp_before_value: {
			switch (*it) {
			case ' ':
				++it;
				break;

			default:
				state = spx_value;
			}
			break;
		}

		case spx_value: {
			switch (*it) {
			case ' ':
				state = spx_sp_after_value;
				++it;
				break;

			case '\r':
				++it;
				state = spx_almost_done;
				break;

			case '\0':
				return error_("invalid value NULL : header");

			default:
				++it;
			}
			break;
		}

		case spx_sp_after_value: {
			switch (*it) {
			case ' ':
				++it;
				break;

			default:
				state = spx_value;
			}
			break;
		}

		case spx_almost_done: {
			if (*it == '\n') {
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
