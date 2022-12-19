#ifndef __RESPONSE_HPP__
#define __RESPONSE_HPP__
#pragma once

#include <algorithm>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>

#include "../dev_folder/ClientBuffer.hpp"
#include "ClientBuffer.hpp"
#include "error_pages.hpp"
#include "response_form.hpp"

// tmp for add_event_chagne
#include "tmp_kqueue_headers.hpp"

#define SERVER_HEADER_KEY "Server"
#define SERVER_HEADER_VALUE "SpaceX"
#define CONTENT_LENGTH "Content-Legnth"
#define CONTENT_TYPE "Content-Type";

#define MIME_TYPE_HTML "text/html"
#define MIME_TYPE_JPG "image/jpg"
#define MIME_TYPE_JPEG "image/jpeg"
#define MIME_TYPE_PNG "image/png"
#define MIME_TYPE_BMP "image/bmp"
#define MIME_TYPE_TEXT "text"
#define MIME_TYPE_DEFUALT "application/octet-stream"

// This must change to file that loaded from CONFIG file
#define ERR_PAGE_URL "html/404error.html"

#define LF (u_char)'\n'
#define CR (u_char)'\r'
#define CRLF "\r\n"

struct Response {
	typedef std::pair<std::string, std::string> header;

private:
	std::vector<header> headers_;
	int					version_minor_;
	int					version_major_;
	unsigned int		status_code_;
	std::string			status_;
	bool				keep_alive_ = true;

	std::string
	make_to_string() const {
		std::stringstream stream;
		stream << "HTTP/" << version_major_ << "." << version_minor_ << " " << status_code_ << " " << status_
			   << CRLF;
		for (std::vector<Response::header>::const_iterator it = headers_.begin();
			 it != headers_.end(); ++it)
			stream << it->first << ": " << it->second << CRLF;
		return stream.str();
	}
	// Response Header EXAMPLE
	// HTTP/1.1 400 Bad Request
	// Server: nginx/1.23.3
	// Date: Tue, 13 Dec 2022 15:45:26 GMT
	// Content-Type: text/html
	// Content-Length: 157
	// Connection: close

	int
	file_open(const char* dir) {
		int fd = open(dir, O_RDONLY);
		if (fd < 0)
			return open(ERR_PAGE_URL, O_RDONLY);
	}

	void
	setContentLength(int fd) {
		off_t length = lseek(fd, 0, SEEK_END);
		lseek(fd, SEEK_CUR, SEEK_SET);

		std::stringstream ss;
		ss << length;

		headers_.push_back(header(CONTENT_LENGTH, ss.str()));
	}

	void
	setContentType(std::string uri) {

		// request extention checking
		std::string::size_type uri_ext_size = uri.find_last_of('.');
		std::string			   ext;

		if (uri_ext_size)
			ext = uri.substr(uri_ext_size + 1);
		if (ext == "html")
			headers_.push_back(header("Content-Type", MIME_TYPE_HTML));
		else if (ext == "png")
			headers_.push_back(header("Content-Type", MIME_TYPE_PNG));
		else if (ext == "jpg")
			headers_.push_back(header("Content-Type", MIME_TYPE_JPG));
		else if (ext == "jpeg")
			headers_.push_back(header("Content-Type", MIME_TYPE_JPEG));
		else if (ext == "txt")
			headers_.push_back(header("Content-Type", MIME_TYPE_TEXT));
		else
			headers_.push_back(header("Content-Type", MIME_TYPE_DEFUALT));
	};

	std::string
	handle_static_error_page() {
		char* err = http_error_400_page;
		// this will write to vector<char> response_body
		return make_to_string();
	}

public:
	std::string
	make_error_response(std::vector<struct kevent>& change_list, ClientBuffer& client_buffer, http_status error_code) {

		status_		 = http_status_sstr(error_code);
		status_code_ = error_code;

		if (error_code == HTTP_STATUS_BAD_REQUEST)
			keep_alive_ = false;

		int error_req_fd = open(ERR_PAGE_URL, O_RDONLY);
		if (error_req_fd < 0)
			return handle_static_error_page();
		add_change_list(change_list, error_req_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, &client_buffer);
	}

	// this is main logic to make response
	std::string
	setting_response_header(std::vector<struct kevent>& change_list, ClientBuffer& client_buffer) {
		t_req_field current_request = client_buffer.req_res_queue_.front().first;
		std::string uri				= current_request.req_target_;
		int			request_fd		= file_open(uri.c_str());

		// File Not Found - URL
		if (request_fd < 0)
			return make_error_response(change_list, client_buffer, HTTP_STATUS_NOT_FOUND);
		setContentType(uri);
		setContentLength(request_fd);
		// body event register
		add_change_list(change_list, request_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, &client_buffer);
		return make_to_string();
	}
};

#endif