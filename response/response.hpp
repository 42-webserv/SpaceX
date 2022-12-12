#ifndef __RESPONSE_HPP__
#define __RESPONSE_HPP__

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

struct Response {
	typedef std::pair<std::string, std::string> header;

private:
	std::vector<header> headers_;
	int					versionMinor_;
	int					versionMajor_;
	std::vector<char>	body_;
	bool				keepAlive_;
	unsigned int		statusCode_;
	std::string			status_;

	std::string
	make() const {
		std::stringstream stream;
		stream << "HTTP/" << versionMajor_ << "." << versionMinor_ << " " << status_
			   << "\r\n";
		for (std::vector<Response::header>::const_iterator it = headers_.begin();
			 it != headers_.end(); ++it)
			stream << it->first << ": " << it->second << "\r\n";
		std::string data(body_.begin(), body_.end());
		stream << data << "\r\n";
		return stream.str();
	}
};

#endif