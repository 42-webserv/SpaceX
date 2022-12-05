#include "syntax.hpp"

namespace {

    static uint32_t  syntax_table[] = {
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

    static uint32_t  start_line[] = {
        0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */

                    /* ?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
        0x00002000, /* 0000 0000 0000 0000  0010 0000 0000 0000 */

                    /* _^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
        0x87fffffe, /* 1000 0111 1111 1111  1111 1111 1111 1110 */

                    /*  ~}| {zyx wvut srqp  onml kjih gfed cba` */
        0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */

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

    inline status
    error_(const char *msg)
    {
        // throw(msg);
        std::cerr << msg << std::endl;
        return spx_error;
    }

    inline int
    syntax_(const uint32_t table[8], int c)
    {
        return (table[((unsigned char)c >> 5)] & (1U << (((unsigned char)c) & 0x1f)));
    }

}

status
spx_http_syntax_start_line_request(char *buf)
{
    size_t  pos, method_len, http_len;
    char   *find_end_pos;
    enum    {
        spx_start = 0,
        spx_method,
        spx_sp_before_uri,
        spx_uri_start,
        spx_uri,
        spx_sp_before_http,
        spx_proto,
        spx_almost_done,
        spx_done
    } state;

    state = spx_start;
    pos = 0;
    while (state != spx_done)   {
        switch (state) {


            case spx_start: // start
                if (syntax_(start_line, buf[pos])) {
                    state = spx_method;
                    // add request start point to struct
                    find_end_pos = strchr(&buf[pos], ' ');
                    method_len = find_end_pos - &buf[pos];
                    break;
                }
                return error_("invalid line start");


            case spx_method: // check method

                switch (method_len) {
                    case 3:
                        if (strncmp(&buf[pos], "GET", 3) == 0) {
                            // add method to struct or class_member
                            state = spx_sp_before_uri;
                            break;
                        }
                        if (strncmp(&buf[pos], "PUT", 3) == 0) {
                            // add method to struct or class_member
                            state = spx_sp_before_uri;
                        }
                        break;

                    case 4:
                        if (strncmp(&buf[pos], "POST", 4) == 0) {
                            // add method to struct or class_member
                            state = spx_sp_before_uri;
                            break;
                        }
                        if (strncmp(&buf[pos], "HEAD", 4) == 0) {
                            // add method to struct or class_member
                            state = spx_sp_before_uri;
                        }
                        break;

                    case 6:
                        if (strncmp(&buf[pos], "DELETE", 6) == 0) {
                            // add method to struct or class_member
                            state = spx_sp_before_uri;
                        }
                        break;

                    case 7:
                        if (strncmp(&buf[pos], "OPTIONS", 7) == 0) {
                            // add method to struct or class_member
                            state = spx_sp_before_uri;
                        }
                        break;

                    default:
                        break;
                }

                if (state != spx_sp_before_uri) {
                    return error_("invalid method");
                }
                pos += method_len;
                break;


            case spx_sp_before_uri: // check uri
                if (buf[pos] == ' ') {
                    ++pos;
                    state = spx_uri_start;
                    break;
                }
                return error_("invalid uri or method");


            case spx_uri_start:
                if (buf[pos] == '/') {
                    ++pos;
                    state = spx_uri;
                    break;
                }
                return error_("invalid uri start");


            case spx_uri: // start uri check
                if (syntax_(syntax_table, buf[pos])) {
                    ++pos;
                    break;
                }
                switch (buf[pos]) {
                    case ' ':
                        state = spx_sp_before_http;
                        break;

                    default:
                        break;
                }
                if (state != spx_sp_before_http) {
                    return error_("invalid uri");
                }
                break;


            case spx_sp_before_http:
                if (buf[pos] == ' ') {
                    ++pos;
                    state = spx_proto;
                    break;
                }
                return error_("invalid proto or uri");


            case spx_proto:
                find_end_pos = strchr(&buf[pos], '\r');
                http_len = find_end_pos - &buf[pos];

                switch (http_len)   {

                    case 4:
                        if (strncmp(&buf[pos], "HTTP", 4) == 0) {
                            // add proto to struct or class_member
                            state = spx_almost_done;
                        }
                        break;

                    case 5:
                        if (strncmp(&buf[pos], "HTTP/", 5) == 0) {
                            // add proto to struct or class_member
                            state = spx_almost_done;
                        }
                        break;

                    case 8:
                        if (strncmp(&buf[pos], "HTTP/1.1", 8) == 0) {
                            // add proto to struct or class_member
                            state = spx_almost_done;
                            break;
                        }
                        if (strncmp(&buf[pos], "HTTP/1.0", 8) == 0) {
                            // add proto to struct or class_member
                            state = spx_almost_done;
                        }
                        break;

                    default:
                        break;
                }

                if (state != spx_almost_done) {
                    return error_("invalid http version or end line");
                }
                pos += http_len;
                break;

            case spx_almost_done:
                if (buf[pos] == '\r' && buf[pos + 1] == '\n')   {
                    state = spx_done;
                    break;
                }
                return error_("invalid request end line");

            default:
                return error_("invalid request");

        }
    }
    return spx_ok;
}
