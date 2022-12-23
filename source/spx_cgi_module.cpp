#include "spx_cgi_module.hpp"
#include <vector>

CgiModule::CgiModule(uri_location_t const& uri_loc, header_field_map const& req_header)
	: cgi_pass_(uri_loc.cgi_pass)
	, cgi_path_info_(uri_loc.cgi_path_info)
	, saved_path_(uri_loc.saved_path)
	, root_path_(uri_loc.root)
	, header_map_(req_header) {
}

void
CgiModule::made_env_for_cgi_(void) const {
	uint32_t count_for_env_size = 17;

	std::vector<std::string> vec_env_;
	vec_env_.push_back("AUTH_TYPE=");
	// AUTH_TYPE identification type, if applicable. 'auth-scheme' token in the request Authorization header field. 서블릿을 보호하는 데 사용되는 인증 스키마의 이름입니다. 예를 들면 BASIC, SSL 또는 서블릿이 보호되지 않는 경우 null입니다.
	vec_env_.push_back("GATEWAY_INTERFACE=CGI/1.1");
	// GATEWAY_INTERFACE CGI/1.1

	// mandatory_env[1] = "CONTENT_LENGTH=          ";
	// // CONTENT_LENGTH similarly, size of input data (decimal, in octets) if provided via HTTP header.
	// mandatory_env[2] = "CONTENT_TYPE=application/x-www-form-urlencoded";
	// // CONTENT_TYPE Internet media type of input data if PUT or POST method are used, as provided via HTTP header.
	// mandatory_env[4] = "PATH_INFO=" + cgi_path_info_;
	// // PATH_INFO URI which request arrived? or substitution path
	// mandatory_env[5] = "PATH_TRANSLATED=" + saved_path_ + cgi_path_info_;
	// // PATH_TRANSLATED PWD/PATH_INFO
	// mandatory_env[6] = "QUERY_STRING=";
	// // QUERY_STRING (ex: var1=value1&var2=with%20percent%20encoding )
	// mandatory_env[7] = "REMOTE_ADDR=";
	// // REMOTE_ADDR IP address of the client (dot-decimal) (ex: "127.0.0.1" )
	// mandatory_env[8] = "REMOTE_IDENT=";
	// // REMOTE_IDENT see ident, only if server performed such lookup.
	// mandatory_env[9] = "REMOTE_USER=";
	// // REMOTE_USER used for certain AUTH_TYPEs.
	// mandatory_env[10] = "REQUEST_METHOD=GET";
	// // REQUEST_METHOD METHOD(upper case) which request arrived
	// mandatory_env[11] = "REMOTE_HOST=";
	// // REMOTE_HOST host ( host name of the client, unset if server did not perform such lookup. )
	// mandatory_env[12] = "SCRIPT_NAME=" + cgi_pass_;
	// // SCRIPT_NAME CGI_PASS ( like /cgi-bin/script.cgi )
	// mandatory_env[13] = "SERVER_NAME=";
	// // SERVER_NAME 127.0.0.1
	// mandatory_env[14] = "SERVER_PORT=80";
	// // SERVER_PORT PORT_NUMBER which request arrived
	// mandatory_env[15] = "SERVER_PROTOCOL=HTTP/1.1";
	// // SERVER_PROTOCOL HTTP/1.1
	// mandatory_env[16] = "SERVER_SOFTWARE=SPX/1.0";
	// // SERVER_SOFTWARE argv[0]/version 1.1
}
