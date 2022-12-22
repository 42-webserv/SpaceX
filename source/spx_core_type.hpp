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

const uint32_t ows_[] = {
	0x00000200, /*	0000 0000 0000 0000  0000 0010 0000 0000 */
	/*				?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
	0x00000001, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	/*				_^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	/*				 ~}| {zyx wvut srqp  onml kjih gfed cba` */
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
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

const uint32_t hexdigit_[] = {
	0x00000000, /*	0000 0000 0000 0000  0000 0000 0000 0000 */
	/*				?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
	0x03ff0000, /*	0000 0011 1111 1111  0000 0000 0000 0000 */
	/*				_^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
	0x0000007e, /*	0000 0000 0000 0000  0000 0000 0111 1110 */
	/*				 ~}| {zyx wvut srqp  onml kjih gfed cba` */
	0x0000007e, /*	0000 0000 0000 0000  0000 0000 0111 1110 */
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

#endif
