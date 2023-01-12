#pragma once
#ifndef __SPACEX__CORE_UTIL_BOX_HPP__
#define __SPACEX__CORE_UTIL_BOX_HPP__

#include "spx_core_type.hpp"

#include <fstream>

#include <cerrno>
#include <cstdio>
#include <cstdlib>

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
main_log_(std::string const& msg, std::string const& color = COLOR_WHITE) {
	std::cout << color << msg << COLOR_RESET << std::endl;
}

inline void
error_msg(std::string err) {
	std::cerr << BCOLOR_RED << "[" << err << "]" << COLOR_RESET << "\terrno: " << errno << "\t" << strerror(errno) << std::endl;
}

template <typename T>
inline void
error_fn(std::string err, T (*func)(int), int fd) {
	error_msg(err);
	if (func != NULL) {
		func(fd);
	}
}

inline void
error_exit(std::string err) {
	std::cerr << BCOLOR_RED << err << COLOR_RESET << std::endl;
	exit(spx_error);
}

#endif
