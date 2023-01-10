#pragma once
#ifndef __SPX__CLIENT__HPP
#define __SPX__CLIENT__HPP

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
#include <unistd.h>

#include <map>
#include <queue>
#include <vector>

#include "spacex.hpp"
#include "spx_buffer.hpp"
#include "spx_response_generator.hpp"
#include "spx_session_storage.hpp"
#include "spx_syntax_checker.hpp"

#define BUFFER_SIZE 8 * 1024
#define IOV_VEC_SIZE 8

#define MAX_EVENT_LIST 200

class SessionStorage;

typedef SpxReadBuffer rdbuf_t;
typedef SpxBuffer	  buf_t;

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
	REQ_CLEAR = 0,
	REQ_LINE_PARSING,
	REQ_HEADER_PARSING,
	REQ_BODY,
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

// enum e_req_flag { REQ_FILE_OPEN = 1 << 0,
// 				  READ_BODY_END = 1 << 1
// };

// enum e_res_flag { WRITE_READY = 1 };

// gzip & deflate are not implemented. for extension.
enum e_transfer_encoding { TE_CHUNKED = 1 << 0,
						   TE_GZIP	  = 1 << 1,
						   TE_DEFLATE = 1 << 2 };

typedef std::vector<char>					buffer_t;
typedef std::vector<struct kevent>			event_list_t;
typedef std::vector<port_info_t>			port_list_t;
typedef std::map<std::string, std::string>	cgi_header_t;
typedef std::map<std::string, std::string>	req_header_t;
typedef std::pair<std::string, std::string> header;

class ReqField;
class ResField;
class CgiField;
class ChunkedField;
class Client;

class CgiField {
public:
	cgi_header_t _cgi_header;
	buf_t		 _from_cgi;
	buf_t		 _to_cgi;
	size_t		 _cgi_size;
	size_t		 _cgi_read;
	int			 _write_to_cgi_fd;
	int			 _cgi_state;
	int			 _is_chnkd;

	CgiField()
		: _cgi_header()
		, _from_cgi()
		, _to_cgi()
		, _cgi_size(0)
		, _cgi_read(0)
		, _write_to_cgi_fd(0)
		, _cgi_state(CGI_HEADER)
		, _is_chnkd(false) {};
	~CgiField() {};

	void
	clear_() {
		_cgi_header.clear();
		_from_cgi.clear_();
		_to_cgi.clear_();
		_cgi_size		 = 0;
		_cgi_read		 = 0;
		_write_to_cgi_fd = 0;
		_cgi_state		 = CGI_HEADER;
		_is_chnkd		 = false;
	}

	bool cgi_handler_(ReqField& req, event_list_t& change_list, struct kevent* cur_event);
	bool cgi_header_parser_();
	bool cgi_controller_(Client& cl);
};

class ChunkedField {
public:
	buf_t _chnkd_body;
	bool  _first_chnkd;

	void
	clear_() {
		_first_chnkd = 1;
	}

	bool chunked_body_(Client& cl);
	bool chunked_body_can_parse_chnkd_(Client& cl, size_t size);
	bool chunked_body_can_parse_chnkd_skip_(Client& cl, size_t size);
	bool skip_chunked_body_(Client& cl);
};

class ReqField {
public:
	buf_t				  _body_buf;
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
	uri_resolved_t		  _uri_resolv;
	int					  _req_mthd;
	int					  _is_chnkd;
	int					  _flag;
	std::string			  session_id; // session

	ReqField()
		: _body_buf()
		, _body_size(0)
		, _body_read(0)
		, _body_limit(-1)
		, _cnt_len(-1)
		, _body_fd(-1)
		, _header()
		, _uri()
		, _http_ver()
		, _upld_fn()
		, _uri_loc()
		, _uri_resolv()
		, _flag(0)
		, _req_mthd(0)
		, _is_chnkd(0) { }
	~ReqField() { }

	void
	clear_() {
		_body_buf.clear_();
		_header.clear();
		_uri.clear();
		_http_ver.clear();
		_upld_fn.clear();
		// _uri_resolv.clear();
		_body_size	= 0;
		_body_read	= 0;
		_body_limit = -1;
		_cnt_len	= -1;
		_body_fd	= -1;
		_uri_loc	= NULL;
		_flag		= 0;
		_req_mthd	= 0;
		_is_chnkd	= false;
	}
};

class ResField {
public:
	std::string _res_header;
	std::string _dwnl_fn;
	buf_t		_res_buf;
	size_t		_body_read;
	size_t		_body_write;
	size_t		_body_size;
	int			_body_fd;
	int			_is_chnkd;
	int			_header_sent;
	bool		_write_ready;

	/* RESPONSE*/
	std::vector<header> _headers;
	int					_version_minor;
	int					_version_major;
	unsigned int		_status_code;
	std::string			_status;

	ResField()
		: _res_header()
		, _dwnl_fn()
		, _res_buf()
		, _body_fd(-1)
		, _body_read(0)
		, _body_write(0)
		, _body_size(0)
		, _is_chnkd(0)
		, _header_sent(0)
		, _write_ready(0)
		, _headers()
		, _version_minor(1)
		, _version_major(1)
		, _status_code(200)
		, _status("OK") {
	}

	~ResField() {
	}

	void
	clear_() {
		_res_header.clear();
		_dwnl_fn.clear();
		_headers.clear();
		_res_buf.clear_();
		_body_fd	   = -1;
		_body_read	   = 0;
		_body_write	   = 0;
		_body_size	   = 0;
		_is_chnkd	   = 0;
		_header_sent   = 0;
		_write_ready   = 0;
		_version_minor = 1;
		_version_major = 1;
		_status_code   = 200;
		_status		   = "OK";
	};

	int			file_open_(const char* dir) const;
	off_t		setContentLength_(int fd);
	void		setContentType_(std::string uri);
	void		setDate_();
	std::string handle_static_error_page_();
	std::string make_to_string_() const;
	void		write_to_response_buffer_(const std::string& content);

	/* session & SESSION */
	void setSessionHeader(std::string session_id);
	/* RESPONSE END*/

	// 	/* RESPONSE */
	void make_error_response_(Client& cl, http_status error_code);
	void make_response_header_(Client& cl);
	void make_redirect_response_(Client& cl);
	void make_cgi_response_header_(Client& cl);
	// 	/* RESPONSE END*/
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
	rdbuf_t*			   _rdbuf;
	buf_t				   _buf;
	int					   _state;
	int					   _skip_size;
	port_info_t*		   _port_info;
	const struct sockaddr* _sockaddr;
	session_storage_t*	   _storage; // add by space

	Client(event_list_t* chnage_list);
	~Client();

	void reset_();

	bool req_res_controller_(struct kevent* cur_event);
	bool request_line_parser_();
	bool request_line_check_(std::string& req_line);
	bool header_field_parser_();
	bool host_check_(std::string& host);
	void set_cookie_();
	void error_response_keep_alive_(http_status error_code);
	void do_cgi_(struct kevent* cur_event);
	bool res_for_get_head_req_();
	bool res_for_post_put_req_();

	void disconnect_client_();
	bool write_to_cgi_(struct kevent* cur_event);
	bool write_response_();
	bool write_for_upload_(struct kevent* cur_event);

	void read_to_client_buffer_(struct kevent* cur_event);
	void read_to_cgi_buffer_(struct kevent* cur_event);
	void read_to_res_buffer_(struct kevent* cur_event);
};

typedef Client client_t;

#endif
