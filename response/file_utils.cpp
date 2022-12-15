#include "file_utils.hpp"

int
fileOpen(const std::string dir) {
	int fd = open(dir.c_str(), O_RDONLY);
	// error check
	return fd;
}

off_t
getLength(const std::string dir) {
	int fd = fileOpen(dir);
	return lseek(fd, 0, SEEK_END);
}

const char*
fileToChar(const std::string dir) {
	std::stringstream ss;
	int				  fd = fileOpen(dir);
}