#pragma once
#ifndef __SYNTAX_HPP__
#define __SYNTAX_HPP__


#include <cstdint>
#include <cstddef>
#include <cstring>
#include <iostream>

typedef enum {
    spx_error = -1,
    spx_ok
} status;

status	spx_http_syntax_start_line_request(char *buf);

#endif
