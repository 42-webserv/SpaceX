#include "spx_session_storage.hpp"

std::string
SessionStorage::find_key(const std::string& c_key) const {
	storage_t::const_iterator it = storage_.find(c_key);
	if (it == storage_.end())
		return "";
	return it->first;
}

std::string
SessionStorage::find_value_by_key(const std::string& c_key) const {
	storage_t::const_iterator it = storage_.find(c_key);
	if (it == storage_.end())
		return "";
	return it->second;
}

void
SessionStorage::parse_cookie_header(const std::string& cookie_header) {
	size_t start = 0;
	while (start < cookie_header.size()) {
		size_t end = cookie_header.find(';', start);
		if (end == std::string::npos)
			end = cookie_header.size();
		std::string cookie	  = cookie_header.substr(start, end - start);
		size_t		equal_pos = cookie.find('=');
		size_t		key_pos	  = 0;
		if (equal_pos != std::string::npos) {
			std::string key	  = cookie.substr(key_pos, equal_pos);
			std::string value = cookie.substr(equal_pos + 1);
			if (!key.empty()) // additional valid check for key needed
				storage_[key] = value;
		}
		start = end + 1;
		while (cookie_header[start] == ' ')
			++start;
	}
}
