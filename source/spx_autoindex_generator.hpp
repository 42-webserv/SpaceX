#pragma once
#ifndef __SPX_AUTOINDEX_GENERATOR_HPP__
#define __SPX_AUTOINDEX_GENERATOR_HPP__

#include "spx_core_type.hpp"
#include "spx_core_util_box.hpp"

#include <dirent.h>
#include <libgen.h>
#include <sys/stat.h>

#include "spx_response_generator.hpp"

#define HTML_HEAD_TITLE "<html>\r\n<head><title>Index of "
#define HTML_HEAD_TO_BODY "</title>\
<meta charset=\"utf-8\"><style>a{text-decoration: none; color: black;} a:hover{color: blue;}</style>\
</head>\r\n<body>\r\n<h1>Index of "
#define HTML_BEFORE_LIST "</h1><hr>\r\n<pre>\r\n"
#define HTML_AFTER_LIST "</pre>\r\n<hr>\r\n</body>\r\n</html>"
#define TD_STYLE "style=\"padding-left: 500px;padding-right: 100px;\""

#define A_TAG_START "<a href=\""
#define A_TAG_END "\">"
#define CLOSE_A_TAG "</a>"

std::string
generate_autoindex_page(int& req_fd, uri_resolved_t& path);

#endif
