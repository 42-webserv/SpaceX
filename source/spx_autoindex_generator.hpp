#pragma once
#ifndef __SPX_AUTOINDEX_GENERATOR_HPP__
#define __SPX_AUTOINDEX_GENERATOR_HPP__

#include <dirent.h>
#include <iostream>
#include <libgen.h> //basename
#include <sstream>
#include <string>
#include <sys/stat.h>

#include "spacex.hpp"
#include "spx_response_generator.hpp"

#define HTML_HEAD_TITLE "<html>\r\n<head><title>Index of "
#define HTML_HEAD_TO_BODY "</title></head>\r\n<body>\r\n<h1>Index of "
#define HTML_BEFORE_LIST "</h1><hr>\r\n<pre>\r\n"
#define HTML_AFTER_LIST "</pre>\r\n<hr>\r\n</body>\r\n</html>"
#define TD_STYLE "style=\"padding-left: 500px;padding-right: 100px;\""

#define A_TAG_START "<a href=\""
#define A_TAG_END "\">"
#define CLOSE_A_TAG "</a>"

std::string
generate_autoindex_page(int& req_fd, uri_resolved_t& path);

#endif