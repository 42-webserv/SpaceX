#pragma once
#ifndef __SPX__SYNTAX_REQUEST__HPP__
#define __SPX__SYNTAX_REQUEST__HPP__

#include "spx_core_type.hpp"
#include "spx_core_util_box.hpp"
#include <cstddef>
#include <iostream>
#include <string>

typedef class client_buf t_client_buf;

status
spx_http_syntax_start_line(std::string const& line);
// spx_http_syntax_start_line(std::string const& line, t_client_buf& buf);

status
spx_http_syntax_header_line(std::string const& line);

#endif
