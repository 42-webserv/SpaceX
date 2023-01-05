#include "spx_buffer.hpp"

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

void*
SpxBuffer::push_front_addr_() {
	return static_cast<char*>(_buf.front().iov_base) + _partial_point;
}

void*
SpxBuffer::pull_front_addr_() {
	return static_cast<char*>(_buf.front().iov_base) - _partial_point;
}

void
SpxBuffer::delete_size_(size_t size) {
	iov_t::iterator it = _buf.begin();
	if (size < _buf_size) {
		_buf_size -= size;
		while (it != _buf.end() && size >= it->iov_len) {
			delete[] static_cast<char*>(it->iov_base);
			size -= it->iov_len;
			++it;
		}
		_partial_point = size;
		_buf.erase(_buf.begin(), it);
	} else {
		for (; it != _buf.end(); ++it) {
			delete[] static_cast<char*>(it->iov_base);
		}
		_buf.erase(_buf.begin(), _buf.end());
		_buf_size	   = 0;
		_partial_point = 0;
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
	_partial_point = 0;
	del_size	   = it->iov_len;
	to_buf._buf.push_back(new_iov);
	delete[] static_cast<char*>(it->iov_base);
	_buf.erase(_buf.begin());
	move_nonpartial_case_(to_buf, size - del_size);
	return size;
}

size_t
SpxBuffer::move_nonpartial_case_(SpxBuffer& to_buf, size_t size) {
	size_t		 tmp_size = size;
	size_t		 iov_size = 0;
	struct iovec new_iov;

	iov_t::iterator it = _buf.begin();

	while (tmp_size) {
		if (tmp_size < it->iov_len) {
			new_iov.iov_base = new char[tmp_size];
			new_iov.iov_len	 = tmp_size;
			memcpy(new_iov.iov_base, push_front_addr_(), tmp_size);
			_partial_point = tmp_size;
			it->iov_len -= tmp_size;
			tmp_size = 0;
		} else {
			tmp_size -= it->iov_len;
			++iov_size;
			++it;
		}
	}
	to_buf._buf.insert(to_buf._buf.end(), _buf.begin(), it);
	to_buf._buf.push_back(new_iov);
	delete_size_(size);
	return size;
}

size_t
SpxBuffer::move_(SpxBuffer& to_buf, size_t size) {
	if (size > _buf_size) {
		size = _buf_size;
	} else if (size == 0) {
		return 0;
	}
	if (_partial_point) {
		return move_partial_case_(to_buf, size);
	}
	return move_nonpartial_case_(to_buf, size);
}

void
SpxBuffer::clear_() {
	delete_size_(-1);
}

ssize_t
SpxBuffer::write_(int fd) {
	if (_buf_size == 0) {
		return 0;
	}
	_buf.front().iov_base = push_front_addr_();
	ssize_t n_write		  = writev(fd, &_buf.front(), _buf.size());
	if (n_write <= 0) {
		return n_write;
	}
	_buf.front().iov_base = pull_front_addr_();
	delete_size_(n_write);
	return n_write;
}

int
SpxBuffer::get_crlf_line_(std::string& line) {
	size_t size = 0;
	void*  lf_pos;

	lf_pos = std::find(push_front_addr_(), push_front_addr_() + _buf.front().iov_len, LF);
	size   = static_cast<char*>(lf_pos) - static_cast<char*>(push_front_addr_()) + 1;
	if (lf_pos != push_front_addr_() + _buf.front().iov_len) {
		// LF found in first buffer.
		if (*(static_cast<char*>(lf_pos) - 1) == CR) {
			line.assign(push_front_addr_(), (lf_pos - 1));
			_partial_point += size;
			return true;
		} else {
			// error case.
			return -1;
		}
	} else {
		// search next buffer
		iov_t::iterator it = _buf.begin() + 1;
		while (it != _buf.end()) {
			lf_pos = std::find(it->iov_base, it->iov_base + it->iov_len, LF);
			if (lf_pos != it->iov_base + it->iov_len) {
				// LF found.
				if (*(static_cast<char*>(lf_pos) - 1) == CR) {
					line.assign(push_front_addr_(), (lf_pos - 1));
					_partial_point += size;
					return true;
				} else {
					// error case.
					return -1;
				}
			}
		}
	}
}

/*
SPX READ BUFFER
*/

SpxReadBuffer::SpxReadBuffer(size_t _rdbuf_iov_size, size_t _rdbuf_buf_size)
	: SpxBuffer()
	, _rdbuf()
	, _rdbuf_iov_size(_rdbuf_iov_size)
	, _rdbuf_buf_size(_rdbuf_buf_size) {
	// set_empty_buf_();
}

SpxReadBuffer::~SpxReadBuffer() {
	while (_rdbuf.size()) {
		delete[] static_cast<char*>(_rdbuf.back().iov_base);
		_rdbuf.pop_back();
	}
}

size_t
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
	ssize_t n_read = readv(fd, &_rdbuf.front(), _rdbuf_iov_size);

	if (n_read <= 0) {
		return n_read;
	}
	_buf_size += n_read;
	size_t div = n_read / _rdbuf_buf_size;
	size_t mod = n_read % _rdbuf_buf_size;
	if (mod != 0) {
		_rdbuf[div].iov_len = mod;
		++div;
	}
	_buf.insert(_buf.end(), _rdbuf.begin(), _rdbuf.begin() + div);
	_rdbuf.erase(_rdbuf.begin(), _rdbuf.begin() + div);

	return n_read;
}