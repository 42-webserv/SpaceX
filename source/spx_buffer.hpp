#pragma once
#ifndef __SPX__BUFFER__HPP
#define __SPX__BUFFER__HPP

#include <string>
#include <sys/uio.h>
#include <vector>

#include "spx_core_type.hpp"

class SpxBuffer {
private:
	typedef std::vector<struct iovec> iov_t;

	iov_t  _buf;
	size_t _buf_size;
	size_t _partial_point;

	SpxBuffer(const SpxBuffer& spxbuf);
	SpxBuffer& operator=(const SpxBuffer& spxbuf);

	char* front_end_();
	char* push_front_addr_();
	char* pull_front_addr_();
	char* iov_base_(struct iovec& iov);
	char* iov_end_addr_(struct iovec& iov);

	size_t delete_size_for_move_(size_t size);
	size_t move_partial_case_(SpxBuffer& to_buf, size_t size);
	size_t move_nonpartial_case_(SpxBuffer& to_buf, size_t size);

public:
	SpxBuffer();
	~SpxBuffer();

	void	clear_();
	size_t	move_(SpxBuffer& to_buf, size_t size);
	size_t	delete_size_(size_t size);
	ssize_t write_(int fd);
	int		get_crlf_line_(std::string& line, size_t size = 8 * 1024);
	int		get_crlf_cpy_line_(std::string& line, size_t size);
	int		get_lf_line_(std::string& line, size_t size = 8 * 1024);
	size_t	find_pos_(char c, size_t max = -1);
	char	pos_val_(size_t pos);
	void	get_str_(std::string& str, size_t size);
	void	get_str_cpy_(std::string& str, size_t size);
	ssize_t write_debug_(int fd = 1);
	void	add_str(const std::string& str);
	size_t& buf_size_();
	iov_t&	get_buf_();
	size_t	get_partial_point_();
};

class SpxReadBuffer {
private:
	typedef std::vector<struct iovec> iov_t;

	iov_t		 _rdbuf;
	const size_t _rdbuf_buf_size;
	const int	 _rdbuf_iov_vec_size;

	SpxReadBuffer();
	SpxReadBuffer(const SpxReadBuffer& buf);
	SpxReadBuffer& operator=(const SpxReadBuffer& buf);

	void set_empty_buf_();

public:
	SpxReadBuffer(size_t _rdbuf_buf_size, int _rdbuf_iov_vec_size);
	~SpxReadBuffer();

	ssize_t read_(int fd, SpxBuffer& buf);
};

#endif
