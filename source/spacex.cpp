#include "spacex.hpp"
#include "spx_core_type.hpp"
#include "spx_port_info.hpp"
#include <vector>

#include "spx_client_buffer.hpp"
#include "spx_kqueue_module.hpp"

#include "spx_autoindex_generator.hpp"
// #include "spx_response_generator.hpp"

namespace {

#ifdef LEAK
	void
	ft_handler_leak_() {
		system("leaks spacex");
	}
#endif

	inline total_port_server_map_p
	config_file_open_(int argc, char const* argv[], std::string const& cur_path) {
		std::fstream file;
		switch (argc) {
		case 1:
			file.open("./config/default.conf", std::ios_base::in);
			break;
		case 2:
			file.open(argv[1], std::ios_base::in);
			break;
		default:
			error_exit_msg("usage: ./spacex [config_file]");
		}
		if (file.is_open() == false) {
			error_exit_msg_perror("file open error");
		}
		std::stringstream ss;
		ss << file.rdbuf();
		if (ss.fail()) {
			file.close();
			error_exit_msg_perror("file read error");
		}
		file.close();
#ifdef MAIN_DEBUG
		std::cout << ss.str() << std::endl;
#endif
		total_port_server_map_p temp_config;
		if (spx_config_syntax_checker(ss.str(), temp_config, cur_path) != spx_ok) {
			error_exit_msg("config syntax error");
		}
		return temp_config;
	}

	inline void
	get_current_path_(std::string& cur_path) {
		char buf[8192];
		if (getcwd(buf, sizeof(buf)) == NULL) {
			error_exit_msg_perror("getcwd error");
		}
		cur_path = buf;
	}

} // namespace

inline void
main_info_t::port_info_print_(void) {
	uint32_t i = 3;
		std::cout << "------------------------------------" << std::endl;
	while (i < this->socket_size) {
		// std::cout << "listen_sd: " << port_info[i].listen_sd << std::endl;
		// std::cout << "my_port: " << port_info[i].my_port << std::endl;
		// std::cout << "default_server info print: " << std::endl;
		// port_info[i].my_port_default_server.print_();
		for (server_map_p::const_iterator it2 = port_info[i].my_port_map.begin(); it2 != port_info[i].my_port_map.end(); ++it2) {
			// if ((it2->second.default_server_flag == Kother_server)) {
				std::cout << "port: " << COLOR_GREEN << it2->second.port << COLOR_RESET;
				std::cout << " | name: " << COLOR_GREEN << it2->second.server_name << COLOR_RESET;
				if (it2->second.default_server_flag == Kdefault_server){
					std::cout << COLOR_RED << " <---- default server" << COLOR_RESET << std::endl;
				}
				else{

					std::cout << std::endl;
				}
				// it2->second.print_();
			// }
		}
		++i;
	}
		std::cout << "------------------------------------" << std::endl;
}

#ifdef SPACE_RESPONSE_TEST
void
buffer_print_header(buffer_t buf) {
	size_t cnt	= 0;
	bool   flag = false;
	for (buffer_t::iterator it = buf.begin(); it != buf.end(); ++it) {
		cnt++;
		if (*it == '\n' && *(it + 1) == '\r')
			flag = true;
		std::cout << *it;
		if (flag) {
			flag = false;
			std::cout << "====header END====\n";
			std::cout << "header_size = " << cnt << std::endl;
			cnt = 0;
		}
	}
	std::cout << "\nbody_size = " << cnt << std::endl;
	// write(1, &buf[0], 56);
}
void
buffer_print(buffer_t buf) {
	for (buffer_t::iterator it = buf.begin(); it != buf.end(); ++it) {
		std::cout << *it;
	}
	std::cout << "\nbuffer size = " << buf.size() << std::endl;
}
void
test_check(ClientBuffer& cb) {
	res_field_t& response = cb.req_res_queue_.back().second;
	std::cout << "body_fd: " << response.body_fd_ << "\n"
			  << "buf_size: " << response.buf_size_ << "\n";
	buffer_print_header(response.res_buffer_);
	// buffer_print(response.res_buffer_);
}
#endif

int
main(int argc, char const* argv[]) {
#ifdef LEAK
	atexit(ft_handler_leak_);
#endif
	if (argc <= 2) {
		std::string cur_dir;
		get_current_path_(cur_dir);

		total_port_server_map_p config_info = config_file_open_(argc, argv, cur_dir);
		spx_log_("config file open success");

		main_info_t spx;
		spx.socket_size = 0;

		socket_init_and_build_port_info(config_info, spx.port_info, spx.socket_size);
		spx_log_("socket per port_info success");
		spx.port_info_print_();

#ifdef YOMA_SEARCH_DEBUG
		// how to use handler function
		std::cout << "-------------------------------" << std::endl;
		server_info_t const& temp_ = spx.port_info[4].search_server_config_("aoriestnaoiresnt");
		spx_log_(temp_.get_error_page_path_(404));
		spx_log_(temp_.get_error_page_path_(303));
		std::cout << "server_name: " << temp_.server_name << std::endl;
		uri_resolved_t resol_uri_;
		// uri_location_t const* loc_ = temp_.get_uri_location_t_("/upload/blah/%00somepath/adder?arsiotenrstn#rsiten", resol_uri_);
		uri_location_t const* loc_ = temp_.get_uri_location_t_("/upload", resol_uri_);
		std::cout << "location: " << loc_ << std::endl;
		resol_uri_.print_();
#endif

#ifdef YOMA_SEARCH_DEBUG
		// while (1) {
		// }
#else
		kqueue_module(spx.port_info);
#endif
#ifdef SPACE_RESPONSE_TEST
		ResField							res;
		ClientBuffer						c;
		std::pair<req_field_t, res_field_t> one_set;
		ReqField&							req = one_set.first;

		uri_location_for_copy_stage stage;
		stage.autoindex_flag = Kautoindex_on;
		stage.uri			 = "/Users/spacechae/Desktop/webserv/SpaceX";
		stage.root			 = "/Users/spacechae/Desktop/webserv/SpaceX";

		server_info_for_copy_stage serv_info_stage;

		uri_location* uri_loc_tmp	  = new uri_location(stage);
		server_info*  server_info_tmp = new server_info(serv_info_stage);

		req.uri_loc_   = uri_loc_tmp;
		req.serv_info_ = server_info_tmp;

		req.req_type_  = REQ_GET;
		req.file_path_ = "/Users/spacechae/Desktop/webserv/SpaceX";
		c.req_res_queue_.push(one_set);
		res.make_response_header(c);
		test_check(c);

#endif
	} else {
		error_exit_msg("usage: ./spacex [config_file]");
	}
	return spx_ok;
}
