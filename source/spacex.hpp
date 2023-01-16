#pragma once
#ifndef __SPACEX_HPP__
#define __SPACEX_HPP__

#include "spx_core_type.hpp"
#include "spx_core_util_box.hpp"

#include "spx_parse_config.hpp"

typedef struct main_info {
	int64_t		  socket_size;
	int64_t		  first_socket;
	port_info_vec port_info;

} main_info_t;

#endif
