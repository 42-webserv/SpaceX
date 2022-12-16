#pragma once
#ifndef __SYNTAX_REQUEST__HPP__
#define __SYNTAX_REQUEST__HPP__

#include "core_type.hpp"
#include <cstddef>
#include <string>

typedef class client_buf t_client_buf;

status
spx_http_syntax_start_line(std::string const& line);
// spx_http_syntax_start_line(std::string const& line, t_client_buf& buf);

status
spx_http_syntax_header_line(std::string const& line);

#endif
