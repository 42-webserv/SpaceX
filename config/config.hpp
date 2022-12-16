#pragma once
#ifndef __CONFIG__HPP__
#define __CONFIG__HPP__

#include "port_info.hpp"
#include "spacex_type.hpp"
#include <map>
#include <string>

status
spx_config_syntax_checker(std::string const&	   buf,
						  total_port_server_map_p& config_map);

#endif
