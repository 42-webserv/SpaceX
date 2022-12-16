#pragma once
#ifndef __SYNTAX__CHUNKED_HPP__
#define __SYNTAX__CHUNKED_HPP__

#include "core_type.hpp"
#include <map>
#include <string>
#include <vector>

status
spx_chunked_syntax_start_line(std::string const&				  line,
							  uint16_t&							  chunk_size,
							  std::map<std::string, std::string>& chunk_ext);

status
spx_chunked_syntax_data_line(std::string const&					 line,
							 uint16_t&							 chunk_size,
							 std::vector<char>&					 data_store,
							 std::map<std::string, std::string>& trailer_section);

#endif
