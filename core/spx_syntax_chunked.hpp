#pragma once
#ifndef __SPX__SYNTAX__CHUNKED_HPP__
#define __SPX__SYNTAX__CHUNKED_HPP__

#include "spx_core_type.hpp"
#include "spx_core_util_box.hpp"
#include <map>
#include <sstream>
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
