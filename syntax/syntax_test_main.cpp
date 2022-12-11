#include "syntax.hpp"
#include <iostream>

int
main(int argc, char** argv) {

	if (argc == 2) {
		std::string temp(argv[1]);
		temp.append("\r\n\r\n");
		// std::cout << spx_http_syntax_start_line(temp) << std::endl;
		// std::cout << std::endl;
		// std::cout << spx_http_syntax_header_line(temp) << std::endl;
		{
			uint16_t	chunk_size = 0;
			std::string chunk_ext;
			uint8_t		ext_count = 0;
			status		return_status;
			return_status = spx_chunked_syntax_start_line(temp, chunk_size, chunk_ext, ext_count);
			if (return_status == spx_ok) {
				std::cout << chunk_size << std::endl;
				std::cout << chunk_ext << std::endl;
				std::cout << ext_count << std::endl;
			}
		}
	}

	// d;quality=1.0\r\n
}
