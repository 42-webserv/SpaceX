#pragma once
#ifndef __SPACEX__CORE_UTIL_BOX_HPP__
#define __SPACEX__CORE_UTIL_BOX_HPP__

#include "spx_core_type.hpp"
#include <cerrno>
#include <fstream>
#include <iostream>
#include <string>

std::string const generator_error_page_(uint32_t const& error_code);

template <typename T>
inline void
spx_log_(T msg) {
#ifdef DEBUG
	std::cout << "\033[1;32m" << msg << "\033[0m" << std::endl;
#else
	(void)msg;
#endif
}

template <typename T>
inline void
spx_log_(std::string id, T msg) {
#ifdef DEBUG
	std::cout << "\033[1;32m" << id << ": " << msg << "\033[0m" << std::endl;
#else
	(void)msg;
#endif
}

inline void
spx_log_check_(std::string const& msg) {
#ifdef LOG_MODE
	std::fstream file;
	file.open("./log/request.log", std::ios::out | std::ios::ate);
	if (file.is_open()) {
		file << msg << std::endl;
	}
#else
	(void)msg;
#endif
}

inline void
error_exit(std::string err, int (*func)(int), int fd) {
	if (func != NULL) {
		func(fd);
	}
	std::cerr << errno << std::endl;
	perror(err.c_str());
	exit(spx_error);
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
