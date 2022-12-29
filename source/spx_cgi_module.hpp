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
	const uri_resolved_t& cgi_loc_;
	header_field_map	  header_map_;

public:
	std::vector<const char*> env_for_cgi_;

	CgiModule(uri_resolved_t const& uri_loc, header_field_map const& req_header);
	~CgiModule();

	void		made_env_for_cgi_(int status);
	static void check_cgi_response(void);
};

#endif
