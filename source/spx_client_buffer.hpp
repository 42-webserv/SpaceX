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
	RES_BODY
};

enum e_req_flag { REQ_FILE_OPEN = 1,
				  CGI_READ		= 2 };

enum e_res_flag { RES_FILE_OPEN = 1,
				  WRITE_READY	= 2 };

// gzip & deflate are not implemented.
enum e_transfer_encoding { TE_CHUNKED = 0,
						   TE_GZIP,
						   TE_DEFLATE };

typedef std::vector<char> buffer_t;

typedef struct ReqField {
	buffer_t						   body_buffer_;
	std::map<std::string, std::string> field_;
	std::string						   req_target_;
	std::string						   http_ver_;
	std::string						   file_path_;
	uri_location_t*					   uri_loc_;
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
} t_req_field;

typedef struct ResField {
	buffer_t	body_buffer_;
	std::string res_header_;
	std::string file_path_;
	size_t		content_length_;
	int			header_ready_;
	int			sent_pos_;
	int			body_flag_;
	int			transfer_encoding_;

	ResField()
		: body_buffer_()
		, res_header_()
		, file_path_()
		, content_length_(0)
		, header_ready_(0)
		, sent_pos_(0)
		, body_flag_(0)
		, transfer_encoding_(0) {
	}
	~ResField() {
	}
} t_res_field;

class ClientBuffer {

private:
	ClientBuffer(const ClientBuffer& buf);
	ClientBuffer& operator=(const ClientBuffer& buf);

public:
	std::queue<std::pair<t_req_field, t_res_field> > req_res_queue_;
	buffer_t										 rdsaved_;
	timespec										 timeout_;
	uintptr_t										 client_fd;
	port_info_t*									 serv_info;
	int												 rdchecked_;
	int												 flag_;
	int												 state_;
	char											 rdbuf_[BUFFER_SIZE];

	ClientBuffer();
	~ClientBuffer();

	bool request_line_check(std::string& req_line);
	bool request_line_parser();
	bool header_valid_check(std::string& key_val, size_t col_pos);
	bool header_field_parser();
	void disconnect_client(std::vector<struct kevent>& change_list);
	bool write_res_body(uintptr_t fd, std::vector<struct kevent>& change_list);
	bool write_res_header(uintptr_t fd, std::vector<struct kevent>& change_list);
	bool req_res_controller(std::vector<struct kevent>& change_list, struct kevent* cur_event);
	bool skip_body(ssize_t cont_len);
	void client_buffer_read(struct kevent* cur_event, std::vector<struct kevent>& change_list);
};

typedef ClientBuffer client_buf_t;
