#include "spx_buffer.hpp"
#include <iostream>

SpxBuffer::SpxBuffer()
	: _buf()
	, _buf_size(0)
	, _partial_point(0) {
	// std::cout << "constructor buf size " << _buf.size() << std::endl;
}

SpxBuffer::~SpxBuffer() {
	// std::cout << "destructor buf size " << _buf.size() << std::endl;
	for (iov_t::iterator it = _buf.begin(); it != _buf.end(); ++it) {
		delete[] static_cast<char*>(it->iov_base);
	}
}

char*
SpxBuffer::push_front_addr_() {
	return static_cast<char*>(_buf.front().iov_base) + _partial_point;
}

char*
SpxBuffer::pull_front_addr_() {
	return static_cast<char*>(_buf.front().iov_base) - _partial_point;
}

void
SpxBuffer::delete_size_(size_t size) {
	iov_t::iterator it = _buf.begin();
	if (size < _buf_size) {
		_buf_size -= size;
		while (size >= it->iov_len) {
			delete[] static_cast<char*>(it->iov_base);
			size -= it->iov_len;
			++it;
		}
		it->iov_len -= size;
		if (it == _buf.begin()) {
			_partial_point += size;
			return;
		}
		_partial_point = size;
		_buf.erase(_buf.begin(), it);
	} else {
		clear_();
	}
}

size_t
SpxBuffer::move_partial_case_(SpxBuffer& to_buf, size_t size) {
	size_t		 iov_size = 0;
	size_t		 del_size;
	struct iovec new_iov;

	iov_t::iterator it = _buf.begin();

	if (size < it->iov_len) {
		new_iov.iov_base = new char[size];
		new_iov.iov_len	 = size;
		memcpy(new_iov.iov_base, push_front_addr_(), size);
		_partial_point += size;
		to_buf._buf.push_back(new_iov);
		return size;
	}

	new_iov.iov_base = new char[it->iov_len];
	new_iov.iov_len	 = it->iov_len;
	memcpy(new_iov.iov_base, push_front_addr_(), it->iov_len);
	del_size = it->iov_len;
	to_buf._buf.push_back(new_iov);
	delete_size_(del_size);
	if (size - del_size) {
		move_nonpartial_case_(to_buf, size - del_size);
	}
	return size;
}

size_t
SpxBuffer::move_nonpartial_case_(SpxBuffer& to_buf, size_t size) {
	size_t		 tmp_size = size;
	struct iovec new_iov;

	iov_t::iterator it = _buf.begin();

	while (tmp_size) {
		if (tmp_size >= it->iov_len) {
			tmp_size -= it->iov_len;
			++it;
		} else {
			new_iov.iov_base = new char[tmp_size];
			new_iov.iov_len	 = tmp_size;
			memcpy(new_iov.iov_base, it->iov_base, tmp_size);
			break;
		}
	}
	if (it != _buf.begin()) {
		to_buf._buf.insert(to_buf._buf.end(), _buf.begin(), it);
	}
	if (tmp_size) {
		to_buf._buf.push_back(new_iov);
	}
	// std::cout << "nonpartial to_buf " << to_buf._buf.size() << std::endl;
	// std::cout << "nonpartial to_buf " << static_cast<char*>(to_buf._buf.front().iov_base) << std::endl;
	delete_size_(size);
	return size;
}

size_t
SpxBuffer::move_(SpxBuffer& to_buf, size_t size) {
	if (size > _buf_size) {
		size = _buf_size;
	}
	if (size == 0) {
		return 0;
	}
	if (_partial_point) {
		return move_partial_case_(to_buf, size);
	}
	return move_nonpartial_case_(to_buf, size);
}

void
SpxBuffer::clear_() {
	for (iov_t::iterator it = _buf.begin(); it != _buf.end(); ++it) {
		delete[] static_cast<char*>(it->iov_base);
	}
	_buf.clear();
	_buf_size	   = 0;
	_partial_point = 0;
}

ssize_t
SpxBuffer::write_(int fd) {
	if (_buf_size == 0) {
		return 0;
	}
	_buf.front().iov_base = push_front_addr_();
	ssize_t n_write		  = writev(fd, &_buf.front(), _buf.size());
	_buf.front().iov_base = pull_front_addr_();
	if (n_write <= 0) {
		return n_write;
	}
	delete_size_(n_write);
	return n_write;
}

int
SpxBuffer::get_crlf_line_(std::string& line, size_t str_max_size) {
	size_t size		= 0;
	size_t tmp_size = 0;
	char*  lf_pos;

	if (_buf.empty()) {
		return false;
	}
	if (str_max_size > _buf_size) {
		str_max_size = _buf_size;
	}
	lf_pos = std::find(push_front_addr_(), push_front_addr_() + _buf.front().iov_len, LF);
	lf_pos = lf_pos + 1;
	size   = static_cast<char*>(lf_pos) - static_cast<char*>(push_front_addr_());
	if (lf_pos > push_front_addr_() + _buf.front().iov_len) {
		// LF found in the first buffer.
		if (*(static_cast<char*>(lf_pos) - 2) == CR) {
			line.assign(push_front_addr_(), (lf_pos - 2));
			_partial_point += size;
			return true;
		} else {
			// error case.
			return -1;
		}
	} else {
		// search from the second buffer
		iov_t::iterator it = _buf.begin() + 1;
		while (it != _buf.end() && size < str_max_size) {
			lf_pos	 = std::find(static_cast<char*>(it->iov_base), static_cast<char*>(it->iov_base) + it->iov_len, LF);
			lf_pos	 = lf_pos + 1;
			tmp_size = static_cast<char*>(lf_pos) - static_cast<char*>(it->iov_base);
			size += tmp_size;
			if (lf_pos > static_cast<char*>(it->iov_base) + it->iov_len) {
				// LF found.
				if (*(static_cast<char*>(lf_pos) - 2) == CR) {
					line.assign(push_front_addr_(), push_front_addr_() + _buf.front().iov_len);
					for (iov_t::iterator tmp = _buf.begin() + 1; tmp != it; ++tmp) {
						line.insert(line.end(), static_cast<char*>(tmp->iov_base), static_cast<char*>(tmp->iov_base) + tmp->iov_len);
					}
					line.insert(line.end(), static_cast<char*>(it->iov_base), static_cast<char*>(it->iov_base) + it->iov_len);
					_partial_point = tmp_size;
					delete_size_(size);
					return true;
				} else {
					// error case.
					return -1;
				}
			}
			++it;
		}
		// not found
		return false;
	}
}

size_t
SpxBuffer::buf_size_() {
	return _buf_size;
}

/*
SPX READ BUFFER
*/

SpxReadBuffer::SpxReadBuffer(size_t _rdbuf_buf_size, int _rdbuf_iov_size)
	: SpxBuffer()
	, _rdbuf()
	, _rdbuf_buf_size(_rdbuf_buf_size)
	, _rdbuf_iov_size(_rdbuf_iov_size) {
	// set_empty_buf_();
}

SpxReadBuffer::~SpxReadBuffer() {
	// std::cout << "rdbuf destructor size " << _rdbuf.size() << std::endl;
	while (_rdbuf.size()) {
		delete[] static_cast<char*>(_rdbuf.back().iov_base);
		_rdbuf.pop_back();
	}
}

void
SpxReadBuffer::set_empty_buf_() {
	struct iovec tmp;

	tmp.iov_len = _rdbuf_buf_size;
	while (_rdbuf.size() < _rdbuf_iov_size) {
		tmp.iov_base = new char[_rdbuf_buf_size];
		_rdbuf.push_back(tmp);
	}
}

ssize_t
SpxReadBuffer::read_(int fd) {
	set_empty_buf_();
	// std::cout << "empty_buf size " << _rdbuf.size() << std::endl;
	ssize_t n_read = readv(fd, &_rdbuf.front(), _rdbuf_iov_size);

	if (n_read <= 0) {
		return n_read;
	}
	_buf_size += n_read;
	size_t			div = n_read / _rdbuf_buf_size;
	size_t			mod = n_read % _rdbuf_buf_size;
	iov_t::iterator it	= _rdbuf.begin() + div;
	if (mod != 0) {
		it->iov_len = mod;
		++it;
	}
	_buf.insert(_buf.end(), _rdbuf.begin(), it);
	// std::cout << "buf size " << _buf.size() << std::endl;
	_rdbuf.erase(_rdbuf.begin(), it);

	return n_read;
}