#pragma once
#ifndef __SPX__KQUEUE_MODULE__HPP
#define __SPX__KQUEUE_MODULE__HPP

#include "spx_client_buffer.hpp"

void add_change_list(std::vector<struct kevent>& change_list,
					 uintptr_t ident, int64_t filter, uint16_t flags,
					 uint32_t fflags, intptr_t data, void* udata);
bool create_client_event(uintptr_t serv_sd, struct kevent* cur_event,
						 std::vector<struct kevent>& change_list, port_info_t& port_info);
void read_event_handler(std::vector<port_info_t>& port_info, struct kevent* cur_event,
						std::vector<struct kevent>& change_list);
void kevent_error_handler(std::vector<port_info_t>& port_info, struct kevent* cur_event,
						  std::vector<struct kevent>& change_list);
void write_event_handler(std::vector<port_info_t>& port_info, struct kevent* cur_event,
						 std::vector<struct kevent>& change_list);
void proc_event_handler(struct kevent* cur_event, event_list_t& change_list);
void timer_event_handler(struct kevent* cur_event, event_list_t& change_list);
void cgi_handler(struct kevent* cur_event, event_list_t& change_list);
void kqueue_module(std::vector<port_info_t>& port_info);

#endif
