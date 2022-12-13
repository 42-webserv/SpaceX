#include "syntax.hpp"
#include <iostream>
#include <sstream>

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

	uint32_t field_name_token_[] = {
		0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
		/*				?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
		0x03ff6cfa, /*	0000 0011 1111 1111  0110 1100 1111 1010 */
		/*				_^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
		0xc7fffffe, /*	1100 0111 1111 1111  1111 1111 1111 1110 */
		/*				 ~}| {zyx wvut srqp  onml kjih gfed cba` */
		0x57ffffff, /*	0101 0111 1111 1111  1111 1111 1111 1111 */
		0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
		0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
		0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
		0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	};

	uint32_t field_value_vchar_[] = {
		0x00000200, /*	0000 0000 0000 0000  0000 0010 0000 0000 */
		/*				?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
		0xfffffffe, /*	1111 1111 1111 1111  1111 1111 1111 1111 */
		/*				_^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
		0xffffffff, /*	1111 1111 1111 1111  1111 1111 1111 1111 */
		/*				 ~}| {zyx wvut srqp  onml kjih gfed cba` */
		0x7fffffff, /*	0111 1111 1111 1111  1111 1111 1111 1111 */
		0xffffffff, /*	1111 1111 1111 1111  1111 1111 1111 1111 */ /* obs-text */
		0xffffffff, /*	1111 1111 1111 1111  1111 1111 1111 1111 */
		0xffffffff, /*	1111 1111 1111 1111  1111 1111 1111 1111 */
		0xffffffff, /*	1111 1111 1111 1111  1111 1111 1111 1111 */
	};

	uint32_t chunked_size_[] = {
		0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
		/*				?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
		0x03ff0000, /*	0000 0011 1111 1111  0000 0000 0000 0000 */
		/*				_^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
		0x07fffffe, /*	0000 0111 1111 1111  1111 1111 1111 1110 */
		/*				 ~}| {zyx wvut srqp  onml kjih gfed cba` */
		0x07fffffe, /*	0000 0111 1111 1111  1111 1111 1111 1110 */
		0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
		0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
		0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
		0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	};

	inline status
	error_(const char* msg) {
// throw(msg);
#ifdef DEBUG
		std::cout << msg << std::endl;
#endif
		return spx_error;
	}

	inline uint8_t
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
			if (syntax_(start_line_, static_cast<uint8_t>(*it))) {
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
			if (syntax_(usual_, static_cast<uint8_t>(*it))) {
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
			if (syntax_(field_name_token_, static_cast<uint8_t>(*it))) {
				++it;
				break;
			}
			if (*it == ':') {
				state = spx_OWS_before_value;
				++it;
				break;
			}
			return error_("invalid key : header");
		}

		case spx_OWS_before_value: {
			switch (*it) {
			case ' ':
				++it;
				break;
			case '\r':
				return error_("invalid value : header");
			default:
				state = spx_value;
			}
			break;
		}

		case spx_value: {
			if (syntax_(field_value_vchar_, static_cast<uint8_t>(*it))) {
				++it;
				break;
			}
			// need check field-value syntax
			switch (*it) {
			case ' ':
				state = spx_OWS_after_value;
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

		case spx_OWS_after_value: {
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
	uint8_t						f_quoted_open = 0;
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
		spx_ext_quoted_open,
		spx_ext_value,
		spx_ext_quoted_close,
		spx_almost_done,
		spx_last_chunk,
		spx_done
	} state;

	state = spx_start;
	while (state != spx_done) {
		switch (state) {
		case spx_start: {
			if (syntax_(chunked_size_, static_cast<uint8_t>(*it))) {
				state = spx_size;
				break;
			}
			return error_("invalid chunked start line : chunked_start");
		}

		case spx_last_chunk: {
			switch (*it) {
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
			if (syntax_(chunked_size_, static_cast<uint8_t>(*it))) {
				count_size.push_back(*it);
				++it;
				break;
			}
			switch (*it) {
			case ';':
				state = spx_semicolon;
				ss << std::hex << count_size;
				ss >> chunk_size;
				if (chunk_size == 0) {
					state = spx_last_chunk;
				}
				break;
			case ' ':
				++it;
				state = spx_BWS_before_ext;
				ss << std::hex << count_size;
				ss >> chunk_size;
				if (chunk_size == 0) {
					state = spx_last_chunk;
				}
				break;
			case '\r':
				++it;
				state = spx_almost_done;
				ss << std::hex << count_size;
				ss >> chunk_size;
				if (chunk_size == 0) {
					state = spx_last_chunk;
				}
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
			if (syntax_(field_name_token_, static_cast<uint8_t>(*it))) {
				chunk_ext.push_back(*it);
				++it;
				break;
			}
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
				return error_("invalid chunked start line : ext_name : chunked_start");
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
			case '"':
				state = spx_ext_quoted_open;
				break;
			case '\'':
				state = spx_ext_quoted_open;
				break;
			default:
				state = spx_ext_value;
			}
			break;
		}

		case spx_ext_quoted_open: {
			if (*it == '"') {
				f_quoted_open |= 2;
			} else if (*it == '\'') {
				f_quoted_open |= 1;
			}
			++it;
			state = spx_ext_value;
			break;
		}

		case spx_ext_value: {
			if (syntax_(field_name_token_, static_cast<uint8_t>(*it))) {
				if (*it == '\'' && f_quoted_open & 1) {
					state = spx_ext_quoted_close;
					break;
				}
				chunk_ext.push_back(*it);
				++it;
				break;
			}
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
			case '"':
				if (f_quoted_open & 2) {
					state = spx_ext_quoted_close;
					break;
				}
				state = spx_ext_quoted_open;
				break;
			default:
				return error_("invalid chunked ext : ext_value : chunked_start");
			}
			break;
		}

		case spx_ext_quoted_close: {
			if (*it == '"' && f_quoted_open & 2) {
				f_quoted_open &= ~2;
			} else if (*it == '\'' && f_quoted_open & 1) {
				f_quoted_open &= ~1;
			} else {
				return error_("invalid chunked ext : ext_quoted_close : chunked_start");
			}
			++it;
			if (syntax_(field_name_token_, static_cast<uint8_t>(*it))) {
				state = spx_ext_value;
				break;
			}
			switch (*it) {
			case '\'':
				state = spx_ext_quoted_open;
				break;
			case '"':
				state = spx_ext_quoted_open;
				break;
			case ';':
				state = spx_semicolon;
				chunk_ext.push_back(*it);
				++ext_count;
				break;
			case ' ':
				state = spx_BWS_before_ext;
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
				return error_("invalid chunked ext : ext_quoted_close : chunked_start");
			}
			break;
		}

		case spx_almost_done: {
			if (*it == '\n') {
				state = spx_done;
				// ss << std::hex << count_size;
				// ss >> chunk_size;
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

	if (chunk_size == 0) {
		enum {
			last_start = 0,
			last_trailer_start,
			last_trailer_key,
			last_OWS_before_value,
			last_trailer_value,
			last_OWS_after_value,
			last_almost_done,
			last_done
		} state_last_chunk;

		std::string temp_trailer;
		std::string temp_OWS_after_value;

		state_last_chunk = last_start;

		while (state_last_chunk != last_done) {
			// add field-line syntax check
			// same as header field line = field-line
			switch (state_last_chunk) {
			case last_start: {
				if (*it == '\r') {
					state_last_chunk = last_almost_done;
					++it;
					break;
				}
				state_last_chunk = last_trailer_key;
				break;
			}

			case last_trailer_key: {
				if (syntax_(field_name_token_, static_cast<uint8_t>(*it))) {
					temp_trailer.push_back(*it);
					++it;
					break;
				}
				if (*it == ':') {
					state_last_chunk = last_OWS_before_value;
					temp_trailer.push_back(*it);
					++it;
					break;
				}
				return error_("invalid last chunked key : trailer");
			}

			case last_OWS_before_value: {
				switch (*it) {
				case ' ':
					++it;
					break;
				case '\r':
					return error_("invalid last chunked value : trailer");
				default:
					temp_trailer.push_back(' ');
					state_last_chunk = last_trailer_value;
				}
				break;
			}

			case last_trailer_value: {
				if (syntax_(field_value_vchar_, static_cast<uint8_t>(*it))) {
					temp_trailer.push_back(*it);
					++it;
					break;
				}
				switch (*it) {
				case ' ':
					state_last_chunk = last_OWS_after_value;
					break;
				case '\r':
					++it;
					state_last_chunk = last_almost_done;
					break;
				case '\0':
					return error_("invalid value NULL : header");
				default:
					++it;
				}
				break;
			}

			case last_OWS_after_value: {
				switch (*it) {
				case ' ':
					++it;
					temp_OWS_after_value.push_back(*it);
					break;
				case '\r':
					++it;
					state_last_chunk = last_almost_done;
					break;
				default:
					temp_trailer.append(temp_OWS_after_value);
					state_last_chunk = last_trailer_value;
				}
				break;
			}

			case last_almost_done: {
				if (*it == '\n') {
					state_last_chunk = last_done;
					trailer_section.append(temp_trailer);
					++trailer_count;
					break;
				}
				return error_("invalid header end line : header");
			}

			default:
				return error_("invalid chunked : chunked_data");
			}
		}
		return spx_ok;

	} else {
		enum {
			data_start = 0,
			data_read,
			data_almost_done,
			data_done
		} state_data;

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
				state_data = data_almost_done;
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
