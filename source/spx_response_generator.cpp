#include "spx_response_generator.hpp"

std::string
http_status_str(http_status s) {
	switch (s) {
#define XX(num, name, string) \
	case HTTP_STATUS_##name:  \
		return #string;
		HTTP_STATUS_MAP(XX)
#undef XX
	default:
		return "<unknown>";
	}
}
