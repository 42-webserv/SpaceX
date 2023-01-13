#include "spx_session_storage.hpp"
#include "spx_client.hpp"
#include <sys/time.h>

SessionStorage::SessionStorage() { }
SessionStorage::~SessionStorage() { }

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

void
SessionStorage::add_new_session(SessionID id) {
	storage_.insert(session_key_val(id, 0));
}

std::string
SessionStorage::generate_session_id(uintptr_t& seed_in) {
	struct timeval time;
	gettimeofday(&time, NULL);

	uint32_t t = time.tv_usec;
	std::srand(t);

	uint32_t r = std::rand();

	std::bitset<24> rand_char(r);
	std::bitset<24> time_char(t);

	std::string hash_str;

	for (int i = 0; i < 6; ++i) {
		if (seed_in & 1) {
			hash_str += "abcdefghijklmnop"[(time_char.to_ulong() & 0xf)];
			hash_str += "ABCDEFGHIJKLMNOP"[(rand_char.to_ulong() & 0xf)];
		} else {
			hash_str += "ABCDEFGHIJKLMNOP"[(time_char.to_ulong() & 0xf)];
			hash_str += "abcdefghijhlmnop"[(rand_char.to_ulong() & 0xf)];
		}
		if (seed_in & 8) {
			hash_str += "qrstuvwxyz12345+"[(time_char.to_ulong() & 0xf)];
			hash_str += "QRSTUVWXYZ67890-"[(rand_char.to_ulong() & 0xf)];
		} else {
			hash_str += "QRSTUVWXYZ67890-"[(time_char.to_ulong() & 0xf)];
			hash_str += "qrstuvwxyz12345+"[(rand_char.to_ulong() & 0xf)];
		}
		time_char >>= 4;
		rand_char >>= 4;
	}
	return hash_str;
}

void
SessionStorage::session_cleaner() {
	for (storage_t::iterator it = storage_.begin(); it != storage_.end();) {
		if ((*it).second.valid_time_ < std::time(NULL)) {
			std::string session_id = (*it++).first;
			storage_.erase(session_id);
		} else {
			++it;
		}
	}
}
