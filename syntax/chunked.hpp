#pragma once
#ifndef __SYNTAX_CHUNKED_HPP__
#define __SYNTAX_CHUNKED_HPP__

#include "spacex_type.hpp"
#include <string>

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
