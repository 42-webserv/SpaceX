#pragma once

enum e_client_buffer_flag {
	REQ_GET		  = 1,
	REQ_HEAD	  = 2,
	REQ_POST	  = 4,
	REQ_PUT		  = 8,
	REQ_DELETE	  = 16,
	REQ_UNDEFINED = 32,
	SOCK_WRITE	  = 64,
	READ_BODY	  = 256,
	RES_BODY	  = 1024,
	E_BAD_REQ	  = 1 << 31,
	NONE		  = 0
};

enum e_read_status {
	REQ_LINE_PARSING = 0,
	REQ_HEADER_PARSING,
	REQ_BODY,
	RES_BODY,
};

// gzip & deflate are not implemented.
enum { TE_CHUNKED = 0,
	   TE_GZIP,
	   TE_DEFLATE };
