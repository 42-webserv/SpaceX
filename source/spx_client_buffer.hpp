#pragma once
#ifndef __SPX__CLIENT_BUFFER__HPP
#define __SPX__CLIENT_BUFFER__HPP

#include <algorithm>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <stdlib.h>
#include <string>
#include <sys/event.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>

#include <map>
#include <queue>
#include <vector>

#include "spacex.hpp"
#include "spx_response_generator.hpp"
#include "spx_syntax_checker.hpp"

#define SERV_PORT 1234
#define SERV_SOCK_BACKLOG 10
#define EVENT_CHANGE_BUF 10
#define BUFFER_SIZE 8 * 1024
#define WRITE_BUFFER_MAX 40 * 1024
#define MAX_EVENT_LOOP 20
// #define BUFFER_MAX 80 * 1024

// struct Response;
enum e_request_method {
	REQ_GET		  = 1 << 1,
	REQ_POST	  = 1 << 2,
	REQ_PUT		  = 1 << 3,
	REQ_DELETE	  = 1 << 4,
	REQ_HEAD	  = 1 << 5,
	REQ_UNDEFINED = 1 << 6
};

enum e_client_buffer_flag {
	SOCK_WRITE	  = 128,
	READ_BODY	  = 256,
	RDBUF_CHECKED = 1 << 24,
	READ_READY	  = 1 << 30,
	E_BAD_REQ	  = 1 << 31
};

enum e_read_status {
	REQ_LINE_PARSING   = 0,
	REQ_HEADER_PARSING = 1,
	REQ_BODY		   = 2,
	REQ_CGI			   = 3
};

enum e_req_flag { REQ_FILE_OPEN = 1,
				  REQ_PARSED
};

enum e_res_flag { RES_FILE_OPEN = 1,
				  WRITE_READY	= 2,
				  RES_CGI		= 4 };

// gzip & deflate are not implemented.
enum e_transfer_encoding { TE_CHUNKED = 0,
						   TE_GZIP,
						   TE_DEFLATE };

typedef std::vector<char>					buffer_t;
typedef std::vector<struct kevent>			event_list_t;
typedef std::pair<std::string, std::string> header;

class ReqField {
public:
	buffer_t						   cgi_buffer_;
	int								   cgi_in_fd_;
	int								   cgi_out_fd_;
	std::map<std::string, std::string> field_;
	std::string						   req_target_;
	std::string						   http_ver_;
	std::string						   file_path_;
	const server_info_t*			   serv_info_;
	const uri_location_t*			   uri_loc_;
	size_t							   body_recieved_;
	size_t							   body_limit_;
	size_t							   content_length_;
	int								   flag_;
	int								   req_type_;
	int								   transfer_encoding_;

	ReqField()
		: cgi_buffer_()
		, cgi_in_fd_()
		, cgi_out_fd_()
		, field_()
		, req_target_()
		, http_ver_()
		, file_path_()
		, uri_loc_()
		, body_recieved_(0)
		, body_limit_(-1)
		, content_length_(0)
		, flag_(0)
		, req_type_(0)
		, transfer_encoding_(0) {
	}
	~ReqField() {
	}
};

class ResField {
public:
	buffer_t	   res_buffer_;
	uri_resolved_t uri_resolv_;
	std::string	   file_path_;
	size_t		   buf_size_;
	int			   body_fd_;
	int			   header_ready_;
	int			   sent_pos_;
	int			   flag_;
	int			   transfer_encoding_;

	/* RESPONSE*/
	std::vector<header> headers_;
	int					version_minor_;
	int					version_major_;
	unsigned int		status_code_;
	std::string			status_;

	int			file_open(const char* dir) const;
	off_t		setContentLength(int fd);
	void		setContentType(std::string uri);
	void		setDate();
	std::string handle_static_error_page();
	std::string make_to_string() const;
	void		write_to_response_buffer(const std::string& content);

	/* RESPONSE END*/

	ResField()
		: res_buffer_()
		, uri_resolv_()
		, file_path_()
		, buf_size_(0)
		, body_fd_(-1)
		, header_ready_(0)
		, sent_pos_(0)
		, flag_(0)
		, transfer_encoding_(0) {
	}

	~ResField() {
	}

	//
};

typedef ResField res_field_t;
typedef ReqField req_field_t;

class ClientBuffer {
private:
	ClientBuffer(const ClientBuffer& buf);
	ClientBuffer& operator=(const ClientBuffer& buf);

public:
	std::queue<std::pair<req_field_t, res_field_t> > req_res_queue_;
	buffer_t										 rdsaved_;
	timespec										 timeout_;
	uintptr_t										 client_fd_;
	port_info_t*									 port_info_;
	int												 rdchecked_;
	int												 flag_;
	int												 state_;
	char											 rdbuf_[BUFFER_SIZE];

	// TEMP Implement
	ClientBuffer();
	~ClientBuffer();

	void write_filter_enable(event_list_t& change_list, struct kevent* cur_event);

	bool request_line_check(std::string& req_line);
	bool request_line_parser();

	bool header_field_parser();

	void disconnect_client(event_list_t& change_list);

	bool write_response(uintptr_t fd, event_list_t& change_list);

	bool req_res_controller(event_list_t& change_list, struct kevent* cur_event);
	bool skip_body(ssize_t cont_len);

	bool host_check(std::string& host);

	void read_to_client_buffer(event_list_t& change_list, struct kevent* cur_event);
	void read_to_cgi_buffer(event_list_t& change_list, struct kevent* cur_event);
	void read_to_res_buffer(event_list_t& change_list, struct kevent* cur_event);

	/* RESPONSE */
	void make_error_response(http_status error_code);
	void make_response_header();
	void make_redirect_response();
	/* RESPONSE END*/
};

typedef ClientBuffer client_buf_t;

#endif