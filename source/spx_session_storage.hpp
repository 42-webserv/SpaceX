#pragma once
#include "spacex.hpp"

class SessionStorage {
	typedef std::map<std::string, std::string> storage_t;
	storage_t								   storage_;

public:
	SessionStorage() { }
	~SessionStorage() { }

	void
				parse_cookie_header(const std::string& cookie_header);
	std::string find_key(const std::string& c_key) const;
	std::string find_value_by_key(const std::string& c_key) const;
	bool		is_key_exsits() const;
};
