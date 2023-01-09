#include "source/spx_buffer.hpp"
#include <fcntl.h>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>

typedef SpxBuffer	  buf_t;
typedef SpxReadBuffer rdbuf_t;

int
main() {
	{
		for (int i = 3; i < 15; i++) {
			rdbuf_t rdbuf(i, 4);
			buf_t	buf;

			int fd1 = open("aaa", O_WRONLY | O_CREAT | O_TRUNC | O_NONBLOCK, 0644);
			int fd2 = open("aaa", O_RDONLY | O_NONBLOCK);
			// printf("fd1: %d\n", fd1);
			// printf("fd2: %d\n", fd2);

			write(fd1, "testtesttest\r\ntesttesttest\r\ntesttesttest\r\n\r\n", 44);

			while (rdbuf.read_(fd2)) {
				;
			}
			while (rdbuf.buf_size_() > 3) {
				// std::cout << "asdf" << std::endl;
				rdbuf.move_(buf, 44);
			}

			std::string str;

			std::cout << buf.get_crlf_line_(str) << ", " << str << std::endl;
		}
	}
	// {
	// 	rdbuf_t rdbuf(3, 4);
	// 	buf_t	buf;

	// 	int fd1 = open("aaa", O_WRONLY | O_CREAT | O_TRUNC | O_NONBLOCK, 0644);
	// 	int fd2 = open("aaa", O_RDONLY | O_NONBLOCK);
	// 	// printf("fd1: %d\n", fd1);
	// 	// printf("fd2: %d\n", fd2);

	// 	write(fd1, "testtesttest\r\ntesttesttest\r\ntesttesttest\r\n\r\n", 44);

	// 	while (rdbuf.read_(fd2)) {
	// 		;
	// 	}
	// 	for (int i = 0; i < rdbuf.buf_size_(); i++) {
	// 		std::cout << rdbuf.pos_val_(i);
	// 	}
	// 	std::string str;
	// 	std::cout << rdbuf.get_crlf_line_(str) << ", " << str << std::endl;
	// 	str.clear();
	// 	std::cout << rdbuf.get_crlf_line_(str) << ", " << str << std::endl;
	// 	str.clear();
	// 	std::cout << rdbuf.get_crlf_line_(str) << ", " << str << std::endl;
	// 	str.clear();
	// 	std::cout << rdbuf.get_crlf_line_(str) << ", " << str << std::endl;
	// 	str.clear();
	// 	std::cout << rdbuf.get_crlf_line_(str) << ", " << str << std::endl;
	// 	// std::cout << std::endl;
	// }
	// {
	// 	rdbuf_t rdbuf(10, 10);
	// 	buf_t	buf;

	// 	int fd1 = open("bbb", O_RDONLY | O_NONBLOCK);
	// 	// printf("fd1: %d\n", fd1);
	// 	// printf("fd2: %d\n", fd2);

	// 	rdbuf.read_(fd1);

	// 	rdbuf.move_(buf, 10);
	// 	buf.write_(STDOUT_FILENO);
	// 	rdbuf.write_(STDOUT_FILENO);
	// }

	return 0;
}