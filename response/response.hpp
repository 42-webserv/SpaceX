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

#include "client_buffer.hpp"
#include "response_form.hpp"
// tmp
#include "tmp_kqueue_headers.hpp"

#define SERVER_HEADER_KEY "Server"
#define SERVER_HEADER_VALUE "SpaceX"
#define SERVER_HEADER_CONTENT_LENGTH "Content-Legnth"
#define TEST_FILE "html/test.html"

#define CONTENT_TYPE "Content-Type";

#define MIME_TYPE_HTML "text/html"
#define MIME_TYPE_JPG "image/jpg"
#define MIME_TYPE_JPEG "image/jpeg"
#define MIME_TYPE_PNG "image/png"
#define MIME_TYPE_BMP "image/bmp"
#define MIME_TYPE_TEXT "text"

struct Response {
	typedef std::pair<std::string, std::string> header;

private:
	std::vector<header> headers_;
	int					version_minor_;
	int					version_major_;
	std::vector<char>	body_;
	unsigned int		status_code_;
	std::string			status_;

	std::string
	make() const {
		std::stringstream stream;
		stream << "HTTP/" << version_major_ << "." << version_minor_ << " " << status_
			   << "\r\n";
		for (std::vector<Response::header>::const_iterator it = headers_.begin();
			 it != headers_.end(); ++it)
			stream << it->first << ": " << it->second << "\r\n";
		std::string data(body_.begin(), body_.end());
		stream << data << "\r\n";
		return stream.str();
	}
	/*
	Server: nginx/1.23.3
	Date: Tue, 13 Dec 2022 15:45:26 GMT
	Content-Type: text/html
	Content-Length: 157
	Connection: close
	*/
	void
	generateStatus_(enum http_status status) {
		std::stringstream stream;
		stream << status << " " << http_status_sstr(status);
		status_ = stream.str();
	};

public:
	// constructor
	Response()
		: version_minor_(1)
		, version_major_(1)
		, status_code_(HTTP_STATUS_BAD_REQUEST) {
		generateStatus_(HTTP_STATUS_OK);
	};

	Response(enum http_status status)
		: version_minor_(1)
		, version_major_(1)
		, status_code_(status) {
		generateStatus_(status);
	};

	int
	setContentLength_fileOpen(std::vector<struct kevent>& change_list, ClientBuffer& client_buffer) {
		int fd = open(client_buffer.req_res_queue_.front().first.req_target_.c_str(), O_RDONLY);

		// when URL file open failed
		if (fd < 0) {
			// TODO : load 404 file from confing
			fd			 = open("html/error.html", O_RDONLY);
			off_t length = lseek(fd, 0, SEEK_END);
			lseek(fd, SEEK_CUR, SEEK_SET);

			std::stringstream ss;
			ss << length;

			headers_.push_back(header(SERVER_HEADER_CONTENT_LENGTH, ss.str()));
		}
		// 이벤트 등록을 여기서?
		// change_list, fd, EVFILT_READ OR WRITE<status>, EV_ADD | EV_ENABLE <actions>, 0, 0, t_client_buf<udata>
		// add_event_change_list(change_list, fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, &client_buffer);
	}

	void
	generateCommonHeader() {
		headers_.push_back(header(SERVER_HEADER_KEY, SERVER_HEADER_VALUE));

		// headers_.push_back(header());
	};

	// TODO : 분기 생성
	/*
	BAD REQUEST 이거나 config가 비었을경우 text/html
	application/octet-stream => out 파일일경우 (8비트파일)

	*/
	void
	setContentType(t_req_field& req) {

		// bad request or errorpage exceptions
		if (req.body_flag_ >= HTTP_STATUS_BAD_REQUEST) {
			headers_.push_back(header("Content-Type", MIME_TYPE_HTML));
			return;
		}

		// request extention checking
		std::string			   uri			= req.req_target_;
		std::string::size_type uri_ext_size = uri.find_last_of('.');
		std::string			   ext;

		if (uri_ext_size)
			ext = uri.substr(uri_ext_size + 1);
		if (ext == "png")
			headers_.push_back(header("Content-Type", MIME_TYPE_PNG));
		else if (ext == "bmp")
			headers_.push_back(header("Content-Type", MIME_TYPE_BMP));
		else if (ext == "jpg")
			headers_.push_back(header("Content-Type", MIME_TYPE_JPG));
		else if (ext == "jpeg")
			headers_.push_back(header("Content-Type", MIME_TYPE_JPEG));
		else
			headers_.push_back(header("Content-Type", MIME_TYPE_TEXT));
	};

// this define will be replace to config file settings input
#define ERR_PAGE_URL "html/error.html"

	std::string
	make(ClientBuffer& client_buffer) {
		t_req_field	 cur_req = client_buffer.req_res_queue_.front().first;
		t_res_field& res	 = client_buffer.req_res_queue_.front().second;

		setContentType(cur_req);
		setContentLength_fileOpen(NULL, client_buffer);

		std::stringstream stream;
		stream << "HTTP/" << version_major_ << "." << version_minor_ << " " << status_
			   << "\r\n";
		for (std::vector<Response::header>::const_iterator it = headers_.begin();
			 it != headers_.end(); ++it)
			stream << it->first << ": " << it->second << "\r\n";
		stream << "\r\n";
		// std::string data(body_.begin(), body_.end());
		// stream << data << "\r\n";
		return stream.str();
	};

	/*
		// make
		std::string
		make_test() {
			std::stringstream stream;
			// open some file for response
			// content type check - extention check
			setContentType(NULL);
			setBody(TEST_FILE);
			stream << "HTTP/" << version_major_ << "." << version_minor_ << " " << status_
				   << "\r\n";
			for (std::vector<Response::header>::const_iterator it = headers_.begin();
				 it != headers_.end(); ++it)
				stream << it->first << ": " << it->second << "\r\n";
			stream << "\r\n";
			std::string data(body_.begin(), body_.end());
			// std::cout << data << std::endl;
			stream << data << "\r\n";
			return stream.str();
		}
	*/
	// make 분리
	//  TODO : 요청에 따라 BODY 여부 확인
	//  TODO : 요청내용이 어떻게 들어오는지 확인
	//  TODO : 요청에 따른 필수 Header key value 셋팅
	/*
		void
		setBody(const char* dir) {

			int fd = open(dir, O_RDONLY);
			if (fd < 0)
				std::cout << "fd error" << std::endl;
			off_t length = lseek(fd, 0, SEEK_END);
			off_t i		 = 0;
			char  buf[1024];

			std::stringstream ss;
			ss << length;
			headers_.push_back(header(SERVER_HEADER_CONTENT_LENGTH, ss.str()));

			std::cout << read(fd, buf, 1024)
					  << std::endl;
			int x = 0;
			while (buf[x]) {
				std::cout << *buf << std::endl;
				x++;
			}

			for (off_t i = 0; i < length; i++) {
				body_.push_back(buf[i]);
			}
			for (std::vector<char>::iterator it = body_.begin(); it != body_.end(); it++)
				std::cout << *it;
			std::cout << "\n";
		}
	*/
};

/*

HTTP/1.1 400 Bad Request
Server: nginx/1.23.3
Date: Tue, 13 Dec 2022 15:45:26 GMT
Content-Type: text/html
Content-Length: 157
Connection: close

<html>
<head><title>400 Bad Request</title></head>
<body>
<center><h1>400 Bad Request</h1></center>
<hr><center>nginx/1.23.3</center>
</body>
</html>

	const char* str = "<html>\r\n"
					  "<head><title>400 Bad Request</title></head>\r\n"
					  "<body>\r\n"
					  "<center><h1>400 Bad Request</h1></center>\r\n"
					  "<hr><center>nginx/1.23.3</center>\r\n"
					  "</body>\r\n"
					  "</html>\r\n";

*/

#endif