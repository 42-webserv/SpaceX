#pragma once
#ifndef __SPX__SYNTAX_CHECKER__HPP__
#define __SPX__SYNTAX_CHECKER__HPP__

#include "spx_client.hpp"
#include "spx_core_type.hpp"
#include "spx_core_util_box.hpp"

status
spx_http_syntax_start_line(std::string const& line,
						   int&				  req_type);

status
spx_http_syntax_header_line(std::string const& line);

status
spx_chunked_syntax_start_line(std::string&						  line,
							  uint32_t&							  chunk_size,
							  std::map<std::string, std::string>& chunk_ext);

status
spx_chunked_syntax_data_line(std::string const&					 line,
							 uint32_t&							 chunk_size,
							 std::vector<char>&					 data_store,
							 std::map<std::string, std::string>& trailer_section);

#endif
