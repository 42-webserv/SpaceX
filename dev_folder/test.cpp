#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

int main( void ) {
	int  fd = open( "aaa", O_RDWR | O_NONBLOCK, 0777 );
	char asdf[21];

	write( fd, "42424242424242424242424", 23 );

	read( fd, asdf, 20 );
	asdf[20] = 0;
	printf( "%s\n", asdf );

	write( fd, "42424242424242424242424", 23 );

	// read( fd, asdf, 20 );
	// asdf[20] = 0;
	// printf( "%s\n", asdf );
	// read( fd, asdf, 20 );
	// asdf[20] = 0;
	// printf( "%s\n", asdf );
}
