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
#include "spx_session_storage.hpp"
#include "spx_syntax_checker.hpp"

#define BUFFER_SIZE 8 * 1024
#define IOVEC_LEN 8
#define WRITE_BUFFER_MAX 32 * 1024

#define MAX_EVENT_LIST 200

class SessionStorage;

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
typedef std::vector<struct iovec>			iov_t;
typedef std::map<std::string, std::string>	cgi_header_t;
typedef std::map<std::string, std::string>	req_header_t;
typedef std::pair<std::string, std::string> header;

class CgiField {
public:
	cgi_header_t _cgi_header;
	iov_t		 _from_cgi;
	iov_t		 _to_cgi;
	off_t		 _from_cgi_ofs;
	off_t		 _to_cgi_ofs;
	size_t		 _cgi_size;
	size_t		 _cgi_read;
	int			 _is_chnkd;
	int			 _cgi_write_fd;

	CgiField()
		: _cgi_header()
		, _from_cgi()
		, _to_cgi()
		, _from_cgi_ofs(0)
		, _to_cgi_ofs(0)
		, _cgi_size(0)
		, _cgi_read(0)
		, _is_chnkd(0)
		, _cgi_write_fd(0) {};
	~CgiField() {};

	void
	clear_() {
		_cgi_header.clear();
		_from_cgi.clear();
		_to_cgi.clear();
		_from_cgi_ofs = 0;
		_to_cgi_ofs	  = 0;
		_cgi_size	  = 0;
		_cgi_read	  = 0;
		_is_chnkd	  = 0;
		_cgi_write_fd = 0;
	}
};

class ChunkedField {
public:
	iov_t _chnk_body;
	off_t _chnk_ofs;

	void
	clear_() {
		_chnk_ofs = 0;
	}
};

class ReqField {
public:
	size_t				  _body_size;
	size_t				  _body_read;
	size_t				  _body_limit;
	size_t				  _cnt_len;
	int					  _body_fd;
	req_header_t		  _header;
	std::string			  _uri;
	std::string			  _http_ver;
	std::string			  _upld_fn;
	const server_info_t*  _serv_info;
	const uri_location_t* _uri_loc;
	int					  _req_mthd;
	int					  _is_chnkd;
	int					  flag_;
	std::string			  session_id; // session

	ReqField()
		: _body_size(0)
		, _body_read(0)
		, _body_limit(-1)
		, _cnt_len(0)
		, _body_fd(-1)
		, _header()
		, _uri()
		, _http_ver()
		, _upld_fn()
		, _uri_loc()
		, flag_(0)
		, _req_mthd(0)
		, _is_chnkd(0) { }
	~ReqField() { }

	void
	clear_() {
	}
};

class ResField {
public:
	std::string	   _res_header;
	std::string	   _dwnl_fn;
	iov_t		   _res_buf;
	uri_resolved_t _uri_resolv;
	size_t		   _body_read;
	size_t		   _body_size;
	int			   _body_fd;
	int			   _is_chnkd;
	int			   _header_sent;

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

	/* session & SESSION */
	void setSessionHeader(std::string session_id);
	/* RESPONSE END*/

	ResField()
		: _res_header()
		, _dwnl_fn()
		, _res_buf()
		, _uri_resolv()
		, _body_fd(-1)
		, _body_read(0)
		, _body_size(0)
		, _is_chnkd(0)
		, _header_sent(0)
		, headers_()
		, version_minor_(1)
		, version_major_(1)
		, status_code_(200)
		, status_("OK") {
	}

	~ResField() {
	}

	void
	clear_() {
		_res_header.clear();
		_dwnl_fn.clear();
		_body_fd	 = -1;
		_body_read	 = 0;
		_body_size	 = 0;
		_is_chnkd	 = 0;
		_header_sent = 0;
		headers_.clear();
		version_minor_ = 1;
		version_major_ = 1;
		status_code_   = 200;
		status_		   = "OK";
		_res_buf.clear();
	};
};

typedef ResField	 res_field_t;
typedef ReqField	 req_field_t;
typedef CgiField	 cgi_field_t;
typedef ChunkedField chunked_field_t;

class Client {
private:
	Client();
	Client(const Client& client);
	Client& operator=(const Client& client);

public:
	event_list_t*		   change_list;
	ReqField			   _req;
	ResField			   _res;
	CgiField			   _cgi;
	ChunkedField		   _chnkd;
	uintptr_t			   _client_fd;
	iov_t				   _rdbuf;
	int					   _rd_ofs; // read point of _rdbuf.front()
	int					   _rdbuf_end; // _rdbuf empty pos. like vector end iterator.
	int					   _state;
	port_info_t*		   _port_info;
	const struct sockaddr* _sockaddr;
	SessionStorage		   storage; // add by space

	Client(event_list_t* chnage_list);
	~Client();

	void iov_clear_(iov_t& buf);

	void reset_();
	void read_buf_set(iov_t& rdbuf);

	bool request_line_check(std::string& req_line);
	bool request_line_parser();
};

typedef Client client_t;

// class ClientBuffer {
// private:
// 	ClientBuffer(const ClientBuffer& buf);
// 	ClientBuffer& operator=(const ClientBuffer& buf);

// public:
// 	SessionStorage storage; // add by space
// 	buffer_t	   rdsaved_;
// 	timespec	   timeout_;
// 	uintptr_t	   client_fd_;
// 	port_info_t*   port_info_;
// 	size_t		   skip_size_;
// 	size_t		   rdchecked_;
// 	int			   flag_;
// 	int			   state_;
// 	char		   rdbuf_[BUFFER_SIZE];

// 	// TEMP Implement
// 	ClientBuffer();
// 	~ClientBuffer();

// 	void write_filter_enable(event_list_t& change_list, struct kevent* cur_event);

// 	bool request_line_check(std::string& req_line);
// 	bool request_line_parser();

// 	bool header_field_parser();

// 	void disconnect_client(event_list_t& change_list);

// 	bool write_to_cgi(struct kevent* cur_event, std::vector<struct kevent>& change_list);
// 	bool write_response(event_list_t& change_list);
// 	bool write_for_upload(event_list_t& change_list, struct kevent* cur_event);

// 	bool cgi_header_parser();
// 	bool cgi_controller();
// 	// bool cgi_controller(int state, event_list_t& change_list);

// 	bool cgi_handler(struct kevent* cur_event, event_list_t& change_list);

// 	bool req_res_controller(event_list_t& change_list, struct kevent* cur_event);
// 	// bool skip_body(ssize_t cont_len);

// 	bool host_check(std::string& host);

// 	void read_to_client_buffer(event_list_t& change_list, struct kevent* cur_event);
// 	void read_to_cgi_buffer(event_list_t& change_list, struct kevent* cur_event);
// 	void read_to_res_buffer(event_list_t& change_list, struct kevent* cur_event);

// 	/* RESPONSE */
// 	void make_error_response(http_status error_code);
// 	void make_response_header();
// 	void make_redirect_response();
// 	void make_cgi_response_header();
// 	/* RESPONSE END*/
// };

// typedef ClientBuffer client_buf_t;

#endif
