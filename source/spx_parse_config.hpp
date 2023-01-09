#pragma once
#ifndef __SPX__PARSE__CONFIG__HPP__
#define __SPX__PARSE__CONFIG__HPP__

#include "spx_core_type.hpp"
#include "spx_core_util_box.hpp"
#include "spx_port_info.hpp"

status
spx_config_syntax_checker(std::string const&	   buf,
						  total_port_server_map_p& config_map,
						  std::string const&	   cur_path);

#endif
