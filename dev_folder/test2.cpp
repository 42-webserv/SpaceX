#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <stdio.h>
#include <string>
#include <unistd.h>
#include <vector>

#define BUF_SIZE 5

int
main(void) {
	char buf[BUF_SIZE];
	int	 fd = open("/Users/seseo/SpaceX/dev_folder/tmp", O_RDONLY);

	printf("%d\n", fd);
	int n;
	do {
		n = read(fd, buf, BUF_SIZE);
		write(STDOUT_FILENO, buf, n);
	} while (n > 0);

	// perror(strerror(errno));
	// int  fd = open( "asdf", O_RDONLY | O_CREAT | O_NONBLOCK, 0644 );
	// int  fd2 = open( "asdf", O_RDONLY | O_CREAT | O_NONBLOCK, 0644 );
	// int  fd3 = open( "asdf", O_RDONLY | O_CREAT | O_NONBLOCK, 0644 );

	// printf( "%d\n", fd );
	// read( fd, buf, BUF_SIZE );
	// write( STDOUT_FILENO, buf, BUF_SIZE );

	// read( fd, buf, BUF_SIZE );
	// write( STDOUT_FILENO, buf, BUF_SIZE );

	// printf( "\n\n%d\n", fd2 );
	// read( fd2, buf, BUF_SIZE );
	// write( STDOUT_FILENO, buf, BUF_SIZE );

	// read( fd2, buf, BUF_SIZE );
	// write( STDOUT_FILENO, buf, BUF_SIZE );

	// printf( "\n\n%d\n", fd3 );
	// read( fd3, buf, BUF_SIZE );
	// write( STDOUT_FILENO, buf, BUF_SIZE );

	// read( fd3, buf, BUF_SIZE );
	// write( STDOUT_FILENO, buf, BUF_SIZE );
}
