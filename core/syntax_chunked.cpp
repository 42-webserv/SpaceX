#include "syntax_chunked.hpp"
#include "core_type.hpp"
#include <sstream>

#ifdef SYNTAX_DEBUG
#include <iostream>
#endif

namespace {

	inline status
	error_(const char* msg) {

#ifdef SYNTAX_DEBUG
		std::cout << "\033[1;31m" << msg << "\033[0m"
				  << " : "
				  << "\033[1;33m"
				  << ""
				  << "\033[0m" << std::endl;
#endif
		return spx_error;
	}

	template <typename T>
	inline void
	spx_log_(T msg) {
#ifdef SYNTAX_DEBUG
		std::cout << "\033[1;32m" << msg << ";"
				  << "\033[0m" << std::endl;
#else
		(void)msg;
#endif
	}

} // namespace

status
spx_chunked_syntax_start_line(std::string const&				  line,
							  uint16_t&							  chunk_size,
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
							 uint16_t&							 chunk_size,
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
