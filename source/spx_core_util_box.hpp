#pragma once
#ifndef __SPACEX__CORE_UTIL_BOX_HPP__
#define __SPACEX__CORE_UTIL_BOX_HPP__

#include "spx_core_type.hpp"

#include <fstream>

#include <cerrno>
#include <cstdio>
#include <cstdlib>

#include <iomanip>
#include <ios>
#include <sys/time.h>

#define COLOR_RED "\033[1;31m"
#define COLOR_GREEN "\033[1;32m"
#define COLOR_YELLOW "\033[1;33m"
#define COLOR_BLUE "\033[1;34m"
#define COLOR_PURPLE "\033[1;35m"
#define COLOR_CYAN "\033[1;36m"
#define COLOR_WHITE "\033[1;37m"
#define COLOR_RESET "\033[0m"

#define BCOLOR_RED "\033[1;41m"
#define BCOLOR_GREEN "\033[1;42m"
#define BCOLOR_YELLOW "\033[1;43m"
#define BCOLOR_BLUE "\033[1;44m"
#define BCOLOR_PURPLE "\033[1;45m"
#define BCOLOR_CYAN "\033[1;46m"
#define BCOLOR_WHITE "\033[1;47m"

std::string const generator_error_page_(uint32_t const& error_code);

template <typename T>
inline void
spx_log_(T msg) {
#ifdef LOG_FILE_MODE
	std::fstream file;
	file.open("./log/request.log", std::ios::out | std::ios::app);
	if (file.is_open()) {
		file << msg << std::endl;
	}
	file.close();
#endif

#ifdef DEBUG
	std::cout << COLOR_GREEN << msg << COLOR_RESET << std::endl;
#else
	(void)msg;
#endif
}

template <typename T>
inline void
spx_log_(std::string id, T msg) {
#ifdef LOG_FILE_MODE
	std::fstream file;
	file.open("./log/request.log", std::ios::out | std::ios::app);
	if (file.is_open()) {
		file << id << ": " << msg << std::endl;
	}
	file.close();
#endif

#ifdef DEBUG
	std::cout << COLOR_GREEN << id << ": " << msg << COLOR_RESET << std::endl;
#else
	(void)id;
	(void)msg;
#endif
}

inline void
error_log_(std::string const& msg) {
	std::cerr << BCOLOR_RED << msg << COLOR_RESET << std::endl;
}

inline void
error_str(std::string err) {
	std::cerr << BCOLOR_RED << "[" << err << "]" << COLOR_RESET << "\terrno: " << errno << "\t" << strerror(errno) << std::endl;
}

template <typename T>
inline void
error_fn(std::string err, T (*func)(int), int fd) {
	error_str(err);
	if (func != NULL) {
		func(fd);
	}
}

inline void
error_exit(std::string err) {
	error_log_(err);
	exit(spx_error);
}

#define METHOD__MAP_(XX) \
	XX(1, GET)           \
	XX(2, POST)          \
	XX(3, PUT)           \
	XX(4, DELETE)        \
	XX(5, HEAD)

#define METHOD__MAP_COLOR_(XX) \
	XX(1, GET, \033[1;42m)           \
	XX(2, POST, \033[1;44m)          \
	XX(3, PUT, \033[1;46m)           \
	XX(4, DELETE, \033[1;41m)        \
	XX(5, HEAD, \033[1;43m)

inline std::string
method_map_str_(int const status) {
	enum {
		REQ_GET		  = 1 << 1,
		REQ_POST	  = 1 << 2,
		REQ_PUT		  = 1 << 3,
		REQ_DELETE	  = 1 << 4,
		REQ_HEAD	  = 1 << 5,
		REQ_UNDEFINED = 1 << 6
	};
	switch (status) {
#define XX(num, name) \
	case REQ_##name:  \
		return #name;
		METHOD__MAP_(XX)
#undef XX
	default:
		return "<unknown>";
	}
}

inline std::string
method_map_str_color_(int const status) {
	enum {
		REQ_GET		  = 1 << 1,
		REQ_POST	  = 1 << 2,
		REQ_PUT		  = 1 << 3,
		REQ_DELETE	  = 1 << 4,
		REQ_HEAD	  = 1 << 5,
		REQ_UNDEFINED = 1 << 6
	};
	switch (status) {
#define XX(num, name, color) \
	case REQ_##name:         \
		return #color #name COLOR_RESET;
		METHOD__MAP_COLOR_(XX)
#undef XX
	default:
		return BCOLOR_RED "UNKNOWN" COLOR_RESET;
	}
}

inline void
spx_console_log_(std::string start_line, struct timeval const& established, const size_t byte, const int method, const std::string uri) {
	std::string color;
	switch (start_line.at(9)) {
	case '1': {
		color = COLOR_BLUE;
		break;
	}
	case '2': {
		color = COLOR_GREEN;
		break;
	}
	case '3': {
		color = COLOR_YELLOW;
		break;
	}
	case '4': {
		color = COLOR_RED;
		break;
	}
	default: {
		color = BCOLOR_RED;
		break;
	}
	}
	struct timeval now;
	gettimeofday(&now, NULL);
	int msec = (now.tv_usec - established.tv_usec) / 1000;
	if (msec < 0) {
		msec += 1000;
		--now.tv_sec;
	}

	std::cout
		<< color << std::setw(35) << std::left << start_line
		<< std::setw(3) << std::right << now.tv_sec - established.tv_sec << "."
		<< std::setfill('0') << std::setw(3) << msec << " secs: "
		<< std::setfill(' ') << std::setw(13) << std::right << byte << " bytes ==>  "
		<< std::setw(20) << std::left << method_map_str_color_(method) << color << "  "
		<< uri << COLOR_RESET << std::endl;
}

#endif
