#include "spx_buffer.hpp"
#include <iostream>

SpxBuffer::SpxBuffer()
	: _buf()
	, _buf_size(0)
	, _partial_point(0) {
}

SpxBuffer::~SpxBuffer() {
	while (_buf.size()) {
		delete[] static_cast<char*>(_buf.back().iov_base);
		_buf.pop_back();
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

size_t
SpxBuffer::delete_size_(size_t size) {
	iov_t::iterator it		 = _buf.begin();
	size_t			tmp_size = size;
	if (tmp_size < _buf_size) {
		_buf_size -= tmp_size;
		while (tmp_size >= it->iov_len) {
			delete[] static_cast<char*>(it->iov_base);
			tmp_size -= it->iov_len;
			++it;
		}
		it->iov_len -= tmp_size;
		if (it == _buf.begin()) {
			_partial_point += tmp_size;
		} else {
			_partial_point = tmp_size;
			_buf.erase(_buf.begin(), it);
		}
		return size;
	} else {
		size = _buf_size;
		clear_();
		return size;
	}
}

size_t
SpxBuffer::delete_size_for_move_(size_t size) {
	iov_t::iterator it		 = _buf.begin();
	size_t			tmp_size = size;
	if (tmp_size < _buf_size) {
		_buf_size -= tmp_size;
		while (tmp_size >= it->iov_len) {
			tmp_size -= it->iov_len;
			++it;
		}
		it->iov_len -= tmp_size;
		if (it == _buf.begin()) {
			_partial_point += tmp_size;
		} else {
			_partial_point = tmp_size;
			_buf.erase(_buf.begin(), it);
		}
		return size;
	} else {
		size = _buf_size;
		_buf.clear();
		_buf_size	   = 0;
		_partial_point = 0;
		return size;
	}
}

size_t
SpxBuffer::move_partial_case_(SpxBuffer& to_buf, size_t size) {
	size_t		 del_size = _buf.front().iov_len;
	struct iovec new_iov;

	if (size < del_size) {
		new_iov.iov_base = new char[size];
		new_iov.iov_len	 = size;
		memcpy(new_iov.iov_base, push_front_addr_(), size);
		delete_size_for_move_(size);
		to_buf._buf.push_back(new_iov);
		to_buf._buf_size += size;
		return size;
	}

	new_iov.iov_base = new char[del_size];
	new_iov.iov_len	 = del_size;
	memcpy(new_iov.iov_base, push_front_addr_(), del_size);
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
	while (_buf.size()) {
		delete[] static_cast<char*>(_buf.back().iov_base);
		_buf.pop_back();
	}
	_buf_size	   = 0;
	_partial_point = 0;
}

ssize_t
SpxBuffer::write_(int fd) {
	if (_buf_size == 0) {
		return 0;
	}
	_buf.front().iov_base = push_front_addr_();
	ssize_t n_write		  = writev(fd, &_buf.front(), std::min(_buf.size(), (size_t)IOV_MAX));
	_buf.front().iov_base = pull_front_addr_();
	if (n_write <= 0) {
		return n_write;
	}
	delete_size_(n_write);
	return n_write;
}

ssize_t
SpxBuffer::write_debug_(int fd) {
	if (_buf_size == 0) {
		return 0;
	}
	_buf.front().iov_base = push_front_addr_();
	ssize_t n_write		  = writev(fd, &_buf.front(), std::min(_buf.size(), (size_t)IOV_MAX));
	_buf.front().iov_base = pull_front_addr_();
	if (n_write <= 0) {
		return n_write;
	}
	return n_write;
}

void
SpxBuffer::add_str(const std::string& str) {
	struct iovec tmp;

	tmp.iov_base = new char[str.size()];
	memcpy(tmp.iov_base, str.c_str(), str.size());
	tmp.iov_len = str.size();
	_buf_size += str.size();
	_buf.push_back(tmp);
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
		if (pos > max) {
			return -1;
		} else if (tmp != it->iov_len) {
			return pos;
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

void
SpxBuffer::get_str_cpy_(std::string& str, size_t size) {
	if (size >= _buf_size) {
		size = _buf_size;
	}

	if (_buf.front().iov_len >= size) {
		str.insert(str.end(), push_front_addr_(), push_front_addr_() + size);
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
}

int
SpxBuffer::get_crlf_line_(std::string& line, size_t size) {
	size_t lf_pos;

	if (_buf.empty()) {
		return false;
	}
	lf_pos = find_pos_(LF, size);
	if (lf_pos == SIZE_T_MAX) {
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

int
SpxBuffer::get_crlf_cpy_line_(std::string& line, size_t size) {
	size_t lf_pos;

	if (_buf.empty()) {
		return false;
	}
	lf_pos = find_pos_(LF, size);
	if (lf_pos == SIZE_T_MAX) {
		return false;
	} else if (lf_pos == 0) {
		return -1;
	}
	if (pos_val_(lf_pos - 1) == CR) {
		get_str_cpy_(line, lf_pos - 1);
		return true;
	} else {
		return -1;
	}
}

int
SpxBuffer::get_lf_line_(std::string& line, size_t size) {
	size_t lf_pos;

	if (_buf.empty()) {
		return false;
	}
	lf_pos = find_pos_(LF, size);
	if (lf_pos == SIZE_T_MAX) {
		return false;
	} else if (lf_pos == 0) {
		return -1;
	}
	if (pos_val_(lf_pos - 1) == CR) {
		get_str_(line, lf_pos - 1);
		delete_size_(2);
	} else {
		get_str_(line, lf_pos);
		delete_size_(1);
	}
	return true;
}

size_t&
SpxBuffer::buf_size_() {
	return _buf_size;
}

SpxBuffer::iov_t&
SpxBuffer::get_buf_() {
	return _buf;
}

size_t
SpxBuffer::get_partial_point_() {
	return _partial_point;
}

/*
SPX READ BUFFER
*/

SpxReadBuffer::SpxReadBuffer(size_t _rdbuf_buf_size, int _rdbuf_iov_vec_size)
	: _rdbuf()
	, _rdbuf_buf_size(_rdbuf_buf_size)
	, _rdbuf_iov_vec_size(_rdbuf_iov_vec_size) {
	if (_rdbuf_buf_size == 0 || _rdbuf_iov_vec_size == 0) {
		throw(std::exception());
	}
}

SpxReadBuffer::~SpxReadBuffer() {
	while (_rdbuf.size()) {
		delete[] static_cast<char*>(_rdbuf.back().iov_base);
		_rdbuf.pop_back();
	}
}

void
SpxReadBuffer::set_empty_buf_() {
	struct iovec tmp;

	while (_rdbuf.size() < static_cast<size_t>(_rdbuf_iov_vec_size)) {
		tmp.iov_base = new char[_rdbuf_buf_size];
		tmp.iov_len	 = _rdbuf_buf_size;
		_rdbuf.push_back(tmp);
	}
}

ssize_t
SpxReadBuffer::read_(int fd, SpxBuffer& buf) {
	set_empty_buf_();
	ssize_t n_read = readv(fd, &_rdbuf.front(), _rdbuf.size());

	if (n_read <= 0) {
		return n_read;
	}
	buf.buf_size_() += n_read;
	size_t			div = n_read / _rdbuf_buf_size;
	size_t			mod = n_read % _rdbuf_buf_size;
	iov_t::iterator it	= _rdbuf.begin() + div;
	if (mod != 0) {
		it->iov_len = mod;
		++it;
	}
	buf.get_buf_().insert(buf.get_buf_().end(), _rdbuf.begin(), it);
	_rdbuf.erase(_rdbuf.begin(), it);

	return n_read;
}
