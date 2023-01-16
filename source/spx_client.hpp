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
#include <sys/time.h>
#include <unistd.h>

#include <map>
#include <vector>

#include "spacex.hpp"
#include "spx_buffer.hpp"

#include "spx_cgi_chunked.hpp"
#include "spx_req_res_field.hpp"
#include "spx_response_generator.hpp"
#include "spx_session_storage.hpp"
#include "spx_syntax_checker.hpp"

#define BUFFER_SIZE 4 * 1024
#define IOV_VEC_SIZE 16

#define MAX_EVENT_LIST 1000

typedef SpxReadBuffer						rdbuf_t;
typedef SpxBuffer							buf_t;
typedef std::vector<struct kevent>			event_list_t;
typedef std::vector<port_info_t>			port_list_t;
typedef std::map<std::string, std::string>	cgi_header_t;
typedef std::map<std::string, std::string>	req_header_t;
typedef std::pair<std::string, std::string> header;
typedef std::vector<Client*>				close_vec_t;

class Client;
class ReqField;
class ResField;
class CgiField;
class ChunkedField;

class Client {
private:
	Client();
	Client(const Client& client);
	Client& operator=(const Client& client);

public:
	event_list_t*	   change_list;
	ReqField		   _req;
	ResField		   _res;
	CgiField		   _cgi;
	ChunkedField	   _chnkd;
	uintptr_t		   _client_fd;
	rdbuf_t*		   _rdbuf;
	buf_t			   _buf;
	int				   _state;
	size_t			   _skip_size;
	port_info_t*	   _port_info;
	struct sockaddr_in _sockaddr;
	session_storage_t* _storage;
	struct timeval	   _established_time;

	Client(event_list_t* chnage_list);
	~Client();

	void reset_();

	bool req_res_controller_(struct kevent* cur_event);
	bool state_req_body_();
	bool request_line_parser_();
	bool request_line_check_(std::string& req_line);
	bool header_field_parser_();
	void set_cookie_();

	void error_response_keep_alive_(http_status error_code);
	void do_cgi_(struct kevent* cur_event);

	void disconnect_client_();
	void read_to_client_buffer_(struct kevent* cur_event);
	void read_to_cgi_buffer_(struct kevent* cur_event);
	void read_to_res_buffer_(struct kevent* cur_event);

	bool write_for_upload_(struct kevent* cur_event);
	bool write_to_cgi_(struct kevent* cur_event);
	bool write_response_();
};

typedef Client client_t;

#endif
