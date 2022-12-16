#include <iostream>
#include <sstream>
#include <string>

int
main(void) {
	std::stringstream stream;
	std::string		  asdf = "asdf";
	char*			  tmp = "zzzz";
	char			  buf[100];
	// std::cout << buf;
	strcpy(buf, tmp);
	buf[1] = 0;
	stream << asdf << buf;
	std::cout << stream.str()[7] << std::endl;
}
