#include <cstdint>
#include <cstddef>

static uint32_t  start_line[] = {
    0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */

                /* ?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
    0x7fff37d6, /* 0111 1111 1111 1111  0011 0111 1101 0110 */
                /* _^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
    0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
                /*  ~}| {zyx wvut srqp  onml kjih gfed cba` */
    0x7fffffff, /* 0111 1111 1111 1111  1111 1111 1111 1111 */

    0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
    0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
    0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
    0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
};

static uint32_t  start_line_invalid[] = {
    0x00002400, /* 0000 0000 0000 0000  0010 0100 0000 0000 */

                /* ?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
    0x00000000, /* 0111 1111 1111 1111  0011 0111 1101 0110 */
                /* _^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
    0x00000000, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
                /*  ~}| {zyx wvut srqp  onml kjih gfed cba` */
    0x00000000, /* 0111 1111 1111 1111  1111 1111 1111 1111 */

    0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
    0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
    0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
    0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
};

int
syntax_(const uint32_t table[8], int c)
{
	return (table[((unsigned char)c >> 5)] & (1U << (((unsigned char)c) & 0x1f)));
}

typedef enum {
    spx_error = -1,
    spx_ok
} status;

status
spx_http_parse_request_line(char *buf)
{
    size_t  pos, uri_start, uri_end, http;
    enum {
        spx_start = 0,
        spx_method,
        spx_sp_before_uri,
        spx_uri,
        spx_sp_before_http,
        spx_proto,
        spx_almost_done,
        spx_done
    } state;

    state = spx_start;

    while (state != spx_done)   {



    }
    return spx_ok;
}
