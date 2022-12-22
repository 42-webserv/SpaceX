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

#define SERV_PORT 1234
#define SERV_SOCK_BACKLOG 10
#define EVENT_CHANGE_BUF 10
#define BUFFER_SIZE 8 * 1024
#define WRITE_BUFFER_MAX 40 * 1024
#define MAX_EVENT_LOOP 20
// #define BUFFER_MAX 80 * 1024

enum e_client_buffer_flag {
	REQ_GET		  = 1,
	REQ_HEAD	  = 2,
	REQ_POST	  = 4,
	REQ_PUT		  = 8,
	REQ_DELETE	  = 16,
	REQ_UNDEFINED = 32,
	SOCK_WRITE	  = 128,
	READ_BODY	  = 256,
	RES_BODY	  = 1024,
	RDBUF_CHECKED = 1 << 24,
	READ_READY	  = 1 << 30,
	E_BAD_REQ	  = 1 << 31
};

enum e_read_status {
	REQ_LINE_PARSING = 0,
	REQ_HEADER_PARSING,
	REQ_BODY,
	RES_BODY,
};

enum e_req_flag { REQ_FILE_OPEN = 1,
				  CGI_READ		= 2 };

enum e_res_flag { RES_FILE_OPEN = 1,
				  WRITE_READY	= 2,
				  RES_CGI		= 4 };

// gzip & deflate are not implemented.
enum e_transfer_encoding { TE_CHUNKED = 0,
						   TE_GZIP,
						   TE_DEFLATE };

typedef std::vector<char>		   buffer_t;
typedef std::vector<struct kevent> event_list_t;

class ReqField {
public:
	buffer_t						   body_buffer_;
	std::map<std::string, std::string> field_;
	std::string						   req_target_;
	std::string						   http_ver_;
	std::string						   file_path_;
	const server_info_t*			   serv_info_;
	const uri_location_t*			   uri_loc_;
	size_t							   body_recieved_;
	size_t							   body_limit_;
	size_t							   content_length_;
	int								   body_flag_;
	int								   req_type_;
	int								   transfer_encoding_;

	ReqField()
		: body_buffer_()
		, field_()
		, req_target_()
		, http_ver_()
		, file_path_()
		, uri_loc_()
		, body_recieved_(0)
		, body_limit_(-1)
		, content_length_(0)
		, body_flag_(0)
		, req_type_(0)
		, transfer_encoding_(0) {
	}
	~ReqField() {
	}
};

class ResField {
public:
	// res_header
	buffer_t	res_buffer_;
	std::string file_path_;
	size_t		buf_size_;
	int			body_fd_;
	int			header_ready_;
	int			sent_pos_;
	int			flag_;
	int			transfer_encoding_;

	ResField()
		: res_buffer_()
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
	port_info_t*									 serv_info_;
	int												 rdchecked_;
	int												 flag_;
	int												 state_;
	char											 rdbuf_[BUFFER_SIZE];

	ClientBuffer();
	~ClientBuffer();

	void write_filter_enable(event_list_t& change_list, struct kevent* cur_event);

	bool request_line_check(std::string& req_line);
	bool request_line_parser();

	bool header_field_parser();

	void disconnect_client(event_list_t& change_list);

	bool write_res_body(uintptr_t fd, event_list_t& change_list);
	bool write_res_header(uintptr_t fd, event_list_t& change_list);

	bool req_res_controller(event_list_t& change_list, struct kevent* cur_event);
	bool skip_body(ssize_t cont_len);

	void read_to_client_buffer(event_list_t& change_list, struct kevent* cur_event);
	void read_to_res_buffer(event_list_t& change_list, struct kevent* cur_event);
};

typedef ClientBuffer client_buf_t;