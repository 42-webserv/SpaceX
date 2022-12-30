#pragma once
#include "spacex.hpp"

typedef typename std::map<std::string, std::string>	 key_val_t;
typedef typename std::pair<std::string, std::string> cookie_content;

typedef struct Cookie {
	key_val_t content;

	// TODO : TicketTime - currentTime > limit => delete session
	// time_t	  valid_time;

	std::string
	to_string() {
		std::stringstream ss;
		for (key_val_t::iterator it = content.begin(); it != content.end();
			 ++it) {
			ss << it->first << "=" << it->second << "; ";
		}
		std::string result = ss.str();
		size_t		pos	   = result.find_last_of(";");
		return result.substr(0, pos);
	}

	void
	parse_cookie_header(const std::string& cookie_header) {
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
					content[key] = value;
			}
			start = end + 1;
			while (cookie_header[start] == ' ')
				++start;
		}
	}

} t_cookie;

class SessionStorage {
	typedef typename std::string					SessionID;
	typedef typename std::string					SessionValue;
	typedef typename std::pair<SessionID, t_cookie> session_key_val;

	typedef std::map<SessionID, t_cookie> storage_t;
	storage_t							  storage_;

public:
	SessionStorage() { }
	~SessionStorage() { }

	// void
	//  parse_cookie_header(const std::string& cookie_header);
	bool		is_key_exsits(const std::string& c_key) const;
	t_cookie	find_value_by_key(const std::string& c_key) const;
	std::string find_cookie_to_string(const std::string& c_key);
	void		add_new_session(SessionID id, t_cookie cookie);
};
