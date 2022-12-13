#pragma once
#ifndef __SPACEX_TYPE_HPP__
#define __SPACEX_TYPE_HPP__

// #include <cstdint>
typedef signed char		   int8_t;
typedef unsigned char	   uint8_t;
typedef short			   int16_t;
typedef unsigned short	   uint16_t;
typedef int				   int32_t;
typedef unsigned int	   uint32_t;
typedef long long		   int64_t;
typedef unsigned long long uint64_t;

typedef enum {
	spx_error = -1,
	spx_ok,
	spx_CRLF,
	spx_need_more
} status;

#endif