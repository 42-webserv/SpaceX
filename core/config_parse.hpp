#pragma once
#ifndef __CONFIG__PARSE__HPP__
#define __CONFIG__PARSE__HPP__

#include "core_type.hpp"
#include "port_info.hpp"
#include <map>
#include <string>

status
spx_config_syntax_checker(std::string const&	   buf,
						  total_port_server_map_p& config_map);

#endif
