#pragma once
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
#include <vector>

typedef struct main_info {
	uint32_t	  socket_size;
	port_info_vec port_info;
	// server_info_t const& search_server_config_(std::string const& host_name);
	// port_info[n].search_server_config_(host_name);

	void port_info_print_(void);

} main_info_t;

void kqueue_main(port_info_vec& serv_info);
void add_change_list(std::vector<struct kevent>& change_list,
					 uintptr_t ident, int64_t filter, uint16_t flags,
					 uint32_t fflags, intptr_t data, void* udata);

#endif
