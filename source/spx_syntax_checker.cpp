#include "spx_syntax_checker.hpp"
#include <sys/types.h>

namespace {

	inline status
	error__(const char* msg) {
#ifdef DEBUG
		std::cout << COLOR_RED << msg << COLOR_RESET << std::endl;
#else
		(void)msg;
#endif
		return spx_error;
	}

	inline status
	error_flag__(const char* msg, int& flag) {
#ifdef DEBUG
		std::cout << COLOR_RED << msg << COLOR_RESET << std::endl;
#else
		(void)msg;
#endif
		flag = REQ_UNDEFINED;
		return spx_error;
	}

} // namespace

status
spx_http_syntax_start_line(std::string const& line,
						   int&				  req_type) {

	if (line.length() > 8192) {
		return error_flag__("invalid line start : too long", req_type);
	}

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
		start_line__http_protocol_version,
		start_line__almost_done,
		start_line__done
	} state;

	state = start_line__start;

	spx_log_("\n______request_syntax_start_line______");
	spx_log_(line);

	while (state != start_line__done) {
		switch (state) {
		case start_line__start: {
			if (syntax_(alpha_upper_case_, static_cast<uint8_t>(*it))) {
				state = start_line__method;
				break;
			}
			return error_flag__("invalid line start : request line", req_type);
		}

		case start_line__method: {
			while (syntax_(alpha_upper_case_, static_cast<uint8_t>(*it))) {
				temp_str.push_back(*it);
				++it;
				++size_count;
			}
			switch (size_count) {
			case 3: {
				if (temp_str == "GET") {
					req_type = REQ_GET;
					break;
				} else if (temp_str == "PUT") {
					req_type = REQ_PUT;
					break;
				}
				return error_flag__("invalid line start : METHOD - 3", req_type);
			}
			case 4: {
				if (temp_str == "POST") {
					req_type = REQ_POST;
					break;
				} else if (temp_str == "HEAD") {
					req_type = REQ_HEAD;
					break;
				}
				return error_flag__("invalid line start : METHOD - 4", req_type);
			}
			case 6: {
				if (temp_str == "DELETE") {
					req_type = REQ_DELETE;
					break;
				}
				return error_flag__("invalid line start : METHOD - 6", req_type);
			}
			default: {
				return error_flag__("invalid line start : METHOD", req_type);
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
			return error_flag__("invalid uri or method : request line", req_type);
		}

		case start_line__uri_start: {
			if (*it == '/') {
				++it;
				state = start_line__uri;
				break;
			}
			return error_flag__("invalid uri start : request line, we only supported origin from :1*( \"/\"segment )", req_type);
		}

		case start_line__uri: {
			while (syntax_(usual_, *it)) {
				++it;
			}
			switch (*it) {
			case '%': { // % HEXDIG HEXDIG process
				if (syntax_(digit_alpha_, *(it + 1)) && syntax_(digit_alpha_, *(it + 2))) {
					it += 3;
					break;
				}
				return error_flag__("invalid uri : request line : unmatched percent encoding", req_type);
			}
			case ' ': {
				++it;
				state = start_line__http_protocol_version;
				break;
			}
			case '#': {
				++it;
				break;
			}
			case '?': {
				++it;
				break;
			}
			default: {
				return error_flag__("invalid uri : request line", req_type);
			}
			}
			break;
		}

		case start_line__http_protocol_version: {
			if (line.compare(it - line.begin(), 8, "HTTP/1.1") == 0) {
				it += 8;
				state = start_line__almost_done;
				break;
			}
			return error_flag__("invalid http version or end line : request line", req_type);
		}

		case start_line__almost_done: {
			if (it == line.end()) {
				state = start_line__done;
				break;
			}
			return error_flag__("invalid request end line : request line", req_type);
		}

		default: {
			return error_flag__("invalid request : request line", req_type);
		}
		}
	}
	return spx_ok;
}

status
spx_http_syntax_header_line(std::string const& line) {

	if (line.length() > 8192) {
		return error__("invalid header field : too long");
	}

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

	spx_log_(line);

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
			return error__("invalid key : header");
		}

		case spx_OWS_before_value: {
			while (syntax_(ows_, static_cast<uint8_t>(*it))) {
				++it;
			}
			if (syntax_(vchar_, static_cast<uint8_t>(*it))) {
				state = spx_value;
				break;
			}
			return error__("invalid value : header");
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
			return error__("invalid value : header");
		}

		case spx_OWS_after_value: {
			while (syntax_(ows_, static_cast<uint8_t>(*it))) {
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
			return error__("invalid header end line : header");
		}

		default:
			return error__("invalid key or value : header");
		}
	}
	return spx_ok;
}

status
spx_chunked_syntax_start_line(std::string&						  line,
							  uint32_t&							  chunk_size,
							  std::map<std::string, std::string>& chunk_ext) {

	if (line.empty()) {
		return error__("invalid chunked start line : empty line");
	}

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
		chunked_done
	} state,
		next_state;

	state = chunked_start;

	while (state != chunked_done) {
		switch (state) {
		case chunked_start: {
			if (it != line.end() && syntax_(hexdigit_, static_cast<uint8_t>(*it))) {
				state = chunked_size;
				break;
			}
			return error__("invalid chunked start line : chunked_start");
		}

		case chunked_size: {
			while (it != line.end() && syntax_(hexdigit_, static_cast<uint8_t>(*it))) {
				temp_str_key.push_back(*it);
				++it;
			}
			if (it == line.end()) {
				ss << std::hex << temp_str_key;
				ss >> chunk_size;
				return spx_ok;
			}
			switch (*it) {
			case ';':
				next_state = chunked_semicolon;
				break;
			case ' ':
			case '\t':
				next_state = chunked_BWS_before_ext;
				break;
			default:
				return error__("invalid chunked start line number : chunked_size");
			}
			state = chunked_size_input;
			break;
		}

		case chunked_size_input: {
			ss << std::hex << temp_str_key;
			ss >> chunk_size; // XXX : can check last chunk
			state = next_state;
			temp_str_key.clear();
		}

		case chunked_BWS_before_ext: {
			while (it != line.end() && syntax_(ows_, static_cast<uint8_t>(*it))) {
				++it;
			}
			if (it == line.end()) {
				return error__("invalid chunked start line : end with null when expected extension : chunked_BWS_before_ext");
			}
			switch (*it) {
			case ';':
				state = chunked_semicolon;
				break;
			default:
				return error__("invalid chunked start line : unsupported end : chunked_BWS_before_ext");
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
			while (it != line.end() && syntax_(ows_, static_cast<uint8_t>(*it))) {
				++it;
			}
			if (it == line.end()) {
				return error__("invalid chunked start line : end with null when expected extension name : chunked_BWS_before_ext_name");
			}
			state = chunked_ext_name;
			break;
		}

		case chunked_ext_name: {
			while (it != line.end() && syntax_(name_token_, static_cast<uint8_t>(*it))) {
				temp_str_key.push_back(*it);
				++it;
			}
			if (it == line.end()) {
				return error__("invalid chunked start line : end with null when expected extension name : chunked_ext_name");
			}
			switch (*it) {
			case ' ':
			case '\t':
				state = chunked_BWS_after_ext_name;
				break;
			case '=':
				state = chunked_equal;
				break;
			case ';':
				chunk_ext.insert(std::make_pair(temp_str_key, empty_str));
				state = chunked_semicolon;
				break;
			default:
				return error__("invalid chunked start line : invalid end : chunked_ext_name");
			}
			break;
		}

		case chunked_BWS_after_ext_name: {
			while (it != line.end() && syntax_(ows_, static_cast<uint8_t>(*it))) {
				++it;
			}
			if (it == line.end()) {
				return error__("invalid chunked start line : end with null when expected value or another extension : chunked_BWS_after_ext_name");
			}
			switch (*it) {
			case '=':
				state = chunked_equal;
				break;
			case ';':
				chunk_ext.insert(std::make_pair(temp_str_key, empty_str));
				state = chunked_semicolon;
				break;
			default:
				return error__("invalid chunked start line : BWS_after_ext_name : chunked_start");
			}
			break;
		}

		case chunked_equal: {
			++it;
			state = chunked_BWS_before_ext_value;
			break;
		}

		case chunked_BWS_before_ext_value: {
			while (it != line.end() && syntax_(ows_, static_cast<uint8_t>(*it))) {
				++it;
			}
			if (it == line.end()) {
				return error__("invalid chunked start line : end with null when expected value : chunked_BWS_before_ext_value");
			}
			switch (*it) {
			case '"':
				f_quoted_open = 0;
				state		  = chunked_ext_quoted_open;
				break;
			default:
				state = chunked_ext_value;
			}
			break;
		}

		case chunked_ext_quoted_open: {
			if (*it == '"') {
				f_quoted_open |= 2;
			}
			++it;
			state = chunked_ext_value;
			break;
		}

		case chunked_ext_value: {
			while (it != line.end() && syntax_(name_token_, static_cast<uint8_t>(*it))) {
				if (*it == '"' && f_quoted_open & 2) {
					state = chunked_ext_quoted_close;
					break;
				}
				temp_str_value.push_back(*it);
				++it;
			}
			if (it == line.end()) {
				state = chunked_almost_done;
				chunk_ext.insert(std::make_pair(temp_str_key, temp_str_value));
				break;
			}
			switch (*it) {
			case ' ':
			case '\t':
				state = chunked_BWS_before_ext;
				chunk_ext.insert(std::make_pair(temp_str_key, temp_str_value));
				break;
			case ';':
				state = chunked_semicolon;
				chunk_ext.insert(std::make_pair(temp_str_key, temp_str_value));
				break;
			case '"': {
				if (f_quoted_open & 2) {
					state = chunked_ext_quoted_close;
					break;
				}
				state = chunked_ext_quoted_open;
				break;
			}
			default:
				return error__("invalid chunked ext : error char : chunked_ext_value");
			}
			break;
		}

		case chunked_ext_quoted_close: {
			f_quoted_open &= ~2;
			++it;
			if (it != line.end() && syntax_(name_token_, static_cast<uint8_t>(*it))) {
				state = chunked_ext_value;
				break;
			}
			if (it == line.end()) {
				state = chunked_almost_done;
				chunk_ext.insert(std::make_pair(temp_str_key, temp_str_value));
				break;
			}
			switch (*it) {
			case '"':
				state = chunked_ext_quoted_open;
				break;
			case ';':
				state = chunked_semicolon;
				chunk_ext.insert(std::make_pair(temp_str_key, temp_str_value));
				break;
			case ' ':
			case '\t':
				state = chunked_BWS_before_ext;
				chunk_ext.insert(std::make_pair(temp_str_key, temp_str_value));
				break;
			default:
				return error__("invalid chunked ext : ext_quoted_close : chunked_start");
			}
			break;
		}

		case chunked_almost_done: {
			state = chunked_done;
		}

		default:
			return error__("invalid chunked_start : chunked_start");
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
				return error__("invalid last chunked key : trailer");
			}

			case last_OWS_before_value: {
				while (*it == ' ') {
					++it;
				}
				if (*it == '\r') {
					return error__("invalid last chunked value : \\r : trailer");
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
				return error__("invalid last chunked value : unsupported char : trailer");
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
				return error__("invalid header end line : header");
			}

			default:
				return error__("invalid chunked : chunked_data");
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
					return error__("invalid chunked end line : chunked_data");
				}
				if (chunk_size != 0)
					return spx_need_more;
			}

			default:
				return error__("invalid chunked_data : chunked_data");
			}
		}
	}
	return spx_ok;
}
