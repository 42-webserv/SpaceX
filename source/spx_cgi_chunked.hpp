#pragma once
#ifndef __SPX__CGI__CHUNKED__HPP
#define __SPX__CGI__CHUNKED__HPP

#include "spx_req_res_field.hpp"

enum e_cgi_state {
	CGI_HEADER,
	CGI_BODY_CHUNKED,
	CGI_HOLD
};

class ReqField;
class ResField;
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
		// _from_cgi.clear_();
		// _to_cgi.clear_();
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
	buf_t	 _chnkd_body;
	uint32_t _chnkd_size;
	bool	 _first_chnkd;
	bool	 _last_chnkd;

	ChunkedField()
		: _chnkd_body()
		, _chnkd_size(0)
		, _first_chnkd(true)
		, _last_chnkd(false) {
	}
	~ChunkedField() { }

	void
	clear_() {
		// _chnkd_body.clear_();
		_chnkd_size	 = 0;
		_first_chnkd = true;
		_last_chnkd	 = false;
	}

	bool chunked_body_(Client& cl);
	bool chunked_body_can_parse_chnkd_(Client& cl);
	bool chunked_body_can_parse_chnkd_skip_(Client& cl);
	bool skip_chunked_body_(Client& cl);
};

#endif
