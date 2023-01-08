#include "spx_buffer.hpp"
#include <iostream>

SpxBuffer::SpxBuffer()
	: _buf()
	, _buf_size(0)
	, _partial_point(0) {
	// std::cout << "constructor buf size " << _buf.size() << std::endl;
}

SpxBuffer::~SpxBuffer() {
	std::cout << "destructor buf size " << _buf.size() << std::endl;
	for (iov_t::iterator it = _buf.begin(); it != _buf.end(); ++it) {
		delete[] static_cast<char*>(it->iov_base);
	}
}

char*
SpxBuffer::front_end_() {
	return static_cast<char*>(push_front_addr_()) + _buf.front().iov_len;
}

char*
SpxBuffer::iov_base_(struct iovec& iov) {
	return static_cast<char*>(iov.iov_base);
}

char*
SpxBuffer::iov_end_addr_(struct iovec& iov) {
	return static_cast<char*>(iov.iov_base) + iov.iov_len;
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
		} else {
			_partial_point = size;
			_buf.erase(_buf.begin(), it);
		}
	} else {
		clear_();
	}
}

void
SpxBuffer::delete_size_for_move_(size_t size) {
	iov_t::iterator it = _buf.begin();
	if (size < _buf_size) {
		_buf_size -= size;
		while (size >= it->iov_len) {
			size -= it->iov_len;
			++it;
		}
		it->iov_len -= size;
		if (it == _buf.begin()) {
			_partial_point += size;
			return;
		} else {
			_partial_point = size;
			_buf.erase(_buf.begin(), it);
		}
	} else {
		_buf.clear();
		_buf_size	   = 0;
		_partial_point = 0;
	}
}

size_t
SpxBuffer::move_partial_case_(SpxBuffer& to_buf, size_t size) {
	// std::cout << "partial" << std::endl;
	size_t			del_size;
	struct iovec	new_iov;
	iov_t::iterator it = _buf.begin();

	if (size < it->iov_len) {
		new_iov.iov_base = new char[size];
		new_iov.iov_len	 = size;
		memcpy(new_iov.iov_base, push_front_addr_(), size);
		delete_size_for_move_(size);
		to_buf._buf.push_back(new_iov);
		to_buf._buf_size += size;
		return size;
	}

	new_iov.iov_base = new char[it->iov_len];
	new_iov.iov_len	 = it->iov_len;
	memcpy(new_iov.iov_base, push_front_addr_(), it->iov_len);
	del_size = it->iov_len;
	to_buf._buf.push_back(new_iov);
	to_buf._buf_size += del_size;
	delete_size_(del_size);
	if (size - del_size) {
		move_nonpartial_case_(to_buf, size - del_size);
	}
	return size;
}

size_t
SpxBuffer::move_nonpartial_case_(SpxBuffer& to_buf, size_t size) {
	// std::cout << "nonpartial" << std::endl;
	size_t			tmp_size = size;
	struct iovec	new_iov;
	iov_t::iterator it = _buf.begin();

	while (it != _buf.end() && tmp_size >= it->iov_len) {
		tmp_size -= it->iov_len;
		++it;
	}
	to_buf._buf.insert(to_buf._buf.end(), _buf.begin(), it);
	if (tmp_size) {
		new_iov.iov_base = new char[tmp_size];
		new_iov.iov_len	 = tmp_size;
		memcpy(new_iov.iov_base, it->iov_base, tmp_size);
		to_buf._buf.push_back(new_iov);
	}
	to_buf._buf_size += size;
	delete_size_for_move_(size);
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

size_t
SpxBuffer::find_pos_(char c, size_t max) {
	size_t pos;
	size_t tmp;

	pos = std::find(push_front_addr_(), front_end_(), c) - push_front_addr_();
	if (pos != _buf.front().iov_len) {
		return pos;
	}
	iov_t::iterator it = _buf.begin() + 1;
	while (it != _buf.end()) {
		tmp = std::find(iov_base_(*it), iov_end_addr_(*it), c) - iov_base_(*it);
		pos += tmp;
		if (tmp != it->iov_len) {
			return pos;
		}
		if (pos > max) {
			return -1;
		}
		++it;
	}
	return -1;
}

char
SpxBuffer::pos_val_(size_t pos) {
	if (pos >= _buf_size) {
		return 0;
	}

	if (_buf.front().iov_len > pos) {
		return *(push_front_addr_() + pos);
	}
	pos -= _buf.front().iov_len;

	iov_t::iterator it = _buf.begin() + 1;
	while (it->iov_len <= pos) {
		pos -= it->iov_len;
		++it;
	}
	return *(static_cast<char*>(it->iov_base) + pos);
}

void
SpxBuffer::get_str_(std::string& str, size_t size) {
	if (size >= _buf_size) {
		size = _buf_size;
	}

	if (_buf.front().iov_len >= size) {
		str.insert(str.end(), push_front_addr_(), push_front_addr_() + size);
		delete_size_(size);
		return;
	}

	str.insert(str.end(), push_front_addr_(), front_end_());
	size_t tmp_size = size;
	tmp_size -= _buf.front().iov_len;

	iov_t::iterator it = _buf.begin() + 1;

	while (tmp_size > it->iov_len) {
		str.insert(str.end(), iov_base_(*it), iov_end_addr_(*it));
		tmp_size -= it->iov_len;
		++it;
	}
	if (it != _buf.end()) {
		str.insert(str.end(), iov_base_(*it), iov_base_(*it) + tmp_size);
	}
	delete_size_(size);
}

int
SpxBuffer::get_crlf_line_(std::string& line) {
	size_t lf_pos;
	size_t tmp_size;

	if (_buf.empty()) {
		return false;
	}
	lf_pos = find_pos_(LF, 8 * 1024);
	if (lf_pos == -1) {
		return false;
	} else if (lf_pos == 0) {
		return -1;
	}
	if (pos_val_(lf_pos - 1) == CR) {
		get_str_(line, lf_pos - 1);
		delete_size_(2);
		return true;
	} else {
		return -1;
	}
}

size_t
SpxBuffer::buf_size_() {
	return _buf_size;
}

/*
SPX READ BUFFER
*/

SpxReadBuffer::SpxReadBuffer(size_t _rdbuf_buf_size, int _rdbuf_iov_vec_size)
	: SpxBuffer()
	, _rdbuf()
	, _rdbuf_buf_size(_rdbuf_buf_size)
	, _rdbuf_iov_vec_size(_rdbuf_iov_vec_size) {
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
	while (_rdbuf.size() < _rdbuf_iov_vec_size) {
		tmp.iov_base = new char[_rdbuf_buf_size];
		_rdbuf.push_back(tmp);
	}
}

ssize_t
SpxReadBuffer::read_(int fd) {
	set_empty_buf_();
	// std::cout << "empty_buf size " << _rdbuf.size() << std::endl;
	ssize_t n_read = readv(fd, &_rdbuf.front(), _rdbuf_iov_vec_size);

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