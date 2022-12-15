#pragma once

#include <queue>
#include <string>

#include "flags.hpp"
#include "request_field.hpp"
#include "response_field.hpp"

typedef struct ClientBuffer {
	std::map<std::string, std::string>				 field_;
	std::queue<std::pair<t_req_field, t_res_field> > req_res_queue_;
	std::string										 rdsaved_;
	timespec										 timeout_;
	uintptr_t										 client_fd;
	char											 rbuf_[BUFFER_SIZE];
	int												 rdchecked_;
	int												 req_type_;
	int												 flag_;

	ClientBuffer()
		: field_()
		, req_res_queue_()
		, rdsaved_()
		, timeout_()
		, client_fd()
		, rbuf_()
		, rdchecked_(0)
		, req_type_(0)
		, flag_(0) {
	}
	~ClientBuffer() {
	}
} t_client_buf;
