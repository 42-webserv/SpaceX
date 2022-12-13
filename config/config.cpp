#include "config.hpp"
#include "port_info.hpp"
#include <cctype>
#include <cstring>
#include <string>

namespace {

	uint32_t digit_alpha_[] = {
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

	uint32_t digit_[] = {
		0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
		/*				?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
		0x03ff0000, /*	0000 0011 1111 1111  0000 0000 0000 0000 */
		/*				_^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
		0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
		/*				 ~}| {zyx wvut srqp  onml kjih gfed cba` */
		0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
		0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
		0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
		0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
		0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	};

	uint32_t vchar_[] = {
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

	uint32_t listen_[] = {
		0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
		/*				?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
		0x07ff4000, /*	0000 0111 1111 1111  0100 0000 0000 0000 */
		/*				_^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
		0x07fffffe, /*	0000 0111 1111 1111  1111 1111 1111 1110 */
		/*				 ~}| {zyx wvut srqp  onml kjih gfed cba` */
		0x07fffffe, /*	0000 0111 1111 1111  1111 1111 1111 1110 */
		0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
		0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
		0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
		0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	};

	uint32_t config_elem_syntax_[] = {
		0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
		/*				?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
		0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
		/*				_^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
		0x80000000, /*	1000 0000 0000 0000  0000 0000 0000 0000 */
		/*				 ~}| {zyx wvut srqp  onml kjih gfed cba` */
		0x077df3fe, /*	0000 0111 0111 1101  1111 0011 1111 1110 */
		0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
		0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
		0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
		0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	};

	uint32_t isspace_[] = {
		0x00003e00, /*	0000 0000 0000 0000  0011 1110 0000 0000 */
		/*				?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
		0x00000001, /*	0000 0000 0000 0000  0000 0000 0000 0001 */
		/*				_^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
		0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
		/*				 ~}| {zyx wvut srqp  onml kjih gfed cba` */
		0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
		0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
		0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
		0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
		0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	};

	inline status
	error_(const char* msg, const char* msg2) {
// throw(msg);
#ifdef DEBUG
		std::cout << msg << " : " << msg2 << std::endl;
#endif
		return spx_error;
	}

	inline uint8_t
	syntax_(const uint32_t table[8], uint8_t c) {
		return (table[(c >> 5)] & (1U << (c & 0x1f)));
	}

	template <typename T>
	inline void
	memset_(T& target) {
		memset(&target, 0, sizeof(T));
	}
}

status
spx_config_syntax_checker(std::string const&	   buf,
						  total_port_server_map_p& config_map) { // TODO : [add function] check default_server is exist and properly handle

	std::string::const_iterator		  it			= buf.begin();
	total_port_server_map_p::iterator total_conf_it = config_map.begin();
	server_map_p::iterator			  per_port_server_it;
	uri_location_map_p::iterator	  per_uri_location_it;
	std::string						  temp_string;
	uint8_t							  flag_default_part;
	uint8_t							  flag_location_part;
	uint8_t							  size_count;
	server_info_for_copy_stage_t	  temp_basic_server_info;
	uri_location_for_copy_stage_t	  temp_uri_location_info;

	enum {
		conf_zero,
		conf_start, // skip isspace

		conf_waiting_default_value,
		conf_endline,

		conf_server,
		conf_server_CB_open,
		conf_server_CB_close,
		conf_listen,
		conf_listen_default,
		conf_server_name,
		conf_error_page,
		conf_client_max_body_size,

		conf_waiting_location_value,

		conf_location_zero,
		conf_location_uri, // NOTE:: check is_set default value.
		conf_location_CB_open,
		conf_location_CB_close,
		conf_accepted_method,
		conf_root,
		conf_index,
		conf_autoindex,
		conf_redirect,
		conf_saved_path,
		conf_cgi_pass,
		conf_cgi_path_info,

		conf_almost_done,
		conf_done
	} state,
		prev_state,
		next_state;

	state = conf_zero;

	while (state != conf_done) {
		switch (state) {
		case conf_zero: {
			memset_(flag_default_part);
			memset_(flag_location_part);
			memset_(size_count);
			memset_(temp_basic_server_info);
			memset_(temp_uri_location_info);
			state	   = conf_start;
			next_state = conf_server;
			break;
		}

		case conf_start: {
			if (syntax_(isspace_, static_cast<uint8_t>(*it))) {
				++it;
				break;
			}
			prev_state = state;
			state	   = next_state;
			break;
		}

		case conf_waiting_default_value: {
			if (syntax_(config_elem_syntax_, static_cast<uint8_t>(*it))) {
				temp_string.push_back(*it);
				++size_count;
				++it;
				prev_state = state;
				break;
			}
			if (*it == ';') {
				if (prev_state == conf_start) {
					prev_state = state;
					state	   = conf_endline;
					next_state = conf_start;
					break;
				}
				return error_("conf_waiting_default_value", "syntax error");
			}
			if (syntax_(isspace_, static_cast<uint8_t>(*it))) {
				prev_state = state;
				state	   = conf_start;
				switch (size_count) {
				case 6: {
					if (temp_string.compare("listen") == KSame) {
						if (flag_default_part & flag_listen) {
							return error_("conf_waiting_value", "listen is already set");
						}
						next_state = conf_listen;
						break;
					}
					return error_("conf_waiting_value", "syntax error");
				}
				case 8: {
					if (temp_string.compare("location") == KSame) {
						if (!(flag_default_part & flag_listen)
							|| !(flag_default_part & flag_server_name)
							|| !(flag_default_part & flag_error_page)
							|| !(flag_default_part & flag_client_max_body_size)) {
							return error_("conf_waiting_value",
										  "need sed default value before location - syntax error");
						}
						next_state = conf_location_uri;
						break;
					}
					return error_("conf_waiting_value", "syntax error");
				}
				case 10: {
					if (temp_string.compare("error_page") == KSame) {
						if (flag_default_part & flag_error_page) {
							return error_("conf_waiting_value", "error_page is already set");
						}
						next_state = conf_error_page;
						break;
					}
					return error_("conf_waiting_value", "syntax error");
				}
				case 11: {
					if (temp_string.compare("server_name") == KSame) {
						if (flag_default_part & flag_server_name) {
							return error_("conf_waiting_value", "server_name is already set");
						}
						next_state = conf_server_name;
						break;
					}
					return error_("conf_waiting_value", "syntax error");
				}
				case 20: {
					if (temp_string.compare("client_max_body_size") == KSame) {
						if (flag_default_part & flag_client_max_body_size) {
							return error_("conf_waiting_value", "client_max_body_size is already set");
						}
						next_state = conf_client_max_body_size;
						break;
					}
					return error_("conf_waiting_value", "syntax error");
				}
				default:
					return error_("conf_waiting_value", "syntax error");
				}
				temp_string.clear();
				size_count = 0;
				break;
			}
			return error_("conf_waiting_value", "syntax error");
		}

		case conf_endline: {
			if (*it == ';') {
				++it;
				if (prev_state == conf_waiting_default_value) {
					next_state = conf_waiting_default_value;
				} else {
					next_state = conf_waiting_location_value;
				}
				prev_state = state;
				state	   = conf_start;
				break;
			}
		}

		case conf_server: {
			if (buf.compare(it - buf.begin(), 6, "server") == KSame) {
				prev_state = state;
				next_state = conf_server_CB_open;
				state	   = conf_start;
				break;
			}
			return error_("conf_server", "syntax error");
		}

		case conf_server_CB_open: {
			if (*it == '{') {
				++it;
				prev_state = state;
				next_state = conf_waiting_default_value;
				state	   = conf_start;
				break;
			}
			return error_("conf_server_CB_open", "syntax error");
		}

		case conf_server_CB_close: {

			break;
		}

		case conf_listen: {
			if (syntax_(listen_, static_cast<uint8_t>(*it))) {
				temp_string.push_back(*it);
				++size_count;
				++it;
				break;
			}
			if (syntax_(isspace_, static_cast<uint8_t>(*it)) || *it == ';') {
				std::string::const_iterator temp_it	   = temp_string.begin();
				uint8_t						temp_cycle = size_count;

				if (size_count && size_count <= 5) { // min 0, max 65535 // only port case
					while (temp_cycle--) {
						if (std::isdigit(*temp_it)) {
							temp_it++;
						} else {
							return error_("conf_listen", "syntax error");
						}
					}
					temp_basic_server_info.port = std::atoi(temp_string.c_str());
				} else if (10 < size_count && size_count <= 15) { // IP:PORT case / 127.0.0.1:80808 - 15 / localhost:80808 - 15
					if (temp_string.compare("localhost:") == KSame
						|| temp_string.compare("127.0.0.1:") == KSame) {
						temp_it += 10;
						temp_cycle -= 10;
						while (temp_cycle) {
							if (std::isdigit(*(temp_it + temp_cycle))) {
								--temp_cycle;
							} else {
								return error_("conf_listen invalid PORT", "syntax error");
							}
						}
						temp_string.erase(0, 10);
						temp_basic_server_info.port = std::atoi(temp_string.c_str());
					} else {
						return error_("conf_listen invalid IP:", "syntax error");
					}
				} else {
					return error_("conf_listen invalid IP:PORT", "syntax error");
				}
			} else {
				return error_("conf_listen invalid IP:PORT", "syntax error");
			}
			prev_state = state;
			if (*it == ';') {
				state	   = conf_endline;
				next_state = conf_waiting_default_value;
			} else {
				state	   = conf_start;
				next_state = conf_listen_default;
			}
			temp_basic_server_info.ip = "127.0.0.1";
			flag_default_part |= flag_listen;
			temp_string.clear();
			size_count = 0;
			break;
		}

		case conf_listen_default: {
			temp_basic_server_info.default_server_flag = default_server;
			prev_state								   = state;
			state									   = conf_endline;
			next_state								   = conf_waiting_default_value;
			if (buf.compare(it - buf.begin(), 14, "default_server") == KSame) {
				it += 14;
				break;
			}
			if (*it == ';') {
				break;
			}
			return error_("conf_listen_default", "syntax error");
		}

		case conf_server_name: {
			if (syntax_(digit_alpha_, static_cast<uint8_t>(*it))) {
				temp_string.push_back(*it);
				++it;
				break;
			}
			if (syntax_(isspace_, static_cast<uint8_t>(*it)) || *it == ';') {
				temp_basic_server_info.server_name = temp_string;
				prev_state						   = state;
				next_state						   = conf_waiting_default_value;
				state							   = conf_start;
				flag_default_part |= flag_server_name;
				temp_string.clear();
				break;
			}
			return error_("conf_server_name", "syntax error");
		}

		case conf_error_page: { // NOTE : is it need to check valid path?
			if (syntax_(vchar_, static_cast<uint8_t>(*it))) {
				temp_string.push_back(*it);
				++it;
				break;
			}
			if (syntax_(isspace_, static_cast<uint8_t>(*it)) || *it == ';') {
				temp_basic_server_info.error_page = temp_string;
				prev_state						  = state;
				next_state						  = conf_waiting_default_value;
				if (*it == ';') {
					state = conf_endline;
				} else {
					state = conf_start;
				}
				flag_default_part |= flag_error_page;
				temp_string.clear();
				break;
			}
			return error_("conf_error_page", "syntax error");
		}

		case conf_client_max_body_size: {
			if (syntax_(digit_, static_cast<uint8_t>(*it))) {
				temp_string.push_back(*it);
				++size_count;
				++it;
				break;
			}
			if ((*it == 'M' || *it == 'K') && (size_count && size_count <= 4)) {
				uint64_t check_body_size = std::atoi(temp_string.c_str());
				if (check_body_size > 0) {
					if (*it == 'M') {
						check_body_size *= (1024 * 1024);
					} else if (*it == 'K') {
						check_body_size *= 1024;
					}
					temp_basic_server_info.client_max_body_size = check_body_size;
					flag_default_part |= flag_client_max_body_size;
					prev_state = state;
					state	   = conf_start;
					next_state = conf_waiting_default_value;
				} else {
					return error_("conf_client_max_body_size - minus size", "syntax error");
				}
			} else {
				return error_("conf_client_max_body_size", "syntax error");
			}
			break;
		}

		case conf_location_zero: {
			memset_(temp_uri_location_info);
			break;
		}

		case conf_waiting_location_value: {
			if (syntax_(config_elem_syntax_, static_cast<uint8_t>(*it))) {
				temp_string.push_back(*it);
				++size_count;
				++it;
				prev_state = state;
				break;
			}
			if (*it == ';') {
				if (prev_state == conf_start) {
					prev_state = state;
					state	   = conf_endline;
					next_state = conf_start;
					break;
				}
				return error_("conf_waiting_default_value", "syntax error");
			}

			break;
		}

		case conf_location_uri: {
			if (syntax_(vchar_, static_cast<uint8_t>(*it))) {
				temp_string.push_back(*it);
				++it;
				break;
			}
			if (syntax_(isspace_, static_cast<uint8_t>(*it)) || *it == '{') {
				temp_uri_location_info.uri = temp_string;
				prev_state				   = state;
				state					   = conf_start;
				next_state				   = conf_location_CB_open;
				temp_string.clear();
				break;
			}
		}

		case conf_location_CB_open: {
			if (*it == '{') {
				++it;
				prev_state = state;
				state	   = conf_start;
				next_state = conf_waiting_location_value;
				break;
			}
			return error_("conf_location_CB_open", "syntax error");
		}

		case conf_location_CB_close: {

			break;
		}

		case conf_accepted_method: {

			break;
		}

		case conf_root: {

			break;
		}

		case conf_index: {

			break;
		}

		case conf_autoindex: {

			break;
		}

		case conf_redirect: {

			break;
		}
		case conf_saved_path: {

			break;
		}
		case conf_cgi_pass: {

			break;
		}
		case conf_cgi_path_info: {

			break;
		}
		case conf_almost_done: {

			break;
		}

		default:
			return error_("syntax error", "config file");
		}
	}

	return spx_ok;
}
