#include "syntax.hpp"
#include <iostream>

int
main(int argc, char** argv) {

	if (argc == 2 || argc == 3) {
		// ./a.out "d;quality=1.0;test" "hello, world\!"
		// ./a.out "000;quality=1.0;test" "hello, world\!"
		std::string temp(argv[1]);
		temp.append("\r\n");
		// std::cout << spx_http_syntax_start_line(temp) << std::endl;
		// std::cout << std::endl;
		// std::cout << spx_http_syntax_header_line(temp) << std::endl;
		uint16_t	chunk_size = 0;
		std::string chunk_ext;
		uint8_t		ext_count = 0;
		status		return_status;
		return_status = spx_chunked_syntax_start_line(temp, chunk_size, chunk_ext, ext_count);
		if (return_status == spx_ok) {
			std::cout << chunk_size << std::endl;
			std::cout << chunk_ext << std::endl;
			std::cout << ext_count << std::endl;

			{
				std::string temp2(argv[2]);
				temp2.append("\r\n");
				std::string data_store;
				std::string trailer;
				uint8_t		trailer_count = 0;
				status		return_status;
				return_status = spx_chunked_syntax_data_line(temp2, chunk_size, data_store, trailer, trailer_count);
				if (return_status == spx_ok) {
					std::cout << data_store << std::endl;
					std::cout << trailer << std::endl;
					std::cout << trailer_count << std::endl;
				} else if (return_status == spx_need_more) {
					std::cout << "need more" << std::endl;
				} else {
					std::cout << "error" << std::endl;
				}
			}
		}
	}
}
