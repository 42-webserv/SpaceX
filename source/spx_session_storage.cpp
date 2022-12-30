#include "spx_session_storage.hpp"

// temp
#include "spx_client_buffer.hpp"

bool
SessionStorage::is_key_exsits(const std::string& c_key) const {
	storage_t::const_iterator it = storage_.find(c_key);
	it == storage_.end() ? false : true;
}

Cookie
SessionStorage::find_value_by_key(const std::string& c_key) const {
	storage_t::const_iterator it = storage_.find(c_key);
	if (it == storage_.end())
		return;
	return it->second;
}

std::string
SessionStorage::find_cookie_to_string(const std::string& c_key) {
	storage_t::iterator it = storage_.find(c_key);
	if (it == storage_.end())
		return "";
	return it->second.to_string();
}

void
SessionStorage::add_new_session(SessionID id, Cookie cookie) {
	storage_.insert(session_key_val("val", cookie));
}

void
ResField::setCookieHeader(SessionStorage& storage) {
	Cookie		c;
	std::string tmp_session_id = "TMP_SESSION_VALUE1234";
	c.content.insert(cookie_content("SessionID", tmp_session_id));
	headers_.push_back(header("Set-Cookie", c.to_string()));
	storage.add_new_session(tmp_session_id, c);

	// Server - Session register the session value
}

/*
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
*/
// this will moved to spx_client_buffer.cpp
