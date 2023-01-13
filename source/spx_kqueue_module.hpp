#pragma once
#ifndef __SPX__KQUEUE_MODULE__HPP
#define __SPX__KQUEUE_MODULE__HPP

#include "spx_client.hpp"

void add_change_list(event_list_t& change_list,
					 uintptr_t ident, int64_t filter, uint16_t flags,
					 uint32_t fflags, intptr_t data, void* udata);
bool create_client_event(uintptr_t serv_sd, struct kevent* cur_event,
						 event_list_t& change_list, port_info_t& port_info,
						 rdbuf_t* rdbuf, session_storage_t* storage);
void read_event_handler(port_list_t& port_info, struct kevent* cur_event,
						event_list_t& change_list, rdbuf_t* rdbuf,
						session_storage_t* storage);
void kevent_error_handler(port_list_t& port_info, struct kevent* cur_event,
						  event_list_t& change_list);
void write_event_handler(port_list_t& port_info, struct kevent* cur_event,
						 event_list_t& change_list);
void proc_event_wait_pid_(struct kevent* cur_event, event_list_t& change_list);
void timer_event_handler(struct kevent* cur_event, event_list_t& change_list);
void cgi_handler(struct kevent* cur_event, event_list_t& change_list);
void kqueue_module(port_list_t& port_info);

#endif
