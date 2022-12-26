#include "../../source/spx_core_type.hpp"
#include "spx_core_type.hpp"
// #include "spx_parse_config.hpp"
#include "spx_port_info.hpp"
#include "spx_response_generator.hpp"

const uri_location*
set_uri_location() {
	uri_location_for_copy_stage stage;
	// stage.uri		  = "/test";
	// stage.root			 = "/Users/wchae/webserv/SpaceX/response_dev/unit_test/test";
	stage.autoindex_flag = Kautoindex_on;
	uri_location* res	 = new uri_location(stage);

	return res;
}

void
init_request(std::pair<req_field_t, res_field_t>& p) {
	ReqField request;
	request.file_path_ = "test";
	request.req_type_  = REQ_GET;
	request.uri_loc_   = set_uri_location();
}

void
init_buffer(ClientBuffer& cb) {
	std::pair<req_field_t, res_field_t> one_set;
	cb.req_res_queue_.push(one_set);
}

void
buffer_print(buffer_t buf) {
	for (buffer_t::iterator it = buf.begin(); it != buf.end(); ++it) {
		std::cout << *it;
	}
	std::cout << std::endl;
}

void
test_check(ClientBuffer& cb) {
	res_field_t& response = cb.req_res_queue_.back().second;
	std::cout << "body_fd: " << response.body_fd_ << "\n"
			  << "buf_size: " << response.buf_size_ << "\n"
			  << "file_path: " << response.file_path_ << "\n";
	buffer_print(response.res_buffer_);
}

int
main(void) {
	Response	 res;
	ClientBuffer client_buffer;
	init_buffer(client_buffer);
	res.make_response_header(client_buffer);
}