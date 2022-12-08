#pragma once
#ifndef __SYNTAX_HPP__
#define __SYNTAX_HPP__


#include <cstdint>
#include <cstddef>
#include <cstring>
#include <string>

using namespace std;

typedef enum {
    spx_error = -1,
    spx_ok,
    spx_crlf
}   status;

status
spx_http_syntax_start_line_request(string const & line);

status
spx_http_syntax_header_line(string const & line);


#endif
