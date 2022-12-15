#pragma once

#include <map>
#include <string>
#include <vector>

#define SERV_PORT 1234
#define SERV_SOCK_BACKLOG 10
#define EVENT_CHANGE_BUF 10
#define BUFFER_SIZE 8 * 1024
#define BUFFER_MAX 80 * 1024

typedef std::vector<char> t_buffer;

typedef struct ReqField {
	std::map<std::string, std::string> field_;
	t_buffer						   req_body_;
	std::string						   req_target_;
	std::string						   http_ver_;
	size_t							   body_recieved_;
	size_t							   body_limit_;
	size_t							   content_length_;
	int								   body_flag_;
	int								   req_type_;

	ReqField()
		: field_()
		, req_body_()
		, req_target_()
		, http_ver_()
		, body_recieved_(0)
		, body_limit_(-1)
		, content_length_(0)
		, body_flag_(0)
		, req_type_(0) {
	}
	~ReqField() {
	}
} t_req_field;
