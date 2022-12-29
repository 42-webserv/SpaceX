#pragma once
#include "spx_port_info.hpp"
#ifndef __SPX__CGI__MODULE__HPP
#define __SPX__CGI__MODULE__HPP

#include "spx_core_type.hpp"
#include "spx_core_util_box.hpp"
#include <vector>

typedef std::map<std::string, std::string> header_field_map;

class CgiModule {

private:
	header_field_map		 header_map_;
	uri_location_t const&	cgi_loc_;

public:
	std::vector<const char*> env_for_cgi_;
	CgiModule(uri_location_t const& uri_loc, header_field_map const& req_header);
	~CgiModule();

	void made_env_for_cgi_(void);
	static void check_cgi_response(void);
};

#endif
