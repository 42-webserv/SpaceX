#include "config.hpp"
#include "port_info.hpp"
#include "spacex_type.hpp"
#include <cctype>
#include <cstring>
#include <string>
#include <utility>

#ifdef CONFIG_DEBUG
#include <iostream>
#endif

extern const uint32_t isspace_[];
extern const uint32_t config_elem_syntax_[];
extern const uint32_t config_listen_[];
extern const uint32_t vchar_[];
extern const uint32_t digit_[];
extern const uint32_t alpha_upper_case_[];
extern const uint32_t digit_alpha_[];

namespace {

#ifdef CONFIG_STATE_DEBUG
#include <string>

#define CONFIG_STATE_MAP(XX)                               \
	XX(0, ZERO, Continue)                                  \
	XX(1, START, skip space)                               \
	XX(2, ENDLINE, check endline)                          \
	XX(3, WAITING_DEFAULT_VALUE, waiting default value)    \
	XX(4, SERVER, server position found)                   \
	XX(5, SERVER_OPEN, server curly bracket open)          \
	XX(6, SERVER_CLOSE, server curly bracket close)        \
	XX(7, LISTEN, listen found)                            \
	XX(8, LISTEN_DEFAULT, default_server)                  \
	XX(9, SERVER_NAME, server_name)                        \
	XX(10, ERROR_PAGE, error_page)                         \
	XX(11, CLIENT_MAX_BODY_SIZE, client_max_body_size)     \
	XX(12, WAITING_LOCATION_VALUE, waiting location value) \
	XX(13, LOCATION_ZERO, location flush)                  \
	XX(14, LOCATION_URI, location uri)                     \
	XX(15, LOCATION_OPEN, location curly bracket open)     \
	XX(16, LOCATION_CLOSE, location curly bracket close)   \
	XX(17, ACCEPTED_METHODS, accepted_methods)             \
	XX(18, ROOT, root)                                     \
	XX(19, INDEX, index)                                   \
	XX(20, AUTOINDEX, autoindex)                           \
	XX(21, REDIRECT, redirect)                             \
	XX(22, SAVED_PATH, saved_path)                         \
	XX(23, CGI_PASS, cgi_pass)                             \
	XX(24, CGI_PATH_INFO, cgi_path_info)                   \
	XX(25, ALMOST_DONE, almost done)                       \
	XX(26, DONE, done)

	typedef enum config_state {
#define XX(num, name, string) CONFIG_STATE_##name = num,
		CONFIG_STATE_MAP(XX)
#undef XX
	} config_state_e;

	std::string
	config_state_str_(config_state_e s) {
		switch (s) {
#define XX(num, name, string) \
	case CONFIG_STATE_##name: \
		return #string;
			CONFIG_STATE_MAP(XX)
#undef XX
		default:
			return "<unknown>";
		}
	};
#endif

	template <typename T>
	inline void
	memset_(T& target) {
		memset(&target, 0, sizeof(T));
	}

	inline status
	error_(const char* msg, const char* msg2) {
// throw(msg);
#ifdef CONFIG_DEBUG
		std::cout << "\033[1;31m" << msg << "\033[0m"
				  << " : "
				  << "\033[1;33m" << msg2 << "\033[0m" << std::endl;
#else
		(void)msg;
		(void)msg2;
#endif
		return spx_error;
	}

	template <typename T>
	inline void
	spx_log_(T msg) {
#ifdef CONFIG_DEBUG
		std::cout << "\033[1;32m" << msg << ";"
				  << "\033[0m" << std::endl;
#else
		(void)msg;
#endif
	}

} // namespace end;

status
spx_config_syntax_checker(std::string const&	   buf,
						  total_port_server_map_p& config_map) {

	std::string::const_iterator	  it = buf.begin();
	std::string					  temp_string;
	uint8_t						  flag_default_part;
	uint8_t						  flag_location_part;
	uint8_t						  size_count;
	uint32_t					  server_count	 = 0;
	uint32_t					  location_count = 0;
	server_info_for_copy_stage_t  temp_basic_server_info;
	uri_location_for_copy_stage_t temp_uri_location_info;
	uri_location_map_p			  saved_location_uri_map_1;
	server_map_p				  saved_server_name_map_2;
	total_port_server_map_p&	  saved_total_port_map_3 = config_map;

	enum {
		conf_zero = 0,
		conf_start, // skip isspace
		conf_endline,

		conf_waiting_default_value,

		conf_server,
		conf_server_CB_open, // 5
		conf_server_CB_close,
		conf_listen,
		conf_listen_default,
		conf_server_name,
		conf_error_page, // 10
		conf_client_max_body_size,

		conf_waiting_location_value,

		conf_location_zero,
		conf_location_uri,
		conf_location_CB_open, // 15
		conf_location_CB_close,
		conf_accepted_methods,
		// conf_set_module_case, // NOTE: specific action info
		conf_root,
		conf_index,
		conf_autoindex, // 20
		conf_redirect,
		conf_saved_path,
		conf_cgi_pass,
		conf_cgi_path_info,

		conf_almost_done, // 25
		conf_done
	} state,
		prev_state,
		next_state;
	prev_state = conf_zero;
	state	   = conf_zero;
	next_state = conf_start;

#ifdef CONFIG_STATE_DEBUG
	config_state_e debug_prev_state = static_cast<config_state_e>(prev_state);
#endif

	while (state != conf_done) {

#ifdef CONFIG_STATE_DEBUG
		if (debug_prev_state != state) {
			if (state != conf_start && state != conf_waiting_default_value && state != conf_waiting_location_value) {
				std::cout << "state : "
						  << "\033[1;33m" << config_state_str_(static_cast<config_state_e>(state)) << "\033[0m" << std::endl;
			}
			debug_prev_state = static_cast<config_state_e>(state);
		}
#endif
		switch (state) {

		case conf_zero: {
			if (server_count != 0) {
				temp_basic_server_info.uri_case = saved_location_uri_map_1; // STEP 1: saved_uri_ to server
				server_info_t					  yoma(temp_basic_server_info);
				total_port_server_map_p::iterator check_port_map;

				saved_server_name_map_2.insert(std::make_pair(yoma.server_name, yoma)); // STEP2: saved server to map
				check_port_map = saved_total_port_map_3.find(temp_basic_server_info.port);
				if (check_port_map == saved_total_port_map_3.end()) { // STEP3: saved map to total map (port not exist case)
					spx_log_("port not exist case : add new port to map");
					std::pair<std::map<const uint32_t, server_map_p>::iterator, bool> check_dup; // check server name dup case
					check_dup = saved_total_port_map_3.insert(std::make_pair(temp_basic_server_info.port, saved_server_name_map_2));
					if (check_dup.second == false) {
						return error_("conf_zero", "server_map_p insert fail");
					}
				} else { // STEP3: saved map to total map (port exist case)
					spx_log_("port exist case : add new server to port");
					if (temp_basic_server_info.default_server_flag == default_server) { // check default server dup case
						for (server_map_p::iterator it = check_port_map->second.begin(); it != check_port_map->second.end(); ++it) {
							if (it->second.default_server_flag == default_server) {
								return error_("conf_zero", "default server is already exist");
							}
						}
					}
					spx_log_("default_server dup check done");
					std::pair<std::map<const std::string, server_info_t>::iterator, bool> check_dup; // check server name dup case
					check_dup = check_port_map->second.insert(std::make_pair(temp_basic_server_info.server_name, temp_basic_server_info));
					if (check_dup.second == false) {
						return error_("conf_zero", "server name is already exist");
					}
					spx_log_("server name dup check done");
					// TODO: final stage : check default keyword in same port pool, is there only one default server?;
				}
#ifdef CONFIG_DEBUG
#endif
				server_info_for_copy_stage_t flush_server_info;
				temp_basic_server_info = flush_server_info;
				// temp_basic_server_info.uri_case.clear()
			} else {
				memset_(temp_basic_server_info);
			}
			memset_(flag_default_part);
			memset_(temp_uri_location_info);
			memset_(flag_location_part);
			memset_(size_count);
			memset_(location_count);
			saved_location_uri_map_1.clear();
			saved_server_name_map_2.clear();
			temp_string.clear();
			prev_state = state;
			state	   = next_state;
			next_state = conf_server;
			break;
		}

		case conf_start: {
			// if (*it == '\n') {
			// 	spx_log_("new line!!");
			// 	spx_log_((int)syntax_(isspace_, static_cast<uint8_t>(*it)));
			// 	spx_log_((int)syntax_(isspace_, '\n'));
			// 	spx_log_((int)syntax_(isspace_, '\t'));
			// 	spx_log_((int)static_cast<uint8_t>(*it));
			// }
			if (syntax_(isspace_, static_cast<uint8_t>(*it))) {
				++it;
				break;
			}
			if (*it == ';') {
				prev_state = next_state;
				state	   = conf_endline;
			} else {
				state = next_state;
			}
			break;
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
			return error_("conf_endline", "??");
		}

		case conf_waiting_default_value: {
			// spx_log_(std::distance(buf.begin(), it));
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
				return error_("conf_waiting_default_value", "end of line ; - syntax error");
			}
			if (size_count == 0 && *it == '}' && flag_default_part == 30) {
				prev_state = state;
				state	   = conf_server_CB_close;
				next_state = conf_start;
				break;
			} else if (size_count == 0 && *it == '}' && flag_location_part == 0) {
				return error_("conf_waiting_default_value", "empty server - syntax error");
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
					return error_("conf_waiting_value", "6 - syntax error");
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
					return error_("conf_waiting_value", "8 - syntax error");
				}
				case 10: {
					if (temp_string.compare("error_page") == KSame) {
						if (flag_default_part & flag_error_page) {
							return error_("conf_waiting_value", "error_page is already set");
						}
						next_state = conf_error_page;
						break;
					}
					return error_("conf_waiting_value", "10 - syntax error");
				}
				case 11: {
					if (temp_string.compare("server_name") == KSame) {
						if (flag_default_part & flag_server_name) {
							return error_("conf_waiting_value", "server_name is already set");
						}
						next_state = conf_server_name;
						break;
					}
					return error_("conf_waiting_value", "11 - syntax error");
				}
				case 20: {
					if (temp_string.compare("client_max_body_size") == KSame) {
						if (flag_default_part & flag_client_max_body_size) {
							return error_("conf_waiting_value", "client_max_body_size is already set");
						}
						next_state = conf_client_max_body_size;
						break;
					}
					return error_("conf_waiting_value", "20 - syntax error");
				}
				default:
					return error_("conf_waiting_value", "not allowed default value - syntax error");
				}
				temp_string.clear();
				size_count = 0;
				break;
			}
			return error_("conf_waiting_value", "isspace not found - syntax error");
		}

		case conf_server: {
			if (buf.compare(it - buf.begin(), 6, "server") == KSame) {
				prev_state = state;
				state	   = conf_start;
				next_state = conf_server_CB_open;
				it += 6;
				break;
			}
			if (it == buf.end()) {
				if (server_count == 0) {
					return error_("conf_server", "empty config file - syntax error");
				}
				state	   = conf_almost_done;
				next_state = conf_done;
				break;
			}
			return error_("conf_server", "syntax error");
		}

		case conf_server_CB_open: {
			if (*it == '{') {
				++it;
				prev_state = state;
				state	   = conf_start;
				next_state = conf_waiting_default_value;
				++server_count;
				break;
			}
			return error_("conf_server_CB_open", "syntax error");
		}

		case conf_server_CB_close: {
			if (*it == '}') {
				++it;
				prev_state = state;
				state	   = conf_zero;
				next_state = conf_start;
				if (location_count == 0) {
					return error_("conf_server_CB_close", "empty server block - syntax error");
				}
				break;
			}
			return error_("conf_server_CB_close", "syntax error");
		}

		case conf_listen: {
			if (syntax_(config_listen_, static_cast<uint8_t>(*it))) {
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
					if (temp_string.compare(0, 10, "localhost:") == KSame
						|| temp_string.compare(0, 10, "127.0.0.1:") == KSame) {
						temp_it += 10;
						temp_cycle -= 10;
						while (temp_cycle) {
							if (std::isdigit(*temp_it)) {
								temp_it++;
								temp_cycle--;
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
			state	   = conf_start;
			if (*it == ';') {
				++it;
				next_state = conf_waiting_default_value;
			} else {
				next_state = conf_listen_default;
			}
			temp_basic_server_info.ip = "127.0.0.1";
			flag_default_part |= flag_listen;
			temp_string.clear();
			size_count = 0;
			break;
		}

		case conf_listen_default: {
			if (buf.compare(it - buf.begin(), 14, "default_server") == KSame) {
				temp_basic_server_info.default_server_flag = default_server;
				it += 14;
			}
			if (syntax_(isspace_, static_cast<uint8_t>(*it)) || *it == ';') {
				prev_state = state;
				state	   = conf_start;
				next_state = conf_waiting_default_value;
				break;
			}
			return error_("conf_listen_default", "syntax error");
		}

		case conf_server_name: {
			if (syntax_(config_uri_host_, static_cast<uint8_t>(*it))) {
				temp_string.push_back(*it);
				++it;
				break;
			}
			if (syntax_(isspace_, static_cast<uint8_t>(*it)) || *it == ';') {
				temp_basic_server_info.server_name = temp_string;
				prev_state						   = state;
				state							   = conf_start;
				next_state						   = conf_waiting_default_value;
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
				if (*(it - 1) == ';') {
					temp_string.pop_back();
				}
				temp_basic_server_info.error_page = temp_string;
				prev_state						  = state;
				state							  = conf_start;
				next_state						  = conf_waiting_default_value;
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
					++it;
					temp_basic_server_info.client_max_body_size = check_body_size;
					prev_state									= state;
					state										= conf_start;
					next_state									= conf_waiting_default_value;
					flag_default_part |= flag_client_max_body_size;
					temp_string.clear();
					size_count = 0;
				} else {
					return error_("conf_client_max_body_size - minus size", "syntax error");
				}
			} else {
				return error_("conf_client_max_body_size", "syntax error");
			}
			break;
		}

		case conf_location_zero: {
			if (location_count != 0) {
				std::pair<std::map<const std::string, uri_location_t>::iterator, bool> check_dup;
				check_dup = saved_location_uri_map_1.insert(std::make_pair(temp_uri_location_info.uri, temp_uri_location_info));
				if (check_dup.second == false) {
					return error_("conf_location_zero", "duplicate location");
				}
#ifdef CONFIG_DEBUG
				spx_log_(check_dup.first->first.c_str());
				check_dup.first->second.print();
#endif
			}
			memset_(temp_uri_location_info);
			memset_(flag_location_part);
			memset_(size_count);
			temp_string.clear();
			prev_state = state;
			state	   = next_state;
			next_state = conf_waiting_default_value;
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
			if (size_count == 0 && *it == '}' && flag_location_part != 0) {
				prev_state = state;
				state	   = conf_location_CB_close;
				next_state = conf_start;
				break;
			} else if (size_count == 0 && *it == '}' && flag_location_part == 0) {
				return error_("conf_waiting_default_value", "empty location - syntax error");
			}
			if (syntax_(isspace_, static_cast<uint8_t>(*it)) || *it == ';') {
				prev_state = state;
				state	   = conf_start;
				switch (size_count) {
				case 4: {
					if (temp_string.compare("root") == KSame) {
						if (flag_location_part & flag_root) {
							return error_("conf_waiting_locaiton_value", "root is already set");
						}
						next_state = conf_root;
						break;
					}
					return error_("conf_waiting_default_value", "syntax error");
				}
				case 5: {
					if (temp_string.compare("index") == KSame) {
						if (flag_location_part & flag_index) {
							return error_("conf_waiting_locaiton_value", "index is already set");
						}
						next_state = conf_index;
						break;
					}
					return error_("conf_waiting_default_value", "syntax error");
				}
				case 8: {
					if (temp_string.compare("cgi_pass") == KSame) {
						if (flag_location_part & flag_cgi_pass) {
							return error_("conf_waiting_locaiton_value", "cgi_pass is already set");
						}
						next_state = conf_cgi_pass;
						break;
					}
					if (temp_string.compare("redirect") == KSame) {
						if (flag_location_part & flag_redirect) {
							return error_("conf_waiting_locaiton_value", "redirect is already set");
						}
						next_state = conf_redirect;
						break;
					}
					if (temp_string.compare("location") == KSame) {
						return error_("conf_waiting_default_value", "location depth is only 1 supported - syntax error");
					}
				}
				case 9: {
					if (temp_string.compare("autoindex") == KSame) {
						if (flag_location_part & flag_autoindex) {
							return error_("conf_waiting_locaiton_value", "autoindex is already set");
						}
						next_state = conf_autoindex;
						break;
					}
					return error_("conf_waiting_default_value", "syntax error");
				}
				case 10: {
					if (temp_string.compare("saved_path") == KSame) {
						if (flag_location_part & flag_saved_path) {
							return error_("conf_waiting_locaiton_value", "saved_path is already set");
						}
						next_state = conf_saved_path;
						break;
					}
					return error_("conf_waiting_default_value", "syntax error");
				}
				case 13: {
					if (temp_string.compare("cgi_path_info") == KSame) {
						if (flag_location_part & flag_cgi_path_info) {
							return error_("conf_waiting_locaiton_value", "cgi_path_info is already set");
						}
						next_state = conf_cgi_path_info;
						break;
					}
					return error_("conf_waiting_default_value", "syntax error");
				}
				case 16: {
					if (temp_string.compare("accepted_methods") == KSame) {
						if (flag_location_part & flag_accepted_methods) {
							return error_("conf_waiting_locaiton_value", "accepted_methods is already set");
						}
						next_state = conf_accepted_methods;
						break;
					}
					return error_("conf_waiting_default_value", "syntax error");
				}
				default: {
					return error_("conf_waiting_default_value", "syntax error");
				}
				}
				temp_string.clear();
				size_count = 0;
				break;
			}
			return error_("conf_waiting_default_value", "syntax error");
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
			return error_("conf_location_uri", "syntax error");
		}

		case conf_location_CB_open: {
			if (*it == '{') {
				++it;
				prev_state = state;
				state	   = conf_start;
				next_state = conf_waiting_location_value;
				++location_count;
				break;
			}
			return error_("conf_location_CB_open", "syntax error");
		}

		case conf_location_CB_close: {
			if (*it == '}') {
				++it;
				prev_state = state;
				state	   = conf_location_zero;
				next_state = conf_start;
				break;
			}
			return error_("conf_location_CB_close", "syntax error");
		}

		case conf_accepted_methods: {
			if (syntax_(alpha_upper_case_, static_cast<uint8_t>(*it))) {
				temp_string.push_back(*it);
				++it;
				++size_count;
				break;
			}
			if (syntax_(isspace_, static_cast<uint8_t>(*it)) || *it == ';') {
				switch (size_count) {
				case 3: {
					if (temp_string.compare("GET") == KSame) {
						if (temp_uri_location_info.accepted_methods_flag & KGet) {
							return error_("conf_accepted_methods", "GET is already set");
						}
						temp_uri_location_info.accepted_methods_flag |= KGet;
						break;
					}
					if (temp_string.compare("PUT") == KSame) {
						if (temp_uri_location_info.accepted_methods_flag & KPut) {
							return error_("conf_accepted_methods", "PUT is already set");
						}
						temp_uri_location_info.accepted_methods_flag |= KPut;
						break;
					}
					return error_("conf_accepted_methods", "syntax error");
				}
				case 4: {
					if (temp_string.compare("POST") == KSame) {
						if (temp_uri_location_info.accepted_methods_flag & KPost) {
							return error_("conf_accepted_methods", "POST is already set");
						}
						temp_uri_location_info.accepted_methods_flag |= KPost;
						break;
					}
					if (temp_string.compare("HEAD") == KSame) {
						if (temp_uri_location_info.accepted_methods_flag & KHead) {
							return error_("conf_accepted_methods", "HEAD is already set");
						}
						temp_uri_location_info.accepted_methods_flag |= KHead;
						break;
					}
					return error_("conf_accepted_methods", "syntax error");
				}
				case 6: {
					if (temp_string.compare("DELETE") == KSame) {
						if (temp_uri_location_info.accepted_methods_flag & KDelete) {
							return error_("conf_accepted_methods", "DELETE is already set");
						}
						temp_uri_location_info.accepted_methods_flag |= KDelete;
						break;
					}
					return error_("conf_accepted_methods", "syntax error");
				}
				default:
					return error_("conf_accepted_methods", "syntax error");
				}
				prev_state = state;
				state	   = conf_start;
				next_state = conf_accepted_methods;
				temp_string.clear();
				size_count = 0;
				break;
			}
			return error_("conf_accepted_methods", "syntax error");
		}

		case conf_root: {
			if (syntax_(vchar_, static_cast<uint8_t>(*it))) {
				temp_string.push_back(*it);
				++it;
				break;
			}
			if (syntax_(isspace_, static_cast<uint8_t>(*it)) || *it == ';') {
				if (*(it - 1) == ';') {
					temp_string.pop_back();
				}
				temp_uri_location_info.root = temp_string;
				prev_state					= state;
				state						= conf_start;
				next_state					= conf_waiting_location_value;
				flag_location_part |= flag_root;
				temp_string.clear();
				break;
			}
			return error_("conf_root", "syntax error");
		}

		case conf_index: {
			if (syntax_(vchar_, static_cast<uint8_t>(*it))) {
				temp_string.push_back(*it);
				++it;
				break;
			}
			if (syntax_(isspace_, static_cast<uint8_t>(*it)) || *it == ';') {
				if (*(it - 1) == ';') {
					temp_string.pop_back();
				}
				temp_uri_location_info.index = temp_string;
				prev_state					 = state;
				state						 = conf_start;
				next_state					 = conf_waiting_location_value;
				flag_location_part |= flag_index;
				temp_string.clear();
				break;
			}
			return error_("conf_index", "syntax error");
		}

		case conf_autoindex: {
			if (buf.compare(it - buf.begin(), 2, "on") == KSame) {
				temp_uri_location_info.autoindex_flag = Kautoindex_on;
				it += 2;
			} else if (buf.compare(it - buf.begin(), 3, "off") == KSame) {
				temp_uri_location_info.autoindex_flag = Kautoindex_off;
				it += 3;
			} else {
				return error_("conf_autoindex", "syntax error");
			}
			if (syntax_(isspace_, static_cast<uint8_t>(*it)) || *it == ';') {
				prev_state = state;
				state	   = conf_start;
				next_state = conf_waiting_location_value;
				flag_location_part |= flag_autoindex;
				break;
			}
			return error_("conf_autoindex", "syntax error");
		}

		case conf_redirect: {
			if (syntax_(vchar_, static_cast<uint8_t>(*it))) {
				temp_string.push_back(*it);
				++it;
				break;
			}
			if (syntax_(isspace_, static_cast<uint8_t>(*it)) || *it == ';') {
				if (*(it - 1) == ';') {
					temp_string.pop_back();
				}
				temp_uri_location_info.redirect = temp_string;
				prev_state						= state;
				state							= conf_start;
				next_state						= conf_waiting_location_value;
				flag_location_part |= flag_redirect;
				temp_string.clear();
				break;
			}
			return error_("conf_redirect", "syntax error");
		}

		case conf_saved_path: {
			if (syntax_(vchar_, static_cast<uint8_t>(*it))) {
				temp_string.push_back(*it);
				++it;
				break;
			}
			if (syntax_(isspace_, static_cast<uint8_t>(*it)) || *it == ';') {
				if (*(it - 1) == ';') {
					temp_string.pop_back();
				}
				temp_uri_location_info.saved_path = temp_string;
				prev_state						  = state;
				state							  = conf_start;
				next_state						  = conf_waiting_location_value;
				flag_location_part |= flag_saved_path;
				temp_string.clear();
				break;
			}
			return error_("conf_saved_path", "syntax error");
		}

		case conf_cgi_pass: {
			if (syntax_(vchar_, static_cast<uint8_t>(*it))) {
				temp_string.push_back(*it);
				++it;
				break;
			}
			if (syntax_(isspace_, static_cast<uint8_t>(*it)) || *it == ';') {
				if (*(it - 1) == ';') {
					temp_string.pop_back();
				}
				temp_uri_location_info.cgi_pass = temp_string;
				prev_state						= state;
				state							= conf_start;
				next_state						= conf_waiting_location_value;
				flag_location_part |= flag_cgi_pass;
				temp_string.clear();
				break;
			}
			return error_("conf_cgi_pass", "syntax error");
		}

		case conf_cgi_path_info: {
			if (syntax_(vchar_, static_cast<uint8_t>(*it))) {
				temp_string.push_back(*it);
				++it;
				break;
			}
			if (syntax_(isspace_, static_cast<uint8_t>(*it)) || *it == ';') {
				if (*(it - 1) == ';') {
					temp_string.pop_back();
				}
				temp_uri_location_info.cgi_path_info = temp_string;
				prev_state							 = state;
				state								 = conf_start;
				next_state							 = conf_waiting_location_value;
				flag_location_part |= flag_cgi_path_info;
				temp_string.clear();
				break;
			}
			return error_("conf_cgi_path_info", "syntax error");
		}

		case conf_almost_done: {
			// NOTE: what do we do here?
			state = next_state;
			break;
		}

		default: {
			return error_("syntax error", "config file");
		}
		}
	}
#ifdef CONFIG_DEBUG
	std::cout << "\033[1;31m"
			  << " -- config file parsed successfully -- "
			  << "\033[0m" << std::endl;
#endif
	return spx_ok;
}
