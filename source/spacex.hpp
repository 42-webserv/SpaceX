#pragma once
#ifndef __SPACEX_HPP__
#define __SPACEX_HPP__

#include "spx_core_type.hpp"
#include "spx_core_util_box.hpp"

#include "spx_parse_config.hpp"

typedef struct main_info {
	uint32_t	  socket_size;
	port_info_vec port_info;

} main_info_t;

#endif
