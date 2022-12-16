#pragma once
#ifndef __SPACEX_HPP__
#define __SPACEX_HPP__

#include "spx_config_parse.hpp"
#include "spx_socket_init.hpp"
#include "spx_util_box.hpp"
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

typedef struct main_info {
	port_info_map port_info;
	// add handler function
} main_info_t;

#endif
