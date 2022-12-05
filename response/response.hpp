#ifndef __RESPONSE_HPP__
#define __RESPONSE_HPP__

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

struct Response {
  typedef std::pair<std::string, std::string> header;

private:
  std::vector<header> headers;
  int versionMinor;
  int versionMajor;
  std::vector<char> body;
  bool keepAlive;
  unsigned int statusCode;
  std::string status;

  std::string make() const {
    std::stringstream stream;
    stream << "HTTP/" << versionMajor << "." << versionMinor << " " << status
           << "\r\n";
    for (std::vector<Response::header>::const_iterator it = headers.begin();
         it != headers.end(); ++it)
      stream << it->first << ": " << it->second << "\r\n";
    std::string data(body.begin(), body.end());
    stream << data << "\r\n";
    return stream.str();
  }
};

#endif