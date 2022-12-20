#pragma once
#include <vector>
#ifndef __SPACEX_HPP__
#define __SPACEX_HPP__

#include "spx_config_parse.hpp"
#include "spx_core_util_box.hpp"
#include "spx_socket_init.hpp"
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

typedef struct main_info {
	uint32_t				 socket_size;
	std::vector<port_info_t> port_info;
	// add handler function
} main_info_t;

#endif
