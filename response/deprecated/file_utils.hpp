
#pragma once

#include <fcntl.h>
#include <string>
#include <unistd.h>

off_t
getLength(const std::string dir);

const char* fileToChar(const std::string dir);

static int
fileOpen(const std::string dir);