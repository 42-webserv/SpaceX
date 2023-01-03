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
	(void)msg;
#endif
}

inline void
main_log_(std::string const& msg, std::string const& color = COLOR_WHITE) {
	std::cout << color << msg << COLOR_RESET << std::endl;
}

inline void
error_fn(std::string err, int (*func)(int), int fd) {
	if (func != NULL) {
		func(fd);
	}
	std::cerr << errno << std::endl;
	perror(err.c_str());
}

inline void
error_exit_msg(std::string err) {
	std::cerr << "\033[1;31m[ " << err << " ]\033[0m" << std::endl;
	exit(spx_error);
}

inline void
error_exit_msg_perror(std::string err) {
	std::cerr << errno << std::endl;
	perror(err.c_str());
	exit(spx_error);
}

#endif
