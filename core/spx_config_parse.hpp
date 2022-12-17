#pragma once
#ifndef __SPX__CONFIG__PARSE__HPP__
#define __SPX__CONFIG__PARSE__HPP__

#include "spx_config_port_info.hpp"
#include "spx_core_type.hpp"
#include "spx_core_util_box.hpp"
#include <cctype>
#include <cstring>
#include <map>
#include <string>
#include <vector>

status
spx_config_syntax_checker(std::string const&	   buf,
						  total_port_server_map_p& config_map,
						  std::string const&	   cur_path);

#endif
