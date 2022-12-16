#pragma once
#ifndef __SPX_SOCKET_INIT_HPP__
#define __SPX_SOCKET_INIT_HPP__

#include "spx_port_info.hpp"
#include <arpa/inet.h>

#define LISTEN_BACKLOG_SIZE 10

typedef struct port_info {
	int				   listen_sd;
	struct sockaddr_in addr_server;
	uint32_t		   my_port;
	// server_info_t	   my_port_default_server_ptr;
	server_map_p my_port_map;
	// add search function

} port_info_t;

typedef std::map<const uint32_t, port_info_t> port_info_map;

status
socket_init_and_build_port_info(total_port_server_map_p& config_info,
								port_info_map&			 port_info);

int
socket_init(total_port_server_map_p const& config_info);

// typedef std::map<const std::string, const uri_location_t> uri_location_map_p;
// typedef std::map<const std::string, server_info_t>  server_map_p;
// typedef std::map<const std::uint32_t, server_map_p> total_port_server_map_p;

#endif
