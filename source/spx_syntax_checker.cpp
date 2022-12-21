#include "spx_syntax_checker.hpp"
#include "spx_core_type.hpp"

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
			return error_("invalid uri start : request line, we only supported origin from :1*( \"/\"segment )");
		}

		case start_line__uri: {
			while (syntax_(usual_, *it)) { // NOTE: we didn't support query and fragment in usual_ now. maybe change
				++it;
			}
			switch (*it) {
			case '%': {
				if (syntax_(digit_alpha_, *(it + 1)) && syntax_(digit_alpha_, *(it + 2))) {
					it += 3;
					break;
				}
				return error_("invalid uri : request line : unmatched percent encoding");
			}
			case ' ': {
				++it;
				state = start_line__h;
				break;
			}
			case '#': {
				return error_("invalid uri : request line : we didn't support fragment");
			}
			case '?': {
				return error_("invalid uri : request line : we didn't support query");
			}
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

status
spx_chunked_syntax_start_line(std::string const&				  line,
							  uint32_t&							  chunk_size,
							  std::map<std::string, std::string>& chunk_ext) {
	std::string::const_iterator it = line.begin();
	std::string					temp_str_key;
	std::string					empty_str;
	std::string					temp_str_value;
	std::stringstream			ss;
	uint8_t						f_quoted_open = 0;

	enum {
		chunked_start = 0,
		chunked_size,
		chunked_size_input,
		chunked_BWS_before_ext,
		chunked_semicolon,
		chunked_BWS_before_ext_name,
		chunked_ext_name,
		chunked_BWS_after_ext_name,
		chunked_equal,
		chunked_BWS_before_ext_value,
		chunked_ext_quoted_open,
		chunked_ext_value,
		chunked_ext_quoted_close,
		chunked_almost_done,
		chunked_last_chunk,
		chunked_done
	} state,
		next_state;

	state = chunked_start;
	while (state != chunked_done) {
		switch (state) {
		case chunked_start: {
			if (syntax_(digit_alpha_, static_cast<uint8_t>(*it))) {
				state = chunked_size;
				break;
			}
			return error_("invalid chunked start line : chunked_start");
		}

		case chunked_last_chunk: {
			switch (*it) {
			case '\r':
				state = chunked_almost_done;
				break;
			case ' ':
				state = chunked_BWS_before_ext;
				break;
			case ';':
				state = chunked_semicolon;
				break;
			default:
				return error_("invalid chunked start line last chunk : chunked_start");
			}
			break;
		}

		case chunked_size: {
			while (syntax_(digit_alpha_, static_cast<uint8_t>(*it))) {
				temp_str_key.push_back(*it);
				++it;
			}
			switch (*it) {
			case ';':
				next_state = chunked_semicolon;
				state	   = chunked_size_input;
				break;
			case ' ':
				next_state = chunked_BWS_before_ext;
				state	   = chunked_size_input;
				break;
			case '\r':
				next_state = chunked_almost_done;
				state	   = chunked_size_input;
				break;
			default:
				return error_("invalid chunked start line number : chunked_start");
			}
			break;
		}

		case chunked_size_input: {
			ss << std::hex << temp_str_key;
			ss >> chunk_size;
			if (chunk_size == 0) {
				state = chunked_last_chunk;
			} else {
				state = next_state;
			}
			temp_str_key.clear();
		}

		case chunked_BWS_before_ext: {
			while (*it == ' ') {
				++it;
			}
			switch (*it) {
			case ';':
				state = chunked_semicolon;
				break;
			case '\r':
				state = chunked_almost_done;
				break;
			default:
				return error_("invalid chunked start line : BWS_before_ext : chunked_start");
			}
			break;
		}

		case chunked_semicolon: {
			temp_str_key.clear();
			temp_str_value.clear();
			++it;
			state = chunked_BWS_before_ext_name;
			break;
		}

		case chunked_BWS_before_ext_name: {
			while (*it == ' ') {
				++it;
			}
			if (*it == '\r' || *it == '=' || *it == ';') {
				return error_("invalid chunked start line : BWS_before_ext_name : chunked_start");
			}
			state = chunked_ext_name;
			break;
		}

		case chunked_ext_name: {
			while (syntax_(name_token_, static_cast<uint8_t>(*it))) {
				temp_str_key.push_back(*it);
				++it;
			}
			switch (*it) {
			case ' ':
				state = chunked_BWS_after_ext_name;
				break;
			case '=':
				state = chunked_equal;
				break;
			case '\r':
				chunk_ext.insert(std::make_pair(temp_str_key, empty_str));
				state = chunked_almost_done;
				break;
			case ';':
				chunk_ext.insert(std::make_pair(temp_str_key, empty_str));
				state = chunked_semicolon;
				break;
			default:
				return error_("invalid chunked start line : ext_name : chunked_start");
			}
			break;
		}

		case chunked_BWS_after_ext_name: {
			while (*it == ' ') {
				++it;
			}
			switch (*it) {
			case '=':
				state = chunked_equal;
				break;
			case ';':
				chunk_ext.insert(std::make_pair(temp_str_key, empty_str));
				state = chunked_semicolon;
				break;
			case '\r':
				chunk_ext.insert(std::make_pair(temp_str_key, empty_str));
				state = chunked_almost_done;
				break;
			default:
				return error_("invalid chunked start line : BWS_after_ext_name : chunked_start");
			}
			break;
		}

		case chunked_equal: {
			++it;
			state = chunked_BWS_before_ext_value;
			break;
		}

		case chunked_BWS_before_ext_value: {
			while (*it == ' ') {
				++it;
			}
			switch (*it) {
			case '\r':
				return error_("invalid chunked start line : BWS_before_ext_value : chunked_start");
			case '"':
				state = chunked_ext_quoted_open;
				break;
			case '\'':
				state = chunked_ext_quoted_open;
				break;
			default:
				state = chunked_ext_value;
			}
			break;
		}

		case chunked_ext_quoted_open: {
			if (*it == '"') {
				f_quoted_open |= 2;
			} else if (*it == '\'') {
				f_quoted_open |= 1;
			}
			++it;
			state = chunked_ext_value;
			break;
		}

		case chunked_ext_value: {
			while (syntax_(name_token_, static_cast<uint8_t>(*it))) {
				if (*it == '\'' && f_quoted_open & 1) {
					state = chunked_ext_quoted_close;
					break;
				}
				temp_str_value.push_back(*it);
				++it;
			}
			switch (*it) {
			case ' ':
				state = chunked_BWS_before_ext;
				chunk_ext.insert(std::make_pair(temp_str_key, temp_str_value));
				break;
			case ';':
				state = chunked_semicolon;
				chunk_ext.insert(std::make_pair(temp_str_key, temp_str_value));
				break;
			case '\r':
				state = chunked_almost_done;
				chunk_ext.insert(std::make_pair(temp_str_key, temp_str_value));
				break;
			case '"': {
				if (f_quoted_open & 1) {
					temp_str_value.push_back(*it);
					++it;
					break;
				} else if (f_quoted_open & 2) {
					state = chunked_ext_quoted_close;
					break;
				}
				state = chunked_ext_quoted_open;
				break;
			}
			case '\'': {
				if (f_quoted_open & 2) {
					temp_str_value.push_back(*it);
					++it;
					break;
				} else if (f_quoted_open & 1) {
					state = chunked_ext_quoted_close;
					break;
				}
				state = chunked_ext_quoted_open;
				break;
			}
			default:
				return error_("invalid chunked ext : ext_value : chunked_start");
			}
			break;
		}

		case chunked_ext_quoted_close: {
			if (*it == '"' && f_quoted_open & 2) {
				f_quoted_open &= ~2;
			} else if (*it == '\'' && f_quoted_open & 1) {
				f_quoted_open &= ~1;
			} else {
				return error_("invalid chunked ext : ext_quoted_close : chunked_start");
			}
			++it;
			if (syntax_(name_token_, static_cast<uint8_t>(*it))) {
				state = chunked_ext_value;
				break;
			}
			switch (*it) {
			case '\'':
				state = chunked_ext_quoted_open;
				break;
			case '"':
				state = chunked_ext_quoted_open;
				break;
			case ';':
				state = chunked_semicolon;
				chunk_ext.insert(std::make_pair(temp_str_key, temp_str_value));
				break;
			case ' ':
				state = chunked_BWS_before_ext;
				chunk_ext.insert(std::make_pair(temp_str_key, temp_str_value));
				break;
			case '\r':
				state = chunked_almost_done;
				chunk_ext.insert(std::make_pair(temp_str_key, temp_str_value));
				break;
			default:
				return error_("invalid chunked ext : ext_quoted_close : chunked_start");
			}
			break;
		}

		case chunked_almost_done: {
			++it;
			if (*it == '\n') {
				state = chunked_done;
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
spx_chunked_syntax_data_line(std::string const&					 line,
							 uint32_t&							 chunk_size,
							 std::vector<char>&					 data_store,
							 std::map<std::string, std::string>& trailer_section) {

	std::string::const_iterator it = line.begin();

	if (chunk_size == 0) {
		std::string temp_str_key;
		std::string temp_str_value;

		enum {
			last_start = 0,
			last_trailer_start,
			last_trailer_key,
			last_OWS_before_value,
			last_trailer_value,
			// last_OWS_after_value,
			last_almost_done,
			last_done
		} state_last_chunk;

		state_last_chunk = last_start;

		while (state_last_chunk != last_done) {
			switch (state_last_chunk) {
			case last_start: {
				temp_str_key.clear();
				temp_str_value.clear();
				if (*it == '\r') {
					state_last_chunk = last_almost_done;
					break;
				}
				state_last_chunk = last_trailer_key;
				break;
			}

			case last_trailer_key: {
				while (syntax_(name_token_, static_cast<uint8_t>(*it))) {
					temp_str_key.push_back(*it);
					++it;
				}
				spx_log_(temp_str_key);
				if (*it == ':') {
					state_last_chunk = last_OWS_before_value;
					++it;
					break;
				}
				return error_("invalid last chunked key : trailer");
			}

			case last_OWS_before_value: {
				while (*it == ' ') {
					++it;
				}
				if (*it == '\r') {
					return error_("invalid last chunked value : \\r : trailer");
				}
				state_last_chunk = last_trailer_value;
				break;
			}

			case last_trailer_value: {
				while (syntax_(field_value_, static_cast<uint8_t>(*it))) {
					temp_str_value.push_back(*it);
					++it;
				}
				spx_log_(temp_str_value);
				if (*it == '\r') {
					state_last_chunk = last_almost_done;
					break;
				}
				spx_log_(*it);
				return error_("invalid last chunked value : unsupported char : trailer");
			}

			case last_almost_done: {
				++it;
				if (*it == '\n') {
					if (temp_str_key.empty() && temp_str_value.empty()) {
						state_last_chunk = last_done;
						break;
					}
					trailer_section.insert(std::make_pair(temp_str_key, temp_str_value));
					++it;
					state_last_chunk = last_start;
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
	}
	return spx_ok;
}
