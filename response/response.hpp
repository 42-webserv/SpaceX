#ifndef __RESPONSE_HPP__
#define __RESPONSE_HPP__
#pragma once

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// #include "file_utils.hpp"

// for file handling
#include "response_form.hpp"
#include <fcntl.h>
#include <unistd.h>

#define SERVER_HEADER_KEY "Server"
#define SERVER_HEADER_VALUE "SpaceX"
#define SERVER_HEADER_CONTENT_LENGTH "Content-Legnth"
#define TEST_FILE "html/test.html"

struct Response {
	typedef std::pair<std::string, std::string> header;

private:
	std::vector<header> headers_;
	int					version_minor_;
	int					version_major_;
	std::vector<char>	body_;
	bool				keep_alive_;
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
	/*
		void
		makeBaseHeader() {
			headers_.push_back(header("Server:", " webserv"));
			headers_.push_back(header("Date",))
			// headers_.push_back(header("Content-Type:", " webserv"));
		}
	*/
	void
	generateStatus_(enum http_status status) {
		std::stringstream stream;
		stream << status << " " << http_status_sstr(status);
		status_ = stream.str();
	};
	/*
		void
		setContentLength(const std::string dir) {
			off_t			  length = getLength(dir);
			std::stringstream ss;
			ss << length;
			headers_.push_back(header(SERVER_HEADER_CONTENT_LENGTH, ss.str()));
		}
	*/
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
	void
	generateCommonHeader() {
		headers_.push_back(header(SERVER_HEADER_KEY, SERVER_HEADER_VALUE));
		headers_.push_back(header());
		// TODO: lseek 로 content-length 구해서 넣기
		// headers_.push_back();
	};

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
	// make 분리
	//  TODO : 요청에 따라 BODY 여부 확인
	//  TODO : 요청내용이 어떻게 들어오는지 확인
	//  TODO : 요청에 따른 필수 Header key value 셋팅

	void
	setBody(const char* dir) {

		int fd = open(dir, O_RDONLY);
		if (fd < 0)
			std::cout << "fd error" << std::endl;
		off_t length = lseek(fd, 0, SEEK_END);
		off_t i = 0;
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

	void
	setContentType(const char* content_type) {
		if (content_type == NULL)
			headers_.push_back(header("Content-Type", "text"));
		else
			headers_.push_back(header("Content-Type", content_type));
	};
};

/*
int main( void ) {
	std::cout << HTTP_STATUS_BAD_REQUEST << std::endl;
	std::cout << http_status_str( HTTP_STATUS_NOT_ACCEPTABLE ) << std::endl;
}

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