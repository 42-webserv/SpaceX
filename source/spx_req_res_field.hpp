#pragma once
#ifndef __SPX__REQ__RES__FIELD__HPP
#define __SPX__REQ__RES__FIELD__HPP

#include <map>
#include <vector>

#include "spacex.hpp"
#include "spx_buffer.hpp"
#include "spx_response_generator.hpp"

enum e_request_method {
	REQ_GET		  = 1 << 1,
	REQ_POST	  = 1 << 2,
	REQ_PUT		  = 1 << 3,
	REQ_DELETE	  = 1 << 4,
	REQ_HEAD	  = 1 << 5,
	REQ_UNDEFINED = 1 << 6
};

enum e_read_status {
	REQ_CLEAR = 0,
	REQ_LINE_PARSING,
	REQ_HEADER_PARSING,
	REQ_BODY,
	REQ_BODY_CHUNKED,
	REQ_SKIP_BODY,
	REQ_SKIP_BODY_CHUNKED,
	REQ_CGI_BODY,
	REQ_CGI_BODY_CHUNKED,
	REQ_HOLD,
	E_BAD_REQ
};

typedef SpxBuffer							buf_t;
typedef std::vector<struct kevent>			event_list_t;
typedef std::vector<port_info_t>			port_list_t;
typedef std::map<std::string, std::string>	cgi_header_t;
typedef std::map<std::string, std::string>	req_header_t;
typedef std::pair<std::string, std::string> header;

class Client;

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
	std::string			  session_id;

	ReqField();
	~ReqField();

	void clear_();
	bool set_uri_info_and_cookie_(Client& cl);
	bool host_check_(std::string& host);

	bool res_for_get_head_req_(Client& cl);
	bool res_for_post_put_req_(Client& cl);
	void res_for_delete_req_(Client& cl);
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
	bool		_write_finished;

	/* RESPONSE*/
	std::vector<header> _headers;
	int					_version_minor;
	int					_version_major;
	unsigned int		_status_code;
	std::string			_status;

	ResField();
	~ResField();

	void clear_();

	void make_error_response_(Client& cl, http_status error_code);
	void make_response_header_(Client& cl);
	void make_cgi_response_header_(Client& cl);
	void make_redirect_response_(Client& cl);

	int			file_open_(const char* dir) const;
	off_t		set_content_length_(int fd);
	void		set_content_type_(std::string uri);
	void		set_date_();
	std::string make_to_string_() const;
	void		write_to_response_buffer_(const std::string& content);

	// 	/* RESPONSE END*/
};

typedef ResField res_field_t;

#endif
