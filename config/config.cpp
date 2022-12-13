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
		0x80000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
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
						  total_port_server_map_p& config_map) {

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

	memset_(flag_default_part);
	memset_(flag_location_part);
	memset_(size_count);
	memset_(temp_basic_server_info);
	memset_(temp_uri_location_info);

	enum {
		conf_start,

		conf_server,
		conf_server_CB_open,
		conf_server_CB_close,
		conf_waiting_value,
		conf_listen,
		conf_server_name,
		conf_error_page,
		conf_client_max_body_size,

		conf_location,
		conf_location_uri,
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

	state	   = conf_start;
	next_state = conf_server;

	while (state != conf_done) {
		switch (state) {
		case conf_start: {
			if (syntax_(isspace_, static_cast<uint8_t>(*it))) {
				++it;
				break;
			}
			prev_state = state;
			state	   = next_state;
			break;
		}

		case conf_waiting_value: {
			if (syntax_(config_elem_syntax_, static_cast<uint8_t>(*it))) {
				temp_string.push_back(*it);
				++size_count;
				++it;
				break;
			}
			if (syntax_(isspace_, static_cast<uint8_t>(*it))) {
				prev_state = state;
				state	   = conf_start;
				switch (size_count) {
				case 6: {
					if (temp_string.compare("listen") == 0) {
						next_state = conf_listen;
						break;
					}
					return error_("conf_waiting_value", "syntax error");
				}
				case 10: {
					if (temp_string.compare("error_page") == 0) {
						next_state = conf_error_page;
						break;
					}
					return error_("conf_waiting_value", "syntax error");
				}
				case 11: {
					if (temp_string.compare("server_name") == 0) {
						next_state = conf_server_name;
						break;
					}
					return error_("conf_waiting_value", "syntax error");
				}
				case 20: {
					if (temp_string.compare("client_max_body_size") == 0) {
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

		case conf_server: {
			// size_count = buf.compare(it - buf.begin(), 6, "server");
			if (buf.compare("server") == 0) {
				prev_state = state;
				next_state = conf_server_CB_open;
				state	   = conf_start;
				break;
			}
			return error_("conf_server", "syntax error");
		}

		case conf_server_CB_open: {
			if (*it == '{') {
				prev_state = state;
				next_state = conf_waiting_value;
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
			if (syntax_(isspace_, static_cast<uint8_t>(*it))) {
				prev_state = state;
				state	   = conf_start;
				if (size_count <= 5) { // min 0, max 65535
					std::string::const_iterator temp_it	   = temp_string.begin();
					uint8_t						temp_cycle = size_count;

					while (temp_cycle--) {
						if (std::isdigit(*temp_it)) {
							temp_it++;
						} else {
							return error_("conf_listen", "syntax error");
						}
					}
				} else if (10 < size_count && size_count <= 15) {
					// 127.0.0.1:80808 - 15
					// localhost:80808 - 15
				}

				// next_state					  = conf_waiting_value;
				// temp_basic_server_info.listen = temp_string;

				temp_string.clear();
				size_count = 0;
				break;
			}
			return error_("conf_listen", "syntax error");
		}

		case conf_server_name: {

			break;
		}
		case conf_error_page: {

			break;
		}
		case conf_client_max_body_size: {

			break;
		}
		case conf_location: {

			break;
		}
		case conf_location_uri: {

			break;
		}
		case conf_location_CB_open: {

			break;
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
