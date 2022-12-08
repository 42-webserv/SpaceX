#ifndef __URL_HPP__
#define __URL_HPP__

#include <iostream>

struct Url {
  std::string scheme;
  std::string hostname;
  std::string port;
  std::string path;
  std::string query;
  std::string fragment;
};

#endif