#include "syntax_chunked.hpp"
#include "syntax_request.hpp"
#include <iostream>
#include <vector>

int
main(int argc, char** argv) {

	if (argc == 2 || argc == 3 || argc == 4) {
		std::string temp(argv[1]);

		// {
		// 	std::cout << spx_http_syntax_start_line(temp) << std::endl;
		// 	std::cout << std::endl;
		// 	std::cout << spx_http_syntax_header_line(temp) << std::endl;
		// }

		{
			temp.append("\r\n");
			// ./a.out "d;quality=1.0;test" "hello, world\!"
			// ./a.out "000;quality=1.0;test" "hello, world\!"
			// ./a.out "0;rstrts=rst;rstrst;     15115      " "Expires: never"  "Content-MD5: f4a5c16584f03d90"
			uint16_t chunk_size = 0;

			std::map<std::string, std::string> chunk_ext;
			uint8_t							   ext_count = 0;
			status							   return_status;
			return_status = spx_chunked_syntax_start_line(temp, chunk_size, chunk_ext);
			if (return_status == spx_ok) {
				std::cout << chunk_size << std::endl;
				std::map<std::string, std::string>::iterator it = chunk_ext.begin();
				while (it != chunk_ext.end()) {
					std::cout << it->first << " : " << it->second << std::endl;
					++it;
				}
				std::cout << ext_count << std::endl;

				{
					std::string temp2(argv[2]);
					std::string temp3(argv[3]);
					temp2.append("\r\n");
					temp3.append("\r\n\r\n");
					temp2.append(temp3);
					std::vector<char>				   data_store;
					std::map<std::string, std::string> trailer;
					status							   return_status;
					return_status = spx_chunked_syntax_data_line(temp2, chunk_size, data_store, trailer);
					if (return_status == spx_ok) {
						std::vector<char>::iterator it = data_store.begin();
						while (it != data_store.end()) {
							std::cout << *it;
							++it;
						}
						std::cout << std::endl;
						std::map<std::string, std::string>::iterator it2 = trailer.begin();
						while (it2 != trailer.end()) {
							std::cout << it2->first << " : " << it2->second << std::endl;
							++it2;
						}
					} else if (return_status == spx_need_more) {
						std::cout << "need more" << std::endl;
					} else {
						std::cout << "error" << std::endl;
					}
				}
			}
		}
	}
}
