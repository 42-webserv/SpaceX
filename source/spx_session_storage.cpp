#include "spx_session_storage.hpp"
#include <cstdlib>

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
SessionStorage::make_hash(uintptr_t& seed_in) {

	std::time_t		t = std::time(0);
	std::bitset<16> left(t);
	std::bitset<8>	right(seed_in);

	if (left[0]) {
		std::bitset<16> tmp;
		int				i = 15;
		while (i >= 0) {
			tmp[i] = left[15 - i];
			--i;
		}
		left = tmp;
	}

	if (right[0]) {
		std::bitset<8> tmp;
		int			   i = 7;
		while (i >= 0) {
			tmp[i] = right[7 - i];
			--i;
		}
		right = tmp;
	}

	uint32_t rand_seed;
	rand_seed |= left.to_ulong();
	rand_seed <<= 8;
	rand_seed |= right.to_ulong();

	std::srand(rand_seed);
	uint32_t r = std::rand();

	std::bitset<24> total(rand_seed);
	std::string		hash_str;
	int				i = 23;
	while (i >= 0) {
		if (r & (1 << i)) {
			hash_str += "abcdefghijklmnopqrstuvwxyz"[i];
		} else {
			hash_str += "ABCDEFGHIJKLMNOPQRSTUVWXYZ"[i];
		}
		// if (total[i]) {
		// 	hash_str += "abcdefghijklmnopqrstuvwxyz"[i];
		// } else {
		// 	hash_str += "ABCDEFGHIJKLMNOPQRSTUVWXYZ"[i];
		// }
		// if ((1 << i) & r) {
		// 	hash_str += "1^2&3*4(5)"[i % 10];
		// } else {
		// 	hash_str += "0!8@6#7$9%"[i % 10];
		// }
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
