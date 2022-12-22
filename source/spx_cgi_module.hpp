#pragma once
#include "spx_config_port_info.hpp"
#ifndef __SPX__CGI__MODULE__HPP
#define __SPX__CGI__MODULE__HPP

#include "spx_core_type.hpp"
#include "spx_core_util_box.hpp"

typedef std::map<std::string, std::string> header_field_map;

class CgiModule {

private:
	mutable char**			env_for_cgi_;
	header_field_map const& header_map_;
	std::string const&		cgi_pass_;
	std::string const&		cgi_path_info_;
	std::string const&		saved_path_;
	std::string const&		root_path_;

public:
	CgiModule(uri_location_t const& uri_loc, header_field_map const& req_header);
	~CgiModule();

	void made_env_for_cgi_(void) const;
};

#endif
