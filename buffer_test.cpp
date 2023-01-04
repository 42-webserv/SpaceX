#include "source/spx_buffer.hpp"
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

typedef SpxBuffer	  buf_t;
typedef SpxReadBuffer rdbuf_t;

int
main() {

	rdbuf_t rdbuf(4, 5);
	// buf_t	buf;

	int fd = open("aaa", O_RDONLY);
	// printf("fd: %d\n", fd);
	fcntl(fd, F_SETFL, O_NONBLOCK);

	while (rdbuf.read_(fd)) {
	}

	rdbuf.write_(STDOUT_FILENO);
	return 0;
}