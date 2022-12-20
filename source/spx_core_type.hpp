#pragma once
#ifndef __SPX__CORE_TYPE_HPP__
#define __SPX__CORE_TYPE_HPP__

// #include <cstdint>
typedef signed char		   int8_t;
typedef unsigned char	   uint8_t;
typedef short			   int16_t;
typedef unsigned short	   uint16_t;
typedef int				   int32_t;
typedef unsigned int	   uint32_t;
typedef long long		   int64_t;
typedef unsigned long long uint64_t;

#define LF "\n"
#define CR "\r"
#define CRLF "\r\n"

typedef enum {
	spx_error = -1,
	spx_ok,
	spx_CRLF,
	spx_need_more
} status;

inline bool
syntax_(const uint32_t table[8], uint8_t c) {
	return (table[(c >> 5)] & (1U << (c & 0x1f)));
}

const uint32_t usual_[] = {
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	/*				?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
	0x7ffff7d6, /*	0111 1111 1111 1111  1111 1111 1101 0110 */
	/*				_^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
	0xffffffff, /*	1111 1111 1111 1111  1111 1111 1111 1111 */
	/*				 ~}| {zyx wvut srqp  onml kjih gfed cba` */
	0x7fffffff, /*	0111 1111 1111 1111  1111 1111 1111 1111 */
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
};

const uint32_t start_line_[] = {
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	/*				?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
	0x00002000, /*	0000 0000 0000 0000  0010 0000 0000 0000 */
	/*				_^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
	0x87fffffe, /*	1000 0111 1111 1111  1111 1111 1111 1110 */
	/*				 ~}| {zyx wvut srqp  onml kjih gfed cba` */
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
};

const uint32_t digit_alpha_[] = {
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	/*				?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
	0x03ff0000, /*	0000 0011 1111 1111  0000 0000 0000 0000 */
	/*				_^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
	0x07fffffe, /*	0000 0111 1111 1111  1111 1111 1111 1110 */
	/*				 ~}| {zyx wvut srqp  onml kjih gfed cba` */
	0x07fffffe, /*	0000 0111 1111 1111  1111 1111 1111 1110 */
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
};

const uint32_t alpha_upper_case_[] = {
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	/*				?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	/*				_^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
	0x07fffffe, /*	0000 0111 0000 0000  0000 0000 0000 1110 */
	/*				 ~}| {zyx wvut srqp  onml kjih gfed cba` */
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
};

const uint32_t digit_[] = {
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	/*				?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
	0x03ff0000, /*	0000 0011 1111 1111  0000 0000 0000 0000 */
	/*				_^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	/*				 ~}| {zyx wvut srqp  onml kjih gfed cba` */
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
};

const uint32_t name_token_[] = {
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	/*				?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
	0x03ff6cfa, /*	0000 0011 1111 1111  0110 1100 1111 1010 */
	/*				_^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
	0xc7fffffe, /*	1100 0111 1111 1111  1111 1111 1111 1110 */
	/*				 ~}| {zyx wvut srqp  onml kjih gfed cba` */
	0x57ffffff, /*	0101 0111 1111 1111  1111 1111 1111 1111 */
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
};

const uint32_t field_value_[] = {
	0x00000200, /*	0000 0000 0000 0000  0000 0010 0000 0000 */
	/*				?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
	0xffffffff, /*	1111 1111 1111 1111  1111 1111 1111 1111 */
	/*				_^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
	0xffffffff, /*	1111 1111 1111 1111  1111 1111 1111 1111 */
	/*				 ~}| {zyx wvut srqp  onml kjih gfed cba` */
	0x7fffffff, /*	0111 1111 1111 1111  1111 1111 1111 1111 */
	0xffffffff, /*	1111 1111 1111 1111  1111 1111 1111 1111 */ /* obs-text */
	0xffffffff, /*	1111 1111 1111 1111  1111 1111 1111 1111 */
	0xffffffff, /*	1111 1111 1111 1111  1111 1111 1111 1111 */
	0xffffffff, /*	1111 1111 1111 1111  1111 1111 1111 1111 */
};

const uint32_t vchar_[] = {
	0x00000200, /*	0000 0000 0000 0000  0000 0010 0000 0000 */
	/*				?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
	0xfffffffe, /*	1111 1111 1111 1111  1111 1111 1111 1110 */
	/*				_^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
	0xffffffff, /*	1111 1111 1111 1111  1111 1111 1111 1111 */
	/*				 ~}| {zyx wvut srqp  onml kjih gfed cba` */
	0x7fffffff, /*	0111 1111 1111 1111  1111 1111 1111 1111 */
	0xffffffff, /*	1111 1111 1111 1111  1111 1111 1111 1111 */ /* obs-text */
	0xffffffff, /*	1111 1111 1111 1111  1111 1111 1111 1111 */
	0xffffffff, /*	1111 1111 1111 1111  1111 1111 1111 1111 */
	0xffffffff, /*	1111 1111 1111 1111  1111 1111 1111 1111 */
};

const uint32_t config_vchar_except_delimiter_[] = {
	0x00000200, /*	0000 0000 0000 0000  0000 0010 0000 0000 */
	/*				?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
	0xf7fffffe, /*	1111 0111 1111 1111  1111 1111 1111 1110 */
	/*				_^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
	0xffffffff, /*	1111 1111 1111 1111  1111 1111 1111 1111 */
	/*				 ~}| {zyx wvut srqp  onml kjih gfed cba` */
	0x57ffffff, /*	0101 0111 1111 1111  1111 1111 1111 1111 */
	0xffffffff, /*	1111 1111 1111 1111  1111 1111 1111 1111 */ /* obs-text */
	0xffffffff, /*	1111 1111 1111 1111  1111 1111 1111 1111 */
	0xffffffff, /*	1111 1111 1111 1111  1111 1111 1111 1111 */
	0xffffffff, /*	1111 1111 1111 1111  1111 1111 1111 1111 */
};

const uint32_t config_listen_[] = {
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	/*				?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
	0x07ff4000, /*	0000 0111 1111 1111  0100 0000 0000 0000 */
	/*				_^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
	0x07fffffe, /*	0000 0111 1111 1111  1111 1111 1111 1110 */
	/*				 ~}| {zyx wvut srqp  onml kjih gfed cba` */
	0x07fffffe, /*	0000 0111 1111 1111  1111 1111 1111 1110 */
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
};

const uint32_t config_uri_host_[] = {
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	/*				?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
	0x07ff6000, /*	0000 0111 1111 1111  0110 0000 0000 0000 */
	/*				_^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
	0x87fffffe, /*	1000 0111 1111 1111  1111 1111 1111 1110 */
	/*				 ~}| {zyx wvut srqp  onml kjih gfed cba` */
	0x07fffffe, /*	0000 0111 1111 1111  1111 1111 1111 1110 */
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
};

const uint32_t config_elem_syntax_[] = {
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	/*				?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	/*				_^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
	0x80000000, /*	1000 0000 0000 0000  0000 0000 0000 0000 */
	/*				 ~}| {zyx wvut srqp  onml kjih gfed cba` */
	0x077df3fe, /*	0000 0111 0111 1101  1111 0011 1111 1110 */
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
};

const uint32_t isspace_[] = {
	0x00003e00, /*	0000 0000 0000 0000  0011 1110 0000 0000 */
	/*				?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
	0x00000001, /*	0000 0000 0000 0000  0000 0000 0000 0001 */
	/*				_^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	/*				 ~}| {zyx wvut srqp  onml kjih gfed cba` */
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
};

static char http_error_302_page[] = "<html>" CRLF
									"<head><title>302 Found</title></head>" CRLF
									"<body>" CRLF
									"<center><h1>302 Found</h1></center>" CRLF;

static char http_error_303_page[] = "<html>" CRLF
									"<head><title>303 See Other</title></head>" CRLF
									"<body>" CRLF
									"<center><h1>303 See Other</h1></center>" CRLF;

static char http_error_307_page[] = "<html>" CRLF
									"<head><title>307 Temporary Redirect</title></head>" CRLF
									"<body>" CRLF
									"<center><h1>307 Temporary Redirect</h1></center>" CRLF;

static char http_error_308_page[] = "<html>" CRLF
									"<head><title>308 Permanent Redirect</title></head>" CRLF
									"<body>" CRLF
									"<center><h1>308 Permanent Redirect</h1></center>" CRLF;

static char http_error_400_page[] = "<html>" CRLF
									"<head><title>400 Bad Request</title></head>" CRLF
									"<body>" CRLF
									"<center><h1>400 Bad Request</h1></center>" CRLF;

static char http_error_401_page[] = "<html>" CRLF
									"<head><title>401 Authorization Required</title></head>" CRLF
									"<body>" CRLF
									"<center><h1>401 Authorization Required</h1></center>" CRLF;

static char http_error_402_page[] = "<html>" CRLF
									"<head><title>402 Payment Required</title></head>" CRLF
									"<body>" CRLF
									"<center><h1>402 Payment Required</h1></center>" CRLF;

static char http_error_403_page[] = "<html>" CRLF
									"<head><title>403 Forbidden</title></head>" CRLF
									"<body>" CRLF
									"<center><h1>403 Forbidden</h1></center>" CRLF;

static char http_error_404_page[] = "<html>" CRLF
									"<head><title>404 Not Found</title></head>" CRLF
									"<body>" CRLF
									"<center><h1>404 Not Found</h1></center>" CRLF;

static char http_error_405_page[] = "<html>" CRLF
									"<head><title>405 Not Allowed</title></head>" CRLF
									"<body>" CRLF
									"<center><h1>405 Not Allowed</h1></center>" CRLF;

static char http_error_406_page[] = "<html>" CRLF
									"<head><title>406 Not Acceptable</title></head>" CRLF
									"<body>" CRLF
									"<center><h1>406 Not Acceptable</h1></center>" CRLF;

static char http_error_408_page[] = "<html>" CRLF
									"<head><title>408 Request Time-out</title></head>" CRLF
									"<body>" CRLF
									"<center><h1>408 Request Time-out</h1></center>" CRLF;

static char http_error_409_page[] = "<html>" CRLF
									"<head><title>409 Conflict</title></head>" CRLF
									"<body>" CRLF
									"<center><h1>409 Conflict</h1></center>" CRLF;

static char http_error_410_page[] = "<html>" CRLF
									"<head><title>410 Gone</title></head>" CRLF
									"<body>" CRLF
									"<center><h1>410 Gone</h1></center>" CRLF;

static char http_error_411_page[] = "<html>" CRLF
									"<head><title>411 Length Required</title></head>" CRLF
									"<body>" CRLF
									"<center><h1>411 Length Required</h1></center>" CRLF;

static char http_error_412_page[] = "<html>" CRLF
									"<head><title>412 Precondition Failed</title></head>" CRLF
									"<body>" CRLF
									"<center><h1>412 Precondition Failed</h1></center>" CRLF;

static char http_error_413_page[] = "<html>" CRLF
									"<head><title>413 Request Entity Too Large</title></head>" CRLF
									"<body>" CRLF
									"<center><h1>413 Request Entity Too Large</h1></center>" CRLF;

static char http_error_414_page[] = "<html>" CRLF
									"<head><title>414 Request-URI Too Large</title></head>" CRLF
									"<body>" CRLF
									"<center><h1>414 Request-URI Too Large</h1></center>" CRLF;

static char http_error_415_page[] = "<html>" CRLF
									"<head><title>415 Unsupported Media Type</title></head>" CRLF
									"<body>" CRLF
									"<center><h1>415 Unsupported Media Type</h1></center>" CRLF;

static char http_error_416_page[] = "<html>" CRLF
									"<head><title>416 Requested Range Not Satisfiable</title></head>" CRLF
									"<body>" CRLF
									"<center><h1>416 Requested Range Not Satisfiable</h1></center>" CRLF;

static char http_error_421_page[] = "<html>" CRLF
									"<head><title>421 Misdirected Request</title></head>" CRLF
									"<body>" CRLF
									"<center><h1>421 Misdirected Request</h1></center>" CRLF;

static char http_error_429_page[] = "<html>" CRLF
									"<head><title>429 Too Many Requests</title></head>" CRLF
									"<body>" CRLF
									"<center><h1>429 Too Many Requests</h1></center>" CRLF;

static char http_error_494_page[] = "<html>" CRLF
									"<head><title>400 Request Header Or Cookie Too Large</title></head>" CRLF
									"<body>" CRLF
									"<center><h1>400 Bad Request</h1></center>" CRLF
									"<center>Request Header Or Cookie Too Large</center>" CRLF;

static char http_error_495_page[] = "<html>" CRLF
									"<head><title>400 The SSL certificate error</title></head>" CRLF
									"<body>" CRLF
									"<center><h1>400 Bad Request</h1></center>" CRLF
									"<center>The SSL certificate error</center>" CRLF;

static char http_error_496_page[] = "<html>" CRLF
									"<head><title>400 No required SSL certificate was sent</title></head>" CRLF
									"<body>" CRLF
									"<center><h1>400 Bad Request</h1></center>" CRLF
									"<center>No required SSL certificate was sent</center>" CRLF;

static char http_error_497_page[] = "<html>" CRLF
									"<head><title>400 The plain HTTP request was sent to HTTPS port</title></head>" CRLF
									"<body>" CRLF
									"<center><h1>400 Bad Request</h1></center>" CRLF
									"<center>The plain HTTP request was sent to HTTPS port</center>" CRLF;

static char http_error_500_page[] = "<html>" CRLF
									"<head><title>500 Internal Server Error</title></head>" CRLF
									"<body>" CRLF
									"<center><h1>500 Internal Server Error</h1></center>" CRLF;

static char http_error_501_page[] = "<html>" CRLF
									"<head><title>501 Not Implemented</title></head>" CRLF
									"<body>" CRLF
									"<center><h1>501 Not Implemented</h1></center>" CRLF;

static char http_error_502_page[] = "<html>" CRLF
									"<head><title>502 Bad Gateway</title></head>" CRLF
									"<body>" CRLF
									"<center><h1>502 Bad Gateway</h1></center>" CRLF;

static char http_error_503_page[] = "<html>" CRLF
									"<head><title>503 Service Temporarily Unavailable</title></head>" CRLF
									"<body>" CRLF
									"<center><h1>503 Service Temporarily Unavailable</h1></center>" CRLF;

static char http_error_504_page[] = "<html>" CRLF
									"<head><title>504 Gateway Time-out</title></head>" CRLF
									"<body>" CRLF
									"<center><h1>504 Gateway Time-out</h1></center>" CRLF;

static char http_error_505_page[] = "<html>" CRLF
									"<head><title>505 HTTP Version Not Supported</title></head>" CRLF
									"<body>" CRLF
									"<center><h1>505 HTTP Version Not Supported</h1></center>" CRLF;

static char http_error_507_page[] = "<html>" CRLF
									"<head><title>507 Insufficient Storage</title></head>" CRLF
									"<body>" CRLF
									"<center><h1>507 Insufficient Storage</h1></center>" CRLF;

#endif
