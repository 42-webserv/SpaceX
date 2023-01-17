#include "spx_parse_config.hpp"
#include "spx_port_info.hpp"

#include <dirent.h>
namespace {

#ifdef CONFIG_STATE_DEBUG

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
	XX(26, DONE, done)                                     \
	XX(27, ERROR_PAGE_NUMBER, error page number)           \
	XX(28, MAIN_ROOT, main root dir)

	typedef enum config_state {
#define XX(num, name, string) CONFIG_STATE_##name = num,
		CONFIG_STATE_MAP(XX)
#undef XX
			CONFIG_STATE_LAST
	} config_state_e;

	std::string
	config_state_str__(config_state_e s) {
		switch (s) {
#define XX(num, name, string) \
	case CONFIG_STATE_##name: \
		return #string;
			CONFIG_STATE_MAP(XX)
#undef XX
		default:
			return "<unknown>";
		}
	}
#endif

	inline status
	error__(const char* msg, const char* msg2, uint32_t const& line_number) {
		std::cout << COLOR_RED << "error line: " << line_number << " : " << msg << COLOR_RESET
				  << " : " << COLOR_YELLOW << msg2 << COLOR_RESET << std::endl;
		return spx_error;
	}

} // namespace end;

status
spx_config_syntax_checker(std::string const&	   buf,
						  total_port_server_map_p& config_map,
						  std::string const&	   cur_path) {

	std::string::const_iterator	  it = buf.begin();
	std::string					  temp_string;
	uint16_t					  flag_default_part;
	uint16_t					  flag_location_part;
	uint8_t						  size_count;
	uint32_t					  server_count		= 0;
	uint32_t					  location_count	= 0;
	uint32_t					  line_number_count = 1;
	server_info_for_copy_stage_t  temp_basic_server_info;
	uri_location_for_copy_stage_t temp_uri_location_info;
	std::vector<uint32_t>		  temp_error_page_number;
	uint8_t						  flag_error_page_default = 0;
	error_page_map_p			  saved_error_page_map_0;
	uri_location_map_p			  saved_location_uri_map_1;
	cgi_list_map_p				  saved_cgi_list_map_1;
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
		conf_root,
		conf_index,
		conf_autoindex, // 20
		conf_redirect,
		conf_saved_path,
		conf_cgi_pass,
		conf_cgi_path_info,
		conf_almost_done, // 25
		conf_done,
		conf_error_page_number,
		conf_main_root
	} state,
		prev_state,
		next_state;

	prev_state = conf_zero;
	state	   = conf_zero;
	next_state = conf_start;

	while (state != conf_done) {

#ifdef CONFIG_STATE_DEBUG
		if (state == conf_start) {
			std::cout << "state : "
					  << COLOR_WHITE << config_state_str__(static_cast<config_state_e>(state)) << COLOR_RESET << std::endl;
		} else {
			std::cout << "state : "
					  << COLOR_YELLOW << config_state_str__(static_cast<config_state_e>(state)) << COLOR_RESET << std::endl;
		}
#endif
		switch (state) {

		case conf_zero: {
			if (server_count != 0) {
				if (!(flag_default_part & Kflag_root_slash)) {
					return error__("conf_zero", "server root need to set '/' location ", line_number_count);
				}
				if (!(flag_default_part & Kflag_server_name)) {
					temp_basic_server_info.server_name = "localhost";
				}
				if (!(flag_default_part & Kflag_root)) {
					temp_basic_server_info.root = cur_path;
				}
				DIR* dir = opendir(temp_basic_server_info.root.c_str());
				if (dir == NULL) {
					return error__("conf_zero", "server root is not valid dir_name. check exist or control level", line_number_count);
				}
				closedir(dir);
				total_port_server_map_p::iterator check_default_server;
				check_default_server = saved_total_port_map_3.find(temp_basic_server_info.port);
				if (check_default_server == saved_total_port_map_3.end()) {
					temp_basic_server_info.default_server_flag = Kdefault_server;
				} else if (temp_basic_server_info.default_server_flag == Kdefault_server) {
					return error__("conf_zero", "default server already exist", line_number_count);
				}

				temp_basic_server_info.uri_case		   = saved_location_uri_map_1; // STEP 1: saved_uri_ to server
				temp_basic_server_info.cgi_case		   = saved_cgi_list_map_1; // STEP 1: saved_cgi_ to server
				temp_basic_server_info.error_page_case = saved_error_page_map_0; // STEP 1: saved_error_page_ to server
				server_info_t					  yoma(temp_basic_server_info);
				total_port_server_map_p::iterator check_port_map;
				saved_server_name_map_2.insert(std::make_pair(yoma.server_name, yoma)); // STEP2: saved server to map
				check_port_map = saved_total_port_map_3.find(temp_basic_server_info.port);
				if (check_port_map == saved_total_port_map_3.end()) { // STEP3: saved map to total map (port not exist case)
					std::pair<std::map<const uint32_t, server_map_p>::iterator, bool> check_dup; // check server name dup case
					check_dup = saved_total_port_map_3.insert(std::make_pair(temp_basic_server_info.port, saved_server_name_map_2));
					if (check_dup.second == false) {
						return error__("conf_zero", "server_map_p insert fail", line_number_count);
					}
				} else { // STEP3: saved map to total map (port exist case)
					if (temp_basic_server_info.default_server_flag == Kdefault_server) { // check default server dup case
						for (server_map_p::iterator it = check_port_map->second.begin(); it != check_port_map->second.end(); ++it) {
							if (it->second.default_server_flag == Kdefault_server) {
								return error__("conf_zero", "default server is already exist", line_number_count);
							}
						}
					}
					std::pair<std::map<const std::string, server_info_t>::iterator, bool> check_dup; // check server name dup case
					check_dup = check_port_map->second.insert(std::make_pair(temp_basic_server_info.server_name, temp_basic_server_info));
					if (check_dup.second == false) {
						return error__("conf_zero", "server name is already exist", line_number_count);
					}
				}
			}
			temp_uri_location_info.clear_();
			temp_basic_server_info.clear_();
			flag_default_part		= 0;
			flag_location_part		= 0;
			flag_error_page_default = 0;
			size_count				= 0;
			location_count			= 0;
			temp_error_page_number.clear();
			saved_error_page_map_0.clear();
			saved_location_uri_map_1.clear();
			saved_cgi_list_map_1.clear();
			saved_server_name_map_2.clear();
			temp_string.clear();
			prev_state = state;
			state	   = next_state;
			next_state = conf_server;
			break;
		}

		case conf_start: {
			while (syntax_(isspace_, static_cast<uint8_t>(*it))) {
				if (*it == '\n') {
					++line_number_count;
				}
				++it;
			}
			if (*it == '#'
				&& (next_state == conf_waiting_default_value || next_state == conf_waiting_location_value || next_state == conf_server)) {
				while (*it != '\n') {
					++it;
				}
				break;
			}
			if (*it == ';') {
				if (prev_state == conf_server_CB_open || prev_state == conf_location_CB_open) {
					return error__("conf_start", "semicolon can't exist after CB open - error", line_number_count);
				}
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
				if (prev_state == conf_waiting_default_value || prev_state == conf_listen_default) {
					next_state = conf_waiting_default_value;
				} else {
					next_state = conf_waiting_location_value;
				}
				prev_state = state;
				state	   = conf_start;
				break;
			}
			return error__("conf_endline", "??", line_number_count);
		}

		case conf_waiting_default_value: {
			while (syntax_(config_elem_syntax_, static_cast<uint8_t>(*it))) {
				temp_string.push_back(*it);
				++size_count;
				++it;
				prev_state = state;
			}
			if (*it == ';') {
				if (prev_state == conf_start) {
					prev_state = state;
					state	   = conf_endline;
					next_state = conf_start;
					break;
				}
				return error__("conf_waiting_default_value", "end of line ; - syntax error", line_number_count);
			}
			if (size_count == 0 && *it == '}') {
				prev_state = state;
				state	   = conf_server_CB_close;
				next_state = conf_start;
				break;
			}
			if (syntax_(isspace_, static_cast<uint8_t>(*it))) {
				switch (size_count) {
				case 4: {
					if (temp_string.compare("root") == KSame) {
						if (flag_default_part & Kflag_root) {
							return error__("conf_waiting_default_value", "root is already set", line_number_count);
						}
						next_state = conf_main_root;
						break;
					}
					return error__("conf_waiting_default_value", "4 - syntax error", line_number_count);
				}
				case 6: {
					if (temp_string.compare("listen") == KSame) {
						if (flag_default_part & Kflag_listen) {
							return error__("conf_waiting_default_value", "listen is already set", line_number_count);
						}
						next_state = conf_listen;
						break;
					}
					return error__("conf_waiting_default_value", "6 - syntax error", line_number_count);
				}
				case 8: {
					if (temp_string.compare("location") == KSame) {
						if (!(flag_default_part & Kflag_listen)) {
							return error__("conf_waiting_default_value",
										   "need to set default value before location - syntax error", line_number_count);
						}
						next_state = conf_location_uri;
						break;
					}
					return error__("conf_waiting_default_value", "8 - syntax error", line_number_count);
				}
				case 10: {
					if (temp_string.compare("error_page") == KSame) {
						next_state = conf_error_page_number;
						break;
					}
					return error__("conf_waiting_default_value", "10 - syntax error", line_number_count);
				}
				case 11: {
					if (temp_string.compare("server_name") == KSame) {
						if (flag_default_part & Kflag_server_name) {
							return error__("conf_waiting_default_value", "server_name is already set", line_number_count);
						}
						next_state = conf_server_name;
						break;
					}
					return error__("conf_waiting_default_value", "11 - syntax error", line_number_count);
				}
				default:
					return error__("conf_waiting_default_value", "not allowed default value - syntax error", line_number_count);
				}
				prev_state = state;
				state	   = conf_start;
				temp_string.clear();
				size_count = 0;
				break;
			}
			return error__("conf_waiting_value", "isspace not found or plz close curly bracket - syntax error", line_number_count);
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
					return error__("conf_server", "empty config file - syntax error", line_number_count);
				}
				state	   = conf_almost_done;
				next_state = conf_done;
				break;
			}
			return error__("conf_server", "syntax error", line_number_count);
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
			return error__("conf_server_CB_open", "syntax error", line_number_count);
		}

		case conf_server_CB_close: {
			if (*it == '}') {
				++it;
				prev_state = state;
				state	   = conf_zero;
				next_state = conf_start;
				if (location_count == 0) {
					return error__("conf_server_CB_close", "empty server block - syntax error", line_number_count);
				}
				if (!(flag_default_part & Kflag_listen)) {
					return error__("conf_server_CB_close", "missing must value set - syntax error", line_number_count);
				}
				break;
			}
			return error__("conf_server_CB_close", "syntax error", line_number_count);
		}

		case conf_main_root: {
			while (syntax_(config_vchar_except_delimiter_, static_cast<uint8_t>(*it))) {
				temp_string.push_back(*it);
				++it;
			}
			if (syntax_(isspace_, static_cast<uint8_t>(*it)) || *it == ';') {
				temp_basic_server_info.root = temp_string;
				temp_string.clear();
				prev_state = state;
				state	   = conf_start;
				next_state = conf_waiting_default_value;
				flag_default_part |= Kflag_root;
				break;
			}
			return error__("conf_main_root", "syntax error", line_number_count);
		}

		case conf_listen: {
			while (syntax_(config_listen_, static_cast<uint8_t>(*it))) {
				temp_string.push_back(*it);
				++size_count;
				++it;
			}
			if (syntax_(isspace_, static_cast<uint8_t>(*it)) || *it == ';') {
				std::string::const_iterator temp_it	   = temp_string.begin();
				uint8_t						temp_cycle = size_count;

				if (size_count && size_count <= 5) { // min 0, max 65535 // only port case
					while (temp_cycle--) {
						if (std::isdigit(*temp_it)) {
							temp_it++;
						} else {
							return error__("conf_listen", "syntax error", line_number_count);
						}
					}
				} else if (10 <= size_count && size_count <= 15) { // IP:PORT case / 127.0.0.1:80808 - 15 / localhost:80808 - 15
					if (temp_string.compare(0, 10, "localhost:") == KSame
						|| temp_string.compare(0, 10, "127.0.0.1:") == KSame) {
						temp_it += 10;
						temp_cycle -= 10;
						while (temp_cycle) {
							if (std::isdigit(*temp_it)) {
								temp_it++;
								temp_cycle--;
							} else {
								return error__("conf_listen invalid PORT", "syntax error", line_number_count);
							}
						}
						temp_string.erase(0, 10);
					} else { // NOTE:: may need to parse the IP address
						return error__("conf_listen we didn't supported specific IP:", "syntax error", line_number_count);
					}
				} else {
					return error__("conf_listen invalid IP:PORT", "syntax error", line_number_count);
				}
			} else {
				return error__("conf_listen invalid IP:PORT", "syntax error", line_number_count);
			}
			uint32_t temp_port = std::atoi(temp_string.c_str());
			if (temp_port > 65535 | temp_port <= 0) {
				return error__("conf_listen invalid PORT", "syntax error", line_number_count);
			}
			temp_basic_server_info.port = temp_port;
			prev_state					= state;
			state						= conf_start;
			if (*it == ';') {
				++it;
				next_state = conf_waiting_default_value;
			} else {
				next_state = conf_listen_default;
			}
			temp_basic_server_info.ip = "127.0.0.1";
			flag_default_part |= Kflag_listen;
			temp_string.clear();
			size_count = 0;
			break;
		}

		case conf_listen_default: {
			if (buf.compare(it - buf.begin(), 14, "default_server") == KSame) {
				temp_basic_server_info.default_server_flag = Kdefault_server;
				it += 14;
			}
			if (syntax_(isspace_, static_cast<uint8_t>(*it)) || *it == ';') {
				prev_state = state;
				state	   = conf_start;
				next_state = conf_waiting_default_value;
				break;
			}
			return error__("conf_listen_default", "syntax error", line_number_count);
		}

		case conf_server_name: {
			while (syntax_(config_uri_host_, static_cast<uint8_t>(*it))) {
				temp_string.push_back(*it);
				++it;
			}
			if (syntax_(isspace_, static_cast<uint8_t>(*it)) || *it == ';') {
				temp_basic_server_info.server_name = temp_string;
				prev_state						   = state;
				state							   = conf_start;
				next_state						   = conf_waiting_default_value;
				flag_default_part |= Kflag_server_name;
				temp_string.clear();
				break;
			}
			return error__("conf_server_name", "syntax error", line_number_count);
		}

		case conf_error_page_number: {
			if (syntax_(digit_, static_cast<uint8_t>(*it))) {
				while (syntax_(digit_, static_cast<uint8_t>(*it))) {
					temp_string.push_back(*it);
					++it;
					++size_count;
				}
			} else if (syntax_(config_vchar_except_delimiter_, static_cast<uint8_t>(*it))) {
				if (prev_state == conf_error_page_number) {
					prev_state = state;
					state	   = conf_error_page;
					break;
				} else if (prev_state == conf_waiting_default_value) {
					flag_error_page_default |= 1;
					prev_state = state;
					state	   = conf_error_page;
					break;
				} else {
					return error__("conf_error_page_number : how can you reach here", "syntax error", line_number_count);
				}
			}
			if (syntax_(isspace_, static_cast<uint8_t>(*it))) {
				if (size_count == 3) {
					temp_error_page_number.push_back(std::atoi(temp_string.c_str()));
					size_count = 0;
					temp_string.clear();
					prev_state = state;
					state	   = conf_start;
					next_state = conf_error_page_number;
					break;
				} else {
					return error__("conf_error_page_number : ONLY 3 DIGIT", "syntax error", line_number_count);
				}
			}
			return error__("conf_error_page_number", "syntax error", line_number_count);
		}

		case conf_error_page: {
			// typedef std::map<const uint32_t, const std::string> error_page_map_p;
			while (syntax_(config_vchar_except_delimiter_, static_cast<uint8_t>(*it))) {
				temp_string.push_back(*it);
				++it;
			}
			if (syntax_(isspace_, static_cast<uint8_t>(*it)) || *it == ';') {
				if (flag_error_page_default) {
					if (!(flag_default_part & Kflag_error_page)) {
						flag_default_part |= Kflag_error_page;
						temp_basic_server_info.default_error_page = temp_string;
						flag_error_page_default					  = 0;
					} else {
						return error__("conf_error_page : default error page already exist", "syntax error", line_number_count);
					}
				}
				{
					for (std::vector<uint32_t>::iterator it = temp_error_page_number.begin(); it != temp_error_page_number.end(); ++it) {
						std::pair<error_page_map_p::iterator, bool> dup_check;
						dup_check = saved_error_page_map_0.insert(std::make_pair(*it, temp_string));
						if (dup_check.second == false) {
							return error__("conf_error_page : duplicate error page number", "syntax error", line_number_count);
						}
					}
					temp_error_page_number.clear();
				}
				prev_state = state;
				state	   = conf_start;
				next_state = conf_waiting_default_value;
				temp_string.clear();
				break;
			}
			return error__("conf_error_page", "syntax error", line_number_count);
		}

		case conf_location_zero: {
			if (location_count != 0) {
				if (!(flag_location_part & Kflag_accepted_methods)) {
					return error__("conf_location_zero", "accepted_methods not defined", line_number_count);
				}
				if (temp_uri_location_info.uri.at(0) == '.') { // cgi_case
					if (flag_location_part & (Kflag_index | Kflag_autoindex | Kflag_redirect)) {
						return error__("conf_location_zero", "index, autoindex, redirect can't be used in cgi location", line_number_count);
					}
					if (temp_uri_location_info.module_state == Kmodule_none) {
						temp_uri_location_info.module_state = Kmodule_cgi;
					} else {
						return error__("conf_location_zero", "module_state already defined", line_number_count);
					}
					if (flag_location_part & Kflag_saved_path) {
						if (temp_uri_location_info.saved_path.at(0) != '.') {
							return error__("conf_location_zero", "cgi saved_path must start with '.' ", line_number_count);
						}
					} else {
						temp_uri_location_info.saved_path = temp_basic_server_info.root + "/cgi_saved";
					}
					if (!(flag_location_part & Kflag_cgi_path_info)) {
						return error__("conf_location_zero", "cgi_path_info not defined", line_number_count);
					}
					if (!(flag_location_part & Kflag_cgi_pass)) {
						return error__("conf_location_zero", "cgi_pass not defined", line_number_count);
					}
					std::pair<std::map<const std::string, uri_location_t>::iterator, bool> check_dup;
					check_dup = saved_cgi_list_map_1.insert(std::make_pair(temp_uri_location_info.uri, temp_uri_location_info));
					if (check_dup.second == false) {
						return error__("conf_location_zero", "duplicate cgi location", line_number_count);
					}
				} else if (temp_uri_location_info.uri.at(0) == '/') { // location_case
					if (flag_location_part & Kflag_cgi_pass && (flag_location_part & (Kflag_cgi_path_info | Kflag_redirect | Kflag_index | Kflag_autoindex))) {
						return error__("conf_location_zero", "cgi_pass can't be exist with index, autoindex, redirect, cgi_path_info", line_number_count);
					}
					if (!(flag_location_part & Kflag_root)) {
						if (temp_basic_server_info.root.empty()) {
							temp_uri_location_info.root = cur_path + temp_uri_location_info.uri;
						} else {
							temp_uri_location_info.root = temp_basic_server_info.root + temp_uri_location_info.uri;
						}
					}
					if (flag_location_part & Kflag_saved_path) {
						if (!(flag_location_part & Kflag_cgi_pass) && temp_uri_location_info.saved_path.at(0) == '.') {
							return error__("conf_location_zero", "location saved_path must not start with '.' ", line_number_count);
						}
					} else {
						temp_uri_location_info.saved_path = temp_uri_location_info.uri + "_saved";
					}
					if (!(flag_location_part & (Kflag_redirect | Kflag_cgi_pass))) {
						DIR* dir = opendir(temp_uri_location_info.root.c_str());
						if (dir == NULL) {
							return error__("conf_location_zero", "location root is not valid dir_name. check exist or control level", line_number_count);
						}
						closedir(dir);
					}
					if (temp_uri_location_info.module_state == Kmodule_none) {
						temp_uri_location_info.module_state = Kmodule_serve;
					}
					std::pair<std::map<const std::string, uri_location_t>::iterator, bool> check_dup;
					check_dup = saved_location_uri_map_1.insert(std::make_pair(temp_uri_location_info.uri, temp_uri_location_info));
					if (check_dup.second == false) {
						return error__("conf_location_zero", "duplicate location", line_number_count);
					}
				} else {
					return error__("conf_location_zero", "location uri syntax error", line_number_count);
				}
			}
			temp_uri_location_info.clear_();
			flag_location_part = 0;
			size_count		   = 0;
			temp_string.clear();
			prev_state = state;
			state	   = next_state;
			next_state = conf_waiting_default_value;
			break;
		}

		case conf_waiting_location_value: {
			while (syntax_(config_elem_syntax_, static_cast<uint8_t>(*it))) {
				temp_string.push_back(*it);
				++size_count;
				++it;
				prev_state = state;
			}
			if (*it == ';') {
				if (prev_state == conf_start) {
					prev_state = state;
					state	   = conf_endline;
					next_state = conf_start;
					break;
				}
				return error__("conf_waiting_location_value", "syntax error", line_number_count);
			}
			if (size_count == 0 && *it == '}' && flag_location_part & Kflag_accepted_methods) {
				prev_state = state;
				state	   = conf_location_CB_close;
				next_state = conf_start;
				break;
			} else if (size_count == 0 && *it == '}' && flag_location_part == 0) {
				return error__("conf_waiting_default_value", "empty location - syntax error", line_number_count);
			}
			if (syntax_(isspace_, static_cast<uint8_t>(*it)) || *it == ';') {
				prev_state = state;
				state	   = conf_start;
				switch (size_count) {
				case 4: {
					if (temp_string.compare("root") == KSame) {
						if (flag_location_part & Kflag_root) {
							return error__("conf_waiting_locaiton_value", "root is already set", line_number_count);
						}
						next_state = conf_root;
						break;
					}
					return error__("conf_waiting_location_value", "4 - syntax error", line_number_count);
				}
				case 5: {
					if (temp_string.compare("index") == KSame) {
						if (flag_location_part & Kflag_index) {
							return error__("conf_waiting_locaiton_value", "index is already set", line_number_count);
						}
						next_state = conf_index;
						break;
					}
					return error__("conf_waiting_location_value", "5 - syntax error", line_number_count);
				}
				case 8: {
					if (temp_string.compare("cgi_pass") == KSame) {
						if (flag_location_part & Kflag_cgi_pass) {
							return error__("conf_waiting_locaiton_value", "cgi_pass is already set", line_number_count);
						}
						next_state = conf_cgi_pass;
						break;
					}
					if (temp_string.compare("redirect") == KSame) {
						if (flag_location_part & Kflag_redirect) {
							return error__("conf_waiting_locaiton_value", "redirect is already set", line_number_count);
						}
						next_state = conf_redirect;
						break;
					}
					if (temp_string.compare("location") == KSame) {
						return error__("conf_waiting_default_value", "location depth is only 1 supported - syntax error", line_number_count);
					}
				}
				case 9: {
					if (temp_string.compare("autoindex") == KSame) {
						if (flag_location_part & Kflag_autoindex) {
							return error__("conf_waiting_locaiton_value", "autoindex is already set", line_number_count);
						}
						next_state = conf_autoindex;
						break;
					}
					return error__("conf_waiting_location_value", "9 - syntax error", line_number_count);
				}
				case 10: {
					if (temp_string.compare("saved_path") == KSame) {
						if (flag_location_part & Kflag_saved_path) {
							return error__("conf_waiting_locaiton_value", "saved_path is already set", line_number_count);
						}
						next_state = conf_saved_path;
						break;
					}
					return error__("conf_waiting_location_value", "10 - syntax error", line_number_count);
				}
				case 13: {
					if (temp_string.compare("cgi_path_info") == KSame) {
						if (flag_location_part & Kflag_cgi_path_info) {
							return error__("conf_waiting_locaiton_value", "cgi_path_info is already set", line_number_count);
						}
						next_state = conf_cgi_path_info;
						break;
					}
					if (temp_string.compare("max_body_size") == KSame) {
						if (flag_location_part & Kflag_client_max_body_size) {
							return error__("conf_waiting_location_value", "max_body_size is already set", line_number_count);
						}
						next_state = conf_client_max_body_size;
						break;
					}
					return error__("conf_waiting_location_value", "13 - syntax error", line_number_count);
				}
				case 16: {
					if (temp_string.compare("accepted_methods") == KSame) {
						if (flag_location_part & Kflag_accepted_methods) {
							return error__("conf_waiting_locaiton_value", "accepted_methods is already set", line_number_count);
						}
						next_state = conf_accepted_methods;
						break;
					}
					return error__("conf_waiting_location_value", "16 - syntax error", line_number_count);
				}
				default: {
					return error__("conf_waiting_location_value", "syntax error", line_number_count);
				}
				}
				temp_string.clear();
				size_count = 0;
				break;
			}
			return error__("conf_waiting_location_value", "end-line syntax error", line_number_count);
		}

		case conf_location_uri: {
			int cgi_dot_checker_ = 0;
			if (*it == '/' || *it == '.') {
				temp_string.push_back(*it);
				if (*it == '.')
					++cgi_dot_checker_;
				++it;
			} else {
				return error__("conf_location_uri", "start char error", line_number_count);
			}
			while (syntax_(vchar_, static_cast<uint8_t>(*it)) && *it != '{') {
				if (*it == '/') {
					return error__("conf_location_uri", "only allowed one depth slash", line_number_count);
				} else if (*it == '.') {
					if (cgi_dot_checker_ == 0) {
						return error__("conf_location_uri", "location uri doesn't support '.' character to prevent ambiguous", line_number_count);
					}
					++cgi_dot_checker_;
					if (cgi_dot_checker_ > 1) {
						return error__("conf_location_uri", "only allowed one depth dot", line_number_count);
					}
				}
				temp_string.push_back(*it);
				++it;
			}
			if (syntax_(isspace_, static_cast<uint8_t>(*it)) || *it == '{') {
				temp_uri_location_info.uri = temp_string;
				prev_state				   = state;
				state					   = conf_start;
				next_state				   = conf_location_CB_open;
				if (!temp_uri_location_info.uri.compare("/")) {
					flag_default_part |= Kflag_root_slash;
				}
				temp_string.clear();
				break;
			}
			return error__("conf_location_uri", "syntax error", line_number_count);
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
			return error__("conf_location_CB_open", "syntax error", line_number_count);
		}

		case conf_location_CB_close: {
			if (*it == '}') {
				++it;
				prev_state = state;
				state	   = conf_location_zero;
				next_state = conf_start;
				break;
			}
			return error__("conf_location_CB_close", "syntax error", line_number_count);
		}

		case conf_client_max_body_size: {
			while (syntax_(digit_, static_cast<uint8_t>(*it))) {
				temp_string.push_back(*it);
				++size_count;
				++it;
			}
			if (0 < size_count && size_count <= 10) {
				uint64_t check_body_size = std::atoi(temp_string.c_str());
				if (check_body_size > 0) {
					if (*it == 'M') {
						check_body_size *= (1024 * 1024);
						++it;
					} else if (*it == 'K') {
						check_body_size *= 1024;
						++it;
					}
					if (check_body_size > 2147483647) {
						return error__("conf_client_max_body_size : 255M or 262131K limited", "syntax error", line_number_count);
					} else if (check_body_size <= 0) {
						return error__("conf_client_max_body_size : 0 or - size not supported", "syntax error", line_number_count);
					}
					if (syntax_(isspace_, static_cast<uint8_t>(*it)) || *it == ';') {
						temp_uri_location_info.client_max_body_size = check_body_size;
						prev_state									= state;
						state										= conf_start;
						next_state									= conf_waiting_location_value;
						flag_location_part |= Kflag_client_max_body_size;
						temp_string.clear();
						size_count = 0;
						break;
					}
					return error__("conf_client_max_body_size : undefined character used", "syntax error", line_number_count);
				}
				return error__("conf_client_max_body_size - minus size", "syntax error", line_number_count);
			}
			return error__("conf_client_max_body_size", "syntax error", line_number_count);
		}

		case conf_accepted_methods: {
			while (syntax_(alpha_upper_case_, static_cast<uint8_t>(*it))) {
				temp_string.push_back(*it);
				++it;
				++size_count;
			}
			if (syntax_(isspace_, static_cast<uint8_t>(*it)) || *it == ';') {
				flag_location_part |= Kflag_accepted_methods;
				switch (size_count) {
				case 3: {
					if (temp_string.compare("GET") == KSame) {
						if (temp_uri_location_info.accepted_methods_flag & KGet) {
							return error__("conf_accepted_methods", "GET is already set", line_number_count);
						}
						temp_uri_location_info.accepted_methods_flag |= KGet;
						break;
					}
					if (temp_string.compare("PUT") == KSame) {
						if (temp_uri_location_info.accepted_methods_flag & KPut) {
							return error__("conf_accepted_methods", "PUT is already set", line_number_count);
						}
						temp_uri_location_info.accepted_methods_flag |= KPut;
						break;
					}
					if (temp_string.compare("ALL") == KSame) {
						temp_uri_location_info.accepted_methods_flag |= (KGet | KPut | KPost | KHead | KDelete);
						break;
					}
					return error__("conf_accepted_methods", "syntax error", line_number_count);
				}
				case 4: {
					if (temp_string.compare("POST") == KSame) {
						if (temp_uri_location_info.accepted_methods_flag & KPost) {
							return error__("conf_accepted_methods", "POST is already set", line_number_count);
						}
						temp_uri_location_info.accepted_methods_flag |= KPost;
						break;
					}
					if (temp_string.compare("HEAD") == KSame) {
						if (temp_uri_location_info.accepted_methods_flag & KHead) {
							return error__("conf_accepted_methods", "HEAD is already set", line_number_count);
						}
						temp_uri_location_info.accepted_methods_flag |= KHead;
						break;
					}
					return error__("conf_accepted_methods", "syntax error", line_number_count);
				}
				case 6: {
					if (temp_string.compare("DELETE") == KSame) {
						if (temp_uri_location_info.accepted_methods_flag & KDelete) {
							return error__("conf_accepted_methods", "DELETE is already set", line_number_count);
						}
						temp_uri_location_info.accepted_methods_flag |= KDelete;
						break;
					}
					return error__("conf_accepted_methods", "syntax error", line_number_count);
				}
				default:
					return error__("conf_accepted_methods", "syntax error", line_number_count);
				}
				prev_state = state;
				state	   = conf_start;
				next_state = conf_accepted_methods;
				temp_string.clear();
				size_count = 0;
				break;
			}
			return error__("conf_accepted_methods", "syntax error", line_number_count);
		}

		case conf_root: {
			while (syntax_(config_vchar_except_delimiter_, static_cast<uint8_t>(*it))) {
				temp_string.push_back(*it);
				++it;
			}
			if (syntax_(isspace_, static_cast<uint8_t>(*it)) || *it == ';') {
				temp_uri_location_info.root = temp_string;
				prev_state					= state;
				state						= conf_start;
				next_state					= conf_waiting_location_value;
				flag_location_part |= Kflag_root;
				temp_string.clear();
				break;
			}
			return error__("conf_root", "syntax error", line_number_count);
		}

		case conf_index: {
			while (syntax_(config_vchar_except_delimiter_, static_cast<uint8_t>(*it))) {
				temp_string.push_back(*it);
				++it;
			}
			if (syntax_(isspace_, static_cast<uint8_t>(*it)) || *it == ';') {
				temp_uri_location_info.index = temp_string;
				prev_state					 = state;
				state						 = conf_start;
				next_state					 = conf_waiting_location_value;
				flag_location_part |= Kflag_index;
				temp_string.clear();
				break;
			}
			return error__("conf_index", "syntax error", line_number_count);
		}

		case conf_autoindex: {
			if (buf.compare(it - buf.begin(), 2, "on") == KSame) {
				temp_uri_location_info.autoindex_flag = Kautoindex_on;
				it += 2;
			} else if (buf.compare(it - buf.begin(), 3, "off") == KSame) {
				temp_uri_location_info.autoindex_flag = Kautoindex_off;
				it += 3;
			} else {
				return error__("conf_autoindex", "not on and off - syntax error", line_number_count);
			}
			if (syntax_(isspace_, static_cast<uint8_t>(*it)) || *it == ';') {
				prev_state = state;
				state	   = conf_start;
				next_state = conf_waiting_location_value;
				flag_location_part |= Kflag_autoindex;
				break;
			}
			return error__("conf_autoindex", "syntax error", line_number_count);
		}

		case conf_redirect: {
			while (syntax_(config_vchar_except_delimiter_, static_cast<uint8_t>(*it))) {
				temp_string.push_back(*it);
				++it;
			}
			if (syntax_(isspace_, static_cast<uint8_t>(*it)) || *it == ';') {
				temp_uri_location_info.redirect = temp_string;
				prev_state						= state;
				state							= conf_start;
				next_state						= conf_waiting_location_value;
				flag_location_part |= Kflag_redirect;
				temp_uri_location_info.module_state = Kmodule_redirect;
				{
					uri_location_map_p::iterator dup_check = saved_location_uri_map_1.find(server_info_t::path_resolve_('/' + temp_string));
					if (dup_check != saved_location_uri_map_1.end()
						&& server_info_t::path_resolve_('/' + dup_check->second.redirect) == temp_uri_location_info.uri) {
						return error__("conf_redirect", "recursive redirect found", line_number_count);
					}
				}
				temp_string.clear();
				break;
			}
			return error__("conf_redirect", "syntax error", line_number_count);
		}

		case conf_saved_path: {
			while (syntax_(config_vchar_except_delimiter_, static_cast<uint8_t>(*it))) {
				temp_string.push_back(*it);
				++it;
			}
			if (syntax_(isspace_, static_cast<uint8_t>(*it)) || *it == ';') {
				temp_uri_location_info.saved_path = temp_string;
				prev_state						  = state;
				state							  = conf_start;
				next_state						  = conf_waiting_location_value;
				flag_location_part |= Kflag_saved_path;
				temp_string.clear();
				break;
			}
			return error__("conf_saved_path", "syntax error", line_number_count);
		}

		case conf_cgi_pass: {
			while (syntax_(config_vchar_except_delimiter_, static_cast<uint8_t>(*it))) {
				temp_string.push_back(*it);
				++it;
			}
			if (syntax_(isspace_, static_cast<uint8_t>(*it)) || *it == ';') {
				temp_uri_location_info.cgi_pass = temp_string;
				std::fstream exist_test;
				exist_test.open(temp_uri_location_info.cgi_pass.c_str(), std::ios::in);
				if (!exist_test.is_open()) {
					return error__("conf_cgi_pass", "invalid cgi_pass", line_number_count);
				}
				exist_test.close();
				prev_state = state;
				state	   = conf_start;
				next_state = conf_waiting_location_value;
				flag_location_part |= Kflag_cgi_pass;
				temp_string.clear();
				break;
			}
			return error__("conf_cgi_pass", "syntax error", line_number_count);
		}

		case conf_cgi_path_info: {
			while (syntax_(config_vchar_except_delimiter_, static_cast<uint8_t>(*it))) {
				temp_string.push_back(*it);
				++it;
			}
			if (syntax_(isspace_, static_cast<uint8_t>(*it)) || *it == ';') {
				temp_uri_location_info.cgi_path_info = temp_string;
				std::fstream exist_test;
				exist_test.open(temp_uri_location_info.cgi_path_info.c_str(), std::ios::in);
				if (!exist_test.is_open()) {
					return error__("conf_cgi_path_info", "invalid cgi_pass_info", line_number_count);
				}
				exist_test.close();
				prev_state = state;
				state	   = conf_start;
				next_state = conf_waiting_location_value;
				flag_location_part |= Kflag_cgi_path_info;
				temp_string.clear();
				break;
			}
			return error__("conf_cgi_path_info", "syntax error", line_number_count);
		}

		case conf_almost_done: {
			state = next_state;
			break;
		}

		default: {
			return error__("syntax error", "config file", line_number_count);
		}
		}
	}

#ifdef CONFIG_DEBUG
	std::cout << COLOR_RED << " -- config file parsed successfully -- " << COLOR_RESET << std::endl;
	for (total_port_server_map_p::iterator print = saved_total_port_map_3.begin(); print != saved_total_port_map_3.end(); ++print) {
		for (server_map_p::iterator print2 = print->second.begin(); print2 != print->second.end(); ++print2) {
			print2->second.print_();
		}
	}
#endif

	return spx_ok;
}
