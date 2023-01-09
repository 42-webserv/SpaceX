#pragma once
#ifndef __SPX__CLIENT_BUFFER__HPP
#define __SPX__CLIENT_BUFFER__HPP

#include "spx_core_type.hpp"
#include "spx_core_util_box.hpp"

#include <algorithm>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/event.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>

#include <queue>

#include "spacex.hpp"
#include "spx_response_generator.hpp"
#include "spx_syntax_checker.hpp"

#include "spx_session_storage.hpp"

#define SERV_PORT 1234
#define SERV_SOCK_BACKLOG 10
#define EVENT_CHANGE_BUF 10
#define BUFFER_SIZE 32 * 1024
#define WRITE_BUFFER_MAX 32 * 1024

#define MAX_EVENT_LIST 200
// #define BUFFER_MAX 80 * 1024

class SessionStorage;
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
	REQ_LINE_PARSING = 0,
	REQ_HEADER_PARSING,
	REQ_BODY_CHUNKED,
	REQ_SKIP_BODY,
	REQ_SKIP_BODY_CHUNKED,
	REQ_CGI,
	REQ_HOLD
};

enum e_cgi_state {
	CGI_HEADER,
	CGI_BODY_CHUNKED,
	CGI_HOLD
};

enum e_req_flag { REQ_FILE_OPEN = 1 << 0,
				  READ_BODY_END = 1 << 1
};

enum e_res_flag { RES_FILE_OPEN = 1,
				  WRITE_READY	= 2,
				  RES_CGI		= 4 };

// gzip & deflate are not implemented.
enum e_transfer_encoding { TE_CHUNKED = 1 << 0,
						   TE_GZIP	  = 1 << 1,
						   TE_DEFLATE = 1 << 2 };

typedef std::vector<char>					buffer_t;
typedef std::vector<struct kevent>			event_list_t;
typedef std::pair<std::string, std::string> header_t;

class ReqField {
public:
	buffer_t						   chunked_body_buffer_;
	size_t							   chunked_checked_;
	size_t							   body_size_;
	size_t							   body_read_;
	int								   body_fd_;
	std::map<std::string, std::string> field_;
	std::string						   req_target_;
	std::string						   http_ver_;
	std::string						   file_path_;
	const server_info_t*			   serv_info_;
	const uri_location_t*			   uri_loc_;
	size_t							   body_limit_;
	size_t							   content_length_;
	int								   flag_;
	int								   req_type_;
	int								   transfer_encoding_;
	std::string						   session_id; // session

	ReqField()
		: chunked_body_buffer_()
		, chunked_checked_(0)
		, body_size_(0)
		, body_read_(0)
		, body_fd_(-1)
		, field_()
		, req_target_()
		, http_ver_()
		, file_path_()
		, uri_loc_()
		, body_limit_(-1)
		, content_length_(0)
		, flag_(0)
		, req_type_(0)
		, transfer_encoding_(0) { }
	~ReqField() { }
};

class ResField {
public:
	buffer_t						   cgi_buffer_;
	buffer_t						   res_buffer_;
	uri_resolved_t					   uri_resolv_;
	std::string						   file_path_;
	std::map<std::string, std::string> cgi_field_;
	size_t							   buf_size_;
	size_t							   body_read_;
	size_t							   body_size_;
	size_t							   sent_pos_;
	size_t							   cgi_checked_;
	size_t							   cgi_size_;
	size_t							   cgi_read_;
	int								   cgi_state_;
	int								   flag_;
	int								   body_fd_;
	int								   cgi_write_fd_;
	int								   header_ready_;
	int								   transfer_encoding_;

	/* RESPONSE*/
	std::vector<header_t> _headers;
	int					  _version_minor;
	int					  _version_major;
	unsigned int		  _status_code;
	std::string			  _status;

	int			file_open_(const char* dir) const;
	off_t		set_content_length_(int fd);
	void		set_content_type_(std::string uri);
	void		set_date_();
	std::string handle_static_error_page_();
	std::string make_to_string_() const;
	void		write_to_response_buffer_(const std::string& content);

	/* session & SESSION */
	void set_session_header_(std::string session_id);
	/* RESPONSE END*/

	ResField()
		: res_buffer_()
		, uri_resolv_()
		, file_path_()
		, buf_size_(0)
		, body_read_(0)
		, cgi_field_()
		, body_size_(0)
		, sent_pos_(0)
		, cgi_checked_(0)
		, cgi_size_(0)
		, cgi_read_(0)
		, cgi_state_(0)
		, flag_(0)
		, body_fd_(-1)
		, cgi_write_fd_(0)
		, header_ready_(0)
		, transfer_encoding_(0)
		, _headers()
		, _version_minor(1)
		, _version_major(1)
		, _status_code(200)
		, _status("OK") {
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
	session_storage_t								 _storage;
	std::queue<std::pair<req_field_t, res_field_t> > req_res_queue_;
	buffer_t										 rdsaved_;
	timespec										 timeout_;
	uintptr_t										 client_fd_;
	port_info_t*									 port_info_;
	size_t											 skip_size_;
	size_t											 rdchecked_;
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

	bool write_to_cgi(struct kevent* cur_event, std::vector<struct kevent>& change_list);
	bool write_response(event_list_t& change_list);
	bool write_for_upload(event_list_t& change_list, struct kevent* cur_event);

	bool cgi_header_parser();
	bool cgi_controller();
	// bool cgi_controller(int state, event_list_t& change_list);

	bool cgi_handler(struct kevent* cur_event, event_list_t& change_list);

	bool req_res_controller(event_list_t& change_list, struct kevent* cur_event);
	// bool skip_body(ssize_t cont_len);

	bool host_check(std::string& host);

	void read_to_client_buffer(event_list_t& change_list, struct kevent* cur_event);
	void read_to_cgi_buffer(event_list_t& change_list, struct kevent* cur_event);
	void read_to_res_buffer(event_list_t& change_list, struct kevent* cur_event);

	/* RESPONSE */
	void make_error_response(http_status error_code);
	void make_response_header();
	void make_redirect_response();
	void make_cgi_response_header();
	/* RESPONSE END*/
};

typedef ClientBuffer client_buf_t;

#endif
