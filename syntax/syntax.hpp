#pragma once
#ifndef __SYNTAX_HPP__
#define __SYNTAX_HPP__

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

typedef enum {
	spx_error = -1,
	spx_ok,
	spx_CRLF,
	spx_need_more
} status;

status
spx_http_syntax_start_line(std::string const& line);

status
spx_http_syntax_header_line(std::string const& line);

status
spx_chunked_syntax_start_line(std::string const& line,
							  uint16_t&			 chunk_size,
							  std::string&		 chunk_ext,
							  uint8_t&			 ext_count);

status
spx_chunked_syntax_data_line(std::string const& line,
							 uint16_t&			chunk_size,
							 std::string&		data_store,
							 std::string&		trailer_section,
							 uint8_t&			trailer_count);

#endif
