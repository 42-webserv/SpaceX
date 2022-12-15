#include "chunked.hpp"
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
			if (syntax_(digit_alpha_, static_cast<uint8_t>(*it))) {
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
			if (syntax_(digit_alpha_, static_cast<uint8_t>(*it))) {
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
			if (syntax_(name_token_, static_cast<uint8_t>(*it))) {
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
			if (syntax_(name_token_, static_cast<uint8_t>(*it))) {
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
			if (syntax_(name_token_, static_cast<uint8_t>(*it))) {
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
				if (syntax_(name_token_, static_cast<uint8_t>(*it))) {
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
				if (syntax_(vchar_, static_cast<uint8_t>(*it))) {
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
	}
	return spx_ok;
}
