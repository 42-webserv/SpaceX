#include "spx_cgi_module.hpp"
#include "spx_core_type.hpp"
#include <cstring>
#include <string>

namespace {

#define METHOD__MAP(XX) \
	XX(1, GET)          \
	XX(2, POST)         \
	XX(3, PUT)          \
	XX(4, DELETE)       \
	XX(5, HEAD)

	std::string
	method_map_str_(int const& status) {
		switch (status) {
#define XX(num, name) \
	case #num:        \
		return #name;
			METHOD__MAP(XX)
#undef XX
		default:
			return "<unknown>";
		}
	};

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

	inline status
	error_flag_(const char* msg, int& flag) {
#ifdef SYNTAX_DEBUG
		std::cout << "\033[1;31m" << msg << "\033[0m"
				  << " : "
				  << "\033[1;33m"
				  << ""
				  << "\033[0m" << std::endl;
#else
		(void)msg;
#endif
		flag = REQ_UNDEFINED;
		return spx_error;
	}

	status
	check_content_type_(std::vector<std::string>&		   temp_header,
						std::string&					   temp,
						std::vector<char>&				   cgi_response,
						std::vector<char>::const_iterator& it) {

		temp.push_back("Content-Type:");
		while (it != cgi_response.end() && syntax_(name_token_, static_cast<uint8_t>(*it))) {
			temp.push_back(*it);
			++it;
		}
		if (it != cgi_response.end() && *it == '/') {
			temp.push_back(*it);
			++it;
			while (it != cgi_response.end() && syntax_(name_token_, static_cast<uint8_t>(*it))) {
				temp.push_back(*it);
				++it;
			}
			if (it != cgi_response.end() && *it == '\n') {
				temp_header.push_back(temp);
				temp.clear();
				++it;
				return spx_ok;
			} else if (it != cgi_response.end() && *it == ';') {
				temp.push_back(*it);
				++it;
				if (it != cgi_response.end() && syntax_(name_token_, static_cast<uint8_t>(*it))) {

					enum {
						param_attr,
						param_value,
						param_quoted_open,
						param_quoted_close,
						param_semicolon,
						param_almost_done,
						param_done
					} state;
					state = param_attr;

					while (state != param_done) {
						switch (state) {
						case param_attr: {
							while (it != cgi_response.end() && syntax_(name_token_, static_cast<uint8_t>(*it))) {
								temp.push_back(*it);
								++it;
							}
							if (it != cgi_response.end() && *it == '=') {
								temp.push_back(*it);
								++it;
								state = param_value;
							} else {
								return error_("Content-Type syntax error : missing '='");
							}
							break;
						}
						case param_value: {
							while (it != cgi_response.end() && syntax_(name_token_, static_cast<uint8_t>(*it))) {
								temp.push_back(*it);
								++it;
							}
							if (it != cgi_response.end()) {
								switch (*it) {
								case '"': {
									temp.push_back(*it);
									++it;
									state = param_quoted_open;
									break;
								}
								case ';': {
									temp.push_back(*it);
									++it;
									state = param_semicolon;
									break;
								}
								case '\n': {
									state = param_almost_done;
									break;
								}
								default: {
									return error_("Content-Type syntax error : invalid parameter value end");
								}
								}
							} else {
								return error_("Content-Type syntax error : reached end of response");
							}
							break;
						}
						case pram_almost_done: {
							if (it != cgi_response.end() && *it == '\n') {
								++it;
								state = param_done;
							} else {
								return error_("Content-Type syntax error : missing endline");
							}
							break;
						}
						}
					}
					return spx_ok;
				}
				return error_("Content-Type syntax error : missing parameter name");
			} else {
				return error_("Content-Type syntax error : missing endline");
			}
		}
		return error_("Content-Type syntax error : missing '/'");
	}

} // namespace

CgiModule::CgiModule(uri_resolved_t const& org_cgi_loc, header_field_map const& req_header, uri_location_t const* cgi_loc_info)
	: cgi_resolved_(org_cgi_loc)
	, header_map_(req_header)
	, cgi_loc_info_(cgi_loc_info) {
}

CgiModule::~CgiModule() { }

void
CgiModule::made_env_for_cgi_(int status) {

	std::vector<std::string> vec_env_;

	{ // pixed part
		vec_env_.push_back("GATEWAY_INTERFACE=CGI/1.1");
		vec_env_.push_back("REMOTE_ADDR=127.0.0.1");
		vec_env_.push_back("SERVER_SOFTWARE=SPX/1.0");
		vec_env_.push_back("SERVER_PROTOCOL=HTTP/1.1");
	}

	{ // variable part
		std::string method = method_map_str_(status) if (!method.empty()) {
			vec_env_.push_back("REQUEST_METHOD=" + method); // GET|POST|...
		}
		vec_env_.push_back("REQUEST_URI=" + cgi_resolved_.request_uri_); // /blah/blah/blah.cgi/remain/blah/blah
		vec_env_.push_back("SCRIPT_NAME=" + cgi_resolved_.script_name_); // /blah/blah/blah.cgi
		if (!cgi_resolved_.path_info_.empty()) {
			vec_env_.push_back("PATH_INFO=" + cgi_resolved_.path_info_); // remain /blah/blah
		}
		if (!cgi_resolved_.query_string_.empty()) {
			vec_env_.push_back("QUERY_STRING=" + cgi_resolved_.query_string_); // key=value&key=value&key=value
		}
		if (cgi_loc_info_ && !(cgi_loc_info_->save_path_.empty())) {
			vec_env_.push_back("SAVED_PATH=" + cgi_loc_info_->save_path_); // /blah/blah/
		}
		// vec_env_.push_back("SERVER_NAME=" + ); // server name from server_info_t //TODO : need to add server_name
		// vec_env_.push_back("SERVER_PORT=" +); // server port from server_info_t
	}

	for (header_field_map::const_iterator it = header_map_.begin(); it != header_map_.end(); ++it) {
		switch (it->first.at(0)) {
		case 'a': {
			if (it->first == "a-im") {
				vec_env_.push_back("HTTP_A_IM=" + it->second);
			} else if (it->first.at(1) == 'c') {
				if (it->first == "accept") {
					vec_env_.push_back("HTTP_ACCEPT=" + it->second);
				} else if (it->first == "accept-charset") {
					vec_env_.push_back("HTTP_ACCEPT_CHARSET=" + it->second);
				} else if (it->first == "accept-datatime") {
					vec_env_.push_back("HTTP_ACCEPT_DATATIME=" + it->second);
				} else if (it->first == "accept-encoding") {
					vec_env_.push_back("HTTP_ACCEPT_ENCODING=" + it->second);
				} else if (it->first == "accept-language") {
					vec_env_.push_back("HTTP_ACCEPT_LANGUAGE=" + it->second);
				} else {
					vec_env_.push_back("X_" + it->first + "=" + it->second);
				}
			} else if (it->first == "authorization") {
				vec_env_.push_back("HTTP_AUTHORIZATION=" + it->second);
				// } else if (it->first == "auth-scheme") {
				// 	vec_env_.push_back("AUTH_TYPE=" + it->second);
			} else {
				vec_env_.push_back("X_" + it->first + "=" + it->second);
			}
			break;
		}
		case 'c': {
			if (it->first == "cache-control") {
				vec_env_.push_back("HTTP_CACHE_CONTROL=" + it->second);
			} else if (it->first == "connection") {
				vec_env_.push_back("HTTP_CONNECTION=" + it->second);
			} else if (it->first == "content-endcoding") {
				vec_env_.push_back("HTTP_CONTENT_ENDCODING=" + it->second);
			} else if (it->first == "content-length") {
				vec_env_.push_back("HTTP_CONTENT_LENGTH=" + it->second);
				vec_env_.push_back("CONTENT_LENGTH=" + it->second);
			} else if (it->first == "content-md5") {
				vec_env_.push_back("HTTP_CONTENT_MD5=" + it->second);
			} else if (it->first == "content-type") {
				vec_env_.push_back("HTTP_CONTENT_TYPE=" + it->second);
				vec_env_.push_back("CONTENT_TYPE=" + it->second);
			} else if (it->first == "cookie") {
				vec_env_.push_back("HTTP_COOKIE=" + it->second);
			} else {
				vec_env_.push_back("X_" + it->first + "=" + it->second);
			}
			break;
		}
		case 'd': {
			if (it->first == "date") {
				vec_env_.push_back("HTTP_DATE=" + it->second);
			} else {
				vec_env_.push_back("X_" + it->first + "=" + it->second);
			}
			break;
		}
		case 'e': {
			if (it->first == "expect") {
				vec_env_.push_back("HTTP_EXPECT=" + it->second);
			} else {
				vec_env_.push_back("X_" + it->first + "=" + it->second);
			}
			break;
		}
		case 'f': {
			if (it->first == "from") {
				vec_env_.push_back("HTTP_FROM=" + it->second);
			} else if (it->first == "forwarded") {
				vec_env_.push_back("HTTP_FORWARDED=" + it->second);
			} else {
				vec_env_.push_back("X_" + it->first + "=" + it->second);
			}
			break;
		}
		case 'h': {
			if (it->first == "host") {
				vec_env_.push_back("HTTP_HOST=" + it->second);
			} else if (it->first == "host2-settings") {
				vec_env_.push_back("HTTP_HOST2_SETTINGS=" + it->second);
			} else {
				vec_env_.push_back("X_" + it->first + "=" + it->second);
			}
			break;
		}
		case 'i': {
			if (it->first.at(1) == 'f') {
				if (it->first == "if-match") {
					vec_env_.push_back("HTTP_IF_MATCH=" + it->second);
				} else if (it->first == "if-modified-since") {
					vec_env_.push_back("HTTP_IF_MODIFIED_SINCE=" + it->second);
				} else if (it->first == "if-none-match") {
					vec_env_.push_back("HTTP_IF_NONE_MATCH=" + it->second);
				} else if (it->first == "if-range") {
					vec_env_.push_back("HTTP_IF_RANGE=" + it->second);
				} else if (it->first == "if-unmodified-since") {
					vec_env_.push_back("HTTP_IF_UNMODIFIED_SINCE=" + it->second);
				} else {
					vec_env_.push_back("X_" + it->first + "=" + it->second);
				}
			} else {
				vec_env_.push_back("X_" + it->first + "=" + it->second);
			}
			break;
		}
		case 'm': {
			if (it->first == "max-forwards") {
				vec_env_.push_back("HTTP_MAX_FORWARDS=" + it->second);
			} else {
				vec_env_.push_back("X_" + it->first + "=" + it->second);
			}
			break;
		}
		case 'o': {
			if (it->first == "origin") {
				vec_env_.push_back("HTTP_ORIGIN=" + it->second);
			} else {
				vec_env_.push_back("X_" + it->first + "=" + it->second);
			}
			break;
		}
		case 'p': {
			if (it->first == "pragma") {
				vec_env_.push_back("HTTP_PRAGMA=" + it->second);
			} else if (it->first == "prefer") {
				vec_env_.push_back("HTTP_PREFER=" + it->second);
			} else if (it->first == "proxy-authorization") {
				vec_env_.push_back("HTTP_PROXY_AUTHORIZATION=" + it->second);
			} else {
				vec_env_.push_back("X_" + it->first + "=" + it->second);
			}
			break;
		}
		case 'r': {
			if (it->first == "range") {
				vec_env_.push_back("HTTP_RANGE=" + it->second);
			} else if (it->first == "referer") {
				vec_env_.push_back("HTTP_REFERER=" + it->second);
			} else {
				vec_env_.push_back("X_" + it->first + "=" + it->second);
			}
			break;
		}
		case 't': {
			if (it->first == "te") {
				vec_env_.push_back("HTTP_TE=" + it->second);
			} else if (it->first == "trailer") {
				vec_env_.push_back("HTTP_TRAILER=" + it->second);
			} else if (it->first == "transfer-encoding") {
				vec_env_.push_back("HTTP_TRANSFER_ENCODING=" + it->second);
			} else {
				vec_env_.push_back("X_" + it->first + "=" + it->second);
			}
			break;
		}
		case 'u': {
			if (it->first == "upgrade") {
				vec_env_.push_back("HTTP_UPGRADE=" + it->second);
			} else if (it->first == "user-agent") {
				vec_env_.push_back("HTTP_USER_AGENT=" + it->second);
			} else {
				vec_env_.push_back("X_" + it->first + "=" + it->second);
			}
			break;
		}
		case 'v': {
			if (it->first == "via") {
				vec_env_.push_back("HTTP_VIA=" + it->second);
			} else {
				vec_env_.push_back("X_" + it->first + "=" + it->second);
			}
			break;
		}
		case 'w': {
			if (it->first == "warning") {
				vec_env_.push_back("HTTP_WARNING=" + it->second);
			} else {
				vec_env_.push_back("X_" + it->first + "=" + it->second);
			}
			break;
		}
		default: {
			vec_env_.push_back("X_" + it->first + "=" + it->second);
		}
		}
	}

	for (uint32_t i = 0; i < vec_env_.size(); ++i) {
		env_for_cgi_.push_back(vec_env_[i].c_str());
	}
	env_for_cgi_.push_back(NULL);
}

static status
CgiModule::check_cgi_response(std::vector<char>&		cgi_response,
							  std::vector<std::string>& cgi_header,
							  uint32_t&					status_code,
							  uint64_t&					body_pos) {

	if (cgi_response.size() < 2) {
		return spx_error;
	}
	// check content-length
	// if chunked, decode chunked

	// check content-type

	enum {
		cgi__start, // check what response type
		cgi__document,
		cgi__status,
		cgi__body,
		cgi__location,
		cgi__local_redirect,
		cgi__client_redirect,
		cgi__cliend_redirect_document,
		cgi__almost_done,
		cgi__done
	} state,
		next_state;

	body_pos = 0;

	std::string				 location	  = "Location:";
	std::string				 content_type = "Content-Type:";
	std::string				 status		  = "Status:";
	std::string				 temp;
	std::vector<std::string> temp_header;

	state								 = cgi__start;
	std::vector<char>::const_iterator it = cgi_response.begin();

	while (state != cgi__done) {
		switch (state) {
		case cgi__start: {
			if (std::equal(location.begin(), location.end(), it)) {
				it += location.size();
				state = cgi__location;
				break;
			} else if (std::equal(content_type.begin(), content_type.end(), it)) {
				it += content_type.size();
				state = cgi__document;
				break;
			}
			return error_("first cgi response line is not location or content-type");
		}

		case cgi__document: {
			if (*it == '\n') {
				temp_header.push_back(std::string("Content-Type:"));
				state = cgi__status;
				break;
			}
			if (it != cgi_response.end() && syntax_(name_token_, static_cast<uint8_t>(*it))) {
				if (check_content_type_(temp_header, temp, cgi_response, it) == spx_error) {
					return error_("Content-Type syntax error");
				}
				temp_header.push_back(temp);
				temp.clear();
				state = cgi__status;
				break;
			}
			return error_("Content-Type syntax error");
		}
		}
	}
	return spx_ok;
}
