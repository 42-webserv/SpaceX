#include "spx_session_storage.hpp"

// temp
#include "spx_client_buffer.hpp"

bool
SessionStorage::is_key_exsits(const std::string& c_key) const {
	storage_t::const_iterator it = storage_.find(c_key);
	if (it == storage_.end()) {
		return false;
	}
	return true;
}

session_t&
SessionStorage::find_value_by_key(std::string& c_key) {
	storage_t::iterator it = storage_.find(c_key);
	return it->second;
}
/*
std::string
SessionStorage::find_session_to_string(const std::string& c_key) {
	storage_t::iterator it = storage_.find(c_key);
	if (it == storage_.end())
		return "";
	return it->second.to_string();
}
*/

void
SessionStorage::add_new_session(SessionID id) {
	storage_.insert(session_key_val(id, 0));
}

std::string
SessionStorage::make_hash(uintptr_t& fd) {

	std::time_t t		 = std::time(0);
	uint32_t	hash_key = t;
	uintptr_t	seed	 = fd % 64;

	std::bitset<64> b(hash_key);
	std::bitset<32> a(hash_key);
	b <<= 32;
	a <<= fd;

	std::string table_left	= "0A1B2C3D4E5F6G7H8I9JaKbLcMdNeOf+";
	std::string table_right = "PgQhRiSjTkUlVmWnXoYpZqvrwsxtyuz-";

	std::string hash_str;
	int			i = 31;
	while (i >= 0) {
		if (b.test(i)) {
			hash_str += table_left[i];
		} else {
			hash_str += table_left[31 - i];
		}
		if (a.test(i)) {
			hash_str += table_right[i];
		} else {
			hash_str += table_right[31 - i];
		}
		--i;
	}
	return hash_str;
}

void
SessionStorage::addCount() {
	++count;
}

// this code will moved to client_buf file

void
ResField::setSessionHeader(std::string session_id) {
	headers_.push_back(header("Set-Cookie", "sessionID=" + session_id));
}
