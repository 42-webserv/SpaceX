#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <string>

// #define BUF_LEN 10
// #define IOVEC_CNT 4

enum { BUF_LEN = 10, IOVEC_CNT = 4 };

class ClientBuffer {
   private:
	struct iovec *rbuf;
	std::string   rsaved;
	struct iovec *wbuf;

	ClientBuffer() {
		set_buffer( IOVEC_CNT, BUF_LEN );
	}
	~ClientBuffer() {
		del_buffer( rbuf, IOVEC_CNT );
	}
};

struct iovec *set_buffer( int iovec_cnt, int buf_len ) {
	struct iovec *tmp = new struct iovec[iovec_cnt];
	for ( int i = 0; i < iovec_cnt; i++ ) {
		tmp[i].iov_base = new char[buf_len];
		tmp[i].iov_len = buf_len;
	}
	return tmp;
}

void del_buffer( struct iovec *buf, int iovec_cnt ) {
	for ( int i = 0; i < iovec_cnt; i++ ) {
		delete (char *)buf[i].iov_base;
	}
	delete buf;
}

void set_rbuf_len( struct iovec *buf, ssize_t read_len, int iovec_cnt,
				   int buf_len ) {
	int n = read_len / buf_len;
	int r = read_len % buf_len;

	if ( r != 0 ) {
		buf[n++].iov_len = r;
	}
	while ( n < iovec_cnt ) {
		buf[n++].iov_len = 0;
	}
}

void reset_buf_len( struct iovec *buf, int iovec_cnt, int buf_len ) {
	for ( int i = 0; i < iovec_cnt; i++ ) {
		buf[i].iov_len = buf_len;
	}
}

int main( void ) {
	int           fd = open( "aaa", O_RDWR | O_NONBLOCK, 0777 );
	int           fd2 = open( "bbb", O_RDWR | O_NONBLOCK, 0777 );
	struct iovec *buf;

	buf = set_buffer( IOVEC_CNT, BUF_LEN );

	int n;
	n = readv( fd, buf, IOVEC_CNT );
	set_rbuf_len( buf, n, IOVEC_CNT, BUF_LEN );

	printf( "read: %lu, %lu, %lu\n", buf[0].iov_len, buf[1].iov_len,
			buf[2].iov_len );
	printf( "len: %d\n", n );

	writev( STDOUT_FILENO, buf, IOVEC_CNT );
	printf( "\n" );

	reset_buf_len( buf, IOVEC_CNT, BUF_LEN );

	n = readv( fd2, buf, IOVEC_CNT );
	set_rbuf_len( buf, n, IOVEC_CNT, BUF_LEN );

	printf( "read: %lu, %lu, %lu\n", buf[0].iov_len, buf[1].iov_len,
			buf[2].iov_len );
	printf( "len: %d\n", n );

	writev( STDOUT_FILENO, buf, IOVEC_CNT );
	printf( "\n" );

	reset_buf_len( buf, IOVEC_CNT, BUF_LEN );

	del_buffer( buf, IOVEC_CNT );

	// read( fd, asdf, 20 );
	// asdf[20] = 0;
	// printf( "%s\n", asdf );

	// write( fd, "42424242424242424242424", 23 );
	// write( fd, "42424242424242424242424", 23 );

	// read( fd, asdf, 20 );
	// asdf[20] = 0;
	// printf( "%s\n", asdf );
	// read( fd, asdf, 20 );
	// asdf[20] = 0;
	// printf( "%s\n", asdf );
}
