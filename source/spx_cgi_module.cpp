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
	case REQ_##name:  \
		return #name;
			METHOD__MAP(XX)
#undef XX
		default:
			return "<unknown>";
		}
	}

	// std::string
	// method_map_str_(int const& status) {
	// 	switch (status) {
	// 	case REQ_GET:
	// 		return "GET";
	// 	case REQ_POST:
	// 		return "POST";
	// 	case REQ_PUT:
	// 		return "PUT";
	// 	case REQ_HEAD:
	// 		return "HEAD";
	// 	case REQ_DELETE:
	// 		return "DELETE";
	// 	}
	// }

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

} // namespace

CgiModule::CgiModule(uri_resolved_t const& org_cgi_loc, header_field_map const& req_header, uri_location_t const* cgi_loc_info)
	: cgi_resolved_(org_cgi_loc)
	, header_map_(req_header)
	, cgi_loc_info_(cgi_loc_info) {
}

CgiModule::~CgiModule() { }

void
CgiModule::made_env_for_cgi_(int status) {

	{ // pixed part
		vec_env_.push_back("GATEWAY_INTERFACE=CGI/1.1");
		vec_env_.push_back("REMOTE_ADDR=127.0.0.1");
		vec_env_.push_back("SERVER_SOFTWARE=SPX/1.0");
		vec_env_.push_back("SERVER_PROTOCOL=HTTP/1.1");
	}

	{ // variable part
		std::string method = method_map_str_(status);
		if (method != "<unknown>") {
			vec_env_.push_back("REQUEST_METHOD=" + method); // GET|POST|...
		}
		vec_env_.push_back("REQUEST_URI=" + cgi_resolved_.resolved_request_uri_); // /blah/blah/blah.cgi/remain/blah/blah
		vec_env_.push_back("SCRIPT_NAME=" + cgi_resolved_.script_name_); // /blah/blah/blah.cgi
		if (!cgi_resolved_.path_info_.empty()) {
			vec_env_.push_back("PATH_INFO=" + cgi_resolved_.path_info_); // remain /blah/blah
		} else {
			vec_env_.push_back("PATH_INFO=" + cgi_resolved_.resolved_request_uri_); // remain /blah/blah
		}
		if (!cgi_resolved_.query_string_.empty()) {
			vec_env_.push_back("QUERY_STRING=" + cgi_resolved_.query_string_); // key=value&key=value&key=value
		}
		if (cgi_loc_info_ && !(cgi_loc_info_->saved_path.empty())) {
			vec_env_.push_back("SAVED_PATH=" + cgi_loc_info_->saved_path); // /blah/blah/
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
		std::cout << vec_env_[i].c_str() << std::endl;
	}
	env_for_cgi_.push_back(NULL);
}
