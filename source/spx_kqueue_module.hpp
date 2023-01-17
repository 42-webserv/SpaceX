#pragma once
#ifndef __SPX__KQUEUE_MODULE__HPP
#define __SPX__KQUEUE_MODULE__HPP

#include "spx_client.hpp"

#ifndef EXPIRED_CLEANER_TIME
#define EXPIRED_CLEANER_TIME 10
#endif

void kqueue_module(port_list_t& port_info);

void add_change_list(event_list_t& change_list, uintptr_t ident, int64_t filter,
					 uint16_t flags, uint32_t fflags, intptr_t data, void* udata);

bool create_client_event(uintptr_t serv_sd, event_list_t& change_list, port_info_t& port_info,
						 rdbuf_t* rdbuf, session_storage_t* storage);

void read_event_handler(port_list_t& port_info, struct kevent* cur_event,
						event_list_t& change_list, rdbuf_t* rdbuf, session_storage_t* storage);
void write_event_handler(struct kevent* cur_event);
void proc_event_wait_pid(struct kevent* cur_event);

void kevent_error_handler(port_list_t&	 port_info,
						  struct kevent* cur_event, close_vec_t& for_close);
void kevent_eof_handler(struct kevent* cur_event, close_vec_t& for_close);

void timer_event_handler(struct kevent* cur_event,
						 event_list_t& change_list, close_vec_t& for_close);
void session_timer_event_handler(struct kevent* cur_eventt);

#endif
