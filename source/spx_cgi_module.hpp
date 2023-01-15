#pragma once
#ifndef __SPX__CGI__MODULE__HPP
#define __SPX__CGI__MODULE__HPP

#include "spx_client.hpp"

#include "spx_core_type.hpp"
#include "spx_core_util_box.hpp"

#include "spx_port_info.hpp"

typedef std::map<std::string, std::string> header_field_map;

class CgiModule {

private:
	uri_resolved_t const&	_cgi_resolved;
	header_field_map const& _header_map;
	uri_location_t const*	_cgi_loc_info;
	server_info_t const*	_server_info;

public:
	std::vector<std::string> vec_env_;
	std::vector<const char*> env_for_cgi_;

	CgiModule(uri_resolved_t const& org_cgi_loc, header_field_map const& req_header, uri_location_t const* cgi_loc_info, server_info_t const* server_info);
	~CgiModule();

	void made_env_for_cgi_(int status);
};

#endif
