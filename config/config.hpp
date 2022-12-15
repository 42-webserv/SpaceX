#pragma once
#ifndef __CONFIG__HPP__
#define __CONFIG__HPP__

#include "port_info.hpp"
#include "spacex_type.hpp"
#include <map>
#include <string>

// < port_number , < server_name, server_info_t> > my_config_map;
// < port_number , < server_name, < uri_location, uri_location_t> > > my_config_map;

status
spx_config_syntax_checker(std::string const&	   buf,
						  total_port_server_map_p& config_map);

#endif
