#pragma once
#include "spx_port_info.hpp"
#ifndef __SPX__CGI__MODULE__HPP
#define __SPX__CGI__MODULE__HPP

#include "spx_client_buffer.hpp"
#include "spx_core_type.hpp"
#include "spx_core_util_box.hpp"
#include "spx_port_info.hpp"
#include <vector>

typedef std::map<std::string, std::string> header_field_map;

class CgiModule {

private:
	header_field_map const& 	header_map_;
	uri_resolved_t const&		cgi_loc_;
	uri_location_t *		cgi_loc_info_;

public:
	std::vector<const char*> env_for_cgi_;

	CgiModule(uri_resolved_t const& org_cgi_loc, header_field_map const& req_header, uri_location_t const* cgi_loc_info);
	~CgiModule();

	void		made_env_for_cgi_(int status);
	static void check_cgi_response(void);
};

#endif
