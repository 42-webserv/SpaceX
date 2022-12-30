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
	// if (it == storage_.end())
	// return ;
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
SessionStorage::add_new_session(SessionID id, t_cookie cookie) {
	storage_.insert(session_key_val(id, cookie));
}

// this code will moved to client_buf file

void
ResField::setCookieHeader(SessionStorage& storage) {
	Cookie c;

	// if(this->)
	std::string session_id = "TMP_SESSION_VALUE1234";
	c.content.insert(cookie_content("SessionID", session_id));
	headers_.push_back(header("Set-Cookie", c.to_string()));

	// Server - Session register the session value
	storage.add_new_session(session_id, c);
}

void
ClientBuffer::find_cookie(SessionStorage& storage) {
	req_field_t& req = this->req_res_queue_.front().first;
	// req.storage.is_key_exsits()
}
