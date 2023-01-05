#pragma once
#ifndef __SPX__BUFFER__HPP
#define __SPX__BUFFER__HPP

#include <string>
#include <sys/uio.h>
#include <vector>

#include "spx_core_type.hpp"

class SpxBuffer {
protected:
	typedef std::vector<struct iovec> iov_t;

	iov_t  _buf;
	size_t _buf_size;
	size_t _partial_point;

	SpxBuffer(const SpxBuffer& spxbuf);
	SpxBuffer& operator=(const SpxBuffer& spxbuf);

	char*  push_front_addr_();
	char*  pull_front_addr_();
	void   delete_size_(size_t size);
	size_t move_partial_case_(SpxBuffer& to_buf, size_t size);
	size_t move_nonpartial_case_(SpxBuffer& to_buf, size_t size);

public:
	SpxBuffer();
	~SpxBuffer();

	void	clear_();
	size_t	move_(SpxBuffer& to_buf, size_t size);
	ssize_t write_(int fd);
	int		get_crlf_line_(std::string& line, size_t str_max_size);
	size_t	buf_size_();
};

class SpxReadBuffer : public SpxBuffer {
private:
	iov_t		 _rdbuf;
	const size_t _rdbuf_buf_size;
	const int	 _rdbuf_iov_size;

	SpxReadBuffer();
	SpxReadBuffer(const SpxReadBuffer& buf);
	SpxReadBuffer& operator=(const SpxReadBuffer& buf);

	void set_empty_buf_();

public:
	SpxReadBuffer(size_t _rdbuf_buf_size, int _rdbuf_iov_size);
	~SpxReadBuffer();

	ssize_t read_(int fd);
};

#endif