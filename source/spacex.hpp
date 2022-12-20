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

void kqueue_main(std::vector<port_info_t>& serv_info);
void add_change_list(std::vector<struct kevent>& change_list,
					 uintptr_t ident, int64_t filter, uint16_t flags,
					 uint32_t fflags, intptr_t data, void* udata);

#endif
