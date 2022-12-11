#include "syntax.hpp"
#include <_types/_uint8_t.h>
#include <iostream>
#include <sstream>
#include <string>

namespace {

	uint32_t usual_[] = {
		0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
		/*				?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
		0x7fffb7d6, /*	0111 1111 1111 1111  1011 0111 1101 0110 */
		/*				_^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
		0xffffffff, /*	1111 1111 1111 1111  1111 1111 1111 1111 */
		/*				 ~}| {zyx wvut srqp  onml kjih gfed cba` */
		0x7fffffff, /*	0111 1111 1111 1111  1111 1111 1111 1111 */
		0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
		0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
		0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
		0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	};

	uint32_t start_line_[] = {
		0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
		/*				?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
		0x00002000, /*	0000 0000 0000 0000  0010 0000 0000 0000 */
		/*				_^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
		0x87fffffe, /*	1000 0111 1111 1111  1111 1111 1111 1110 */
		/*				 ~}| {zyx wvut srqp  onml kjih gfed cba` */
		0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
		0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
		0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
		0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
		0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	};

	uint32_t chunk_ext_[] = {
		0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
		/*				?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
		0xd7fffffe, /*	1101 0111 1111 1111  1111 1111 1111 1110 */
		/*				_^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
		0xffffffff, /*	1111 1111 1111 1111  1111 1111 1111 1111 */
		/*				 ~}| {zyx wvut srqp  onml kjih gfed cba` */
		0x7fffffff, /*	0111 1111 1111 1111  1111 1111 1111 1111 */
		0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
		0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
		0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
		0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	};

	uint8_t chunked_[] = {
		"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
		"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
		"0123456789\0\0\0\0\0\0"
		"\0ABCDEFGHIJKLMNOPQRSTUVWXYZ\0\0\0\0\0"
		"\0abcdefghijklmnopqrstuvwxyz\0\0\0\0\0"
		"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
		"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
		"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
		"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
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
	syntax_(const uint32_t table[8], uint8_t c) {
		return (table[(c >> 5)] & (1U << (c & 0x1f)));
	}

} // namespace

status
spx_http_syntax_start_line(std::string const& line) {
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
			if (state != spx_sp_before_uri) {
				return error_("invalid method : request line");
			}
			it += temp_len;
			break;
		}

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
			c = lowcase_[static_cast<uint8_t>(*it)];
			if (c) {
				// add key to struct or class_member
				++it;
				break;
			}
			if (*it == ':') {
				state = spx_sp_before_value;
				++it;
				break;
			}
			return error_("invalid key : header");
		}

		case spx_sp_before_value: {
			switch (*it) {
			case ' ':
				++it;
				break;
			default:
				// need to check syntax,  rfc token? or some other syntax?
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

status
spx_chunked_syntax_start_line(std::string const& line,
							  uint16_t&			 chunk_size,
							  std::string&		 chunk_ext,
							  uint8_t&			 ext_count) {
	std::string::const_iterator it = line.begin();
	std::string					count_size;
	std::stringstream			ss;
	enum {
		spx_start = 0,
		spx_size,
		spx_BWS_before_ext,
		spx_semicolon,
		spx_BWS_before_ext_name,
		spx_ext_name,
		spx_BWS_after_ext_name,
		spx_equal,
		spx_BWS_before_ext_value,
		spx_ext_value,
		spx_almost_done,
		spx_last_chunk,
		spx_done
	} state;

	state = spx_start;
	while (state != spx_done) {
		switch (state) {
		case spx_start: {
			if (chunked_[static_cast<uint8_t>(*it)]) {
				state = spx_size;
				if (*it == '0') {
					state = spx_last_chunk;
				}
				break;
			}
			return error_("invalid chunked start line : chunked_start");
		}

		case spx_last_chunk: {
			switch (*it) {
			case '0':
				++it;
				break;
			case '\r':
				++it;
				state = spx_almost_done;
				break;
			case ' ':
				state = spx_BWS_before_ext;
				break;
			case ';':
				state = spx_semicolon;
				break;
			default:
				return error_("invalid chunked start line last chunk : chunked_start");
			}
			break;
		}

		case spx_size: {
			if (chunked_[static_cast<uint8_t>(*it)]) {
				count_size.push_back(*it);
				++it;
				break;
			}
			switch (*it) {
			case ';':
				state = spx_semicolon;
				break;
			case ' ':
				++it;
				state = spx_BWS_before_ext;
				break;
			case '\r':
				++it;
				state = spx_almost_done;
				break;
			default:
				return error_("invalid chunked start line number : chunked_start");
			}
			break;
		}

		case spx_BWS_before_ext: {
			switch (*it) {
			case ' ':
				++it;
				break;
			case ';':
				state = spx_semicolon;
				break;
			default:
				return error_("invalid chunked start line : BWS_before_ext : chunked_start");
			}
			break;
		}

		case spx_semicolon: {
			++it;
			state = spx_BWS_before_ext_name;
			break;
		}

		case spx_BWS_before_ext_name: {
			switch (*it) {
			case ' ':
				++it;
				break;
			case '\r':
				return error_("invalid chunked start line : BWS_before_ext_name : chunked_start");
			case '=':
				return error_("invalid chunked start line : BWS_before_ext_name : chunked_start");
			case ';':
				return error_("invalid chunked start line : BWS_before_ext_name : chunked_start");
			default:
				state = spx_ext_name;
			}
			break;
		}

		case spx_ext_name: {
			switch (*it) {
			case ' ':
				state = spx_BWS_after_ext_name;
				break;
			case '=':
				state = spx_equal;
				break;
			case '\r':
				state = spx_almost_done;
				chunk_ext.push_back(';');
				++ext_count;
				++it;
				break;
			case ';':
				state = spx_semicolon;
				chunk_ext.push_back(';');
				++ext_count;
				break;
			default:
				chunk_ext.push_back(*it);
				++it;
				break;
			}
			break;
		}

		case spx_BWS_after_ext_name: {
			switch (*it) {
			case ' ':
				++it;
				break;
			case '=':
				state = spx_equal;
				break;
			case ';':
				state = spx_semicolon;
				chunk_ext.push_back(';');
				++ext_count;
				break;
			case '\r':
				state = spx_almost_done;
				chunk_ext.push_back(';');
				++ext_count;
				++it;
				break;
			default:
				return error_("invalid chunked start line : BWS_after_ext_name : chunked_start");
			}
			break;
		}

		case spx_equal: {
			chunk_ext.push_back('=');
			++it;
			state = spx_BWS_before_ext_value;
			break;
		}

		case spx_BWS_before_ext_value: {
			switch (*it) {
			case ' ':
				++it;
				break;
			case '\r':
				return error_("invalid chunked start line : BWS_before_ext_value : chunked_start");
			default:
				state = spx_ext_value;
			}
			break;
		}

		case spx_ext_value: {
			switch (*it) {
			case ' ':
				state = spx_BWS_before_ext;
				chunk_ext.push_back(';');
				++ext_count;
				break;
			case ';':
				state = spx_semicolon;
				chunk_ext.push_back(*it);
				++ext_count;
				break;
			case '\r':
				state = spx_almost_done;
				chunk_ext.push_back(';');
				++ext_count;
				++it;
				break;
			default:
				chunk_ext.push_back(*it);
				it++;
			}
			break;
		}

		case spx_almost_done: {
			if (*it == '\n') {
				state = spx_done;
				ss << std::hex << count_size;
				ss >> chunk_size;
				break;
			}
			return error_("invalid chunked end line : chunked_start");
		}

		default:
			return error_("invalid chunked_start : chunked_start");
		}
	}

	return spx_ok;
}

status
spx_chunked_syntax_data_line(std::string const& line,
							 uint16_t&			chunk_size,
							 std::string&		data_store,
							 std::string&		trailer_section,
							 uint8_t&			trailer_count) {
	std::string::const_iterator it = line.begin();
	enum {
		last_start = 0,
		last_trailer_start,
		last_trailer_line,
		last_trailer_almost_done,
		last_trailer_done,
		last_almost_done,
		last_done
	} state_last_chunk;

	enum {
		data_start = 0,
		data_read,
		data_almost_done,
		data_done
	} state_data;

	if (chunk_size == 0) {
		state_last_chunk = last_start;
		while (state_last_chunk != data_done) {
			// add field-line syntax check
			// same as header field line = field-line
			switch (state_last_chunk) {

			default:
				return error_("invalid chunked : chunked_data");
			}
		}
		return spx_ok;
	} else {
		state_data = data_start;
		while (state_data != data_done) {
			switch (state_data) {
			case data_start: {
				data_store.push_back(*it);
				--chunk_size;
				++it;
				state_data = data_read;
				break;
			}

			case data_read: {
				while (chunk_size > 0) {
					data_store.push_back(*it);
					++it;
					--chunk_size;
					if (it == line.end()) {
						state_data = data_almost_done;
						break;
					}
				}
				break;
			}

			case data_almost_done: {
				if (chunk_size == 0) {
					if (*it == '\r' && *(it + 1) == '\n') {
						state_data = data_done;
						break;
					}
					return error_("invalid chunked end line : chunked_data");
				}
				if (chunk_size != 0)
					return spx_need_more;
			}

			default:
				return error_("invalid chunked_data : chunked_data");
			}
		}
		return spx_ok;
	}
}
