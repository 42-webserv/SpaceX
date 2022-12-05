#include "syntax.hpp"

int main(int argc, char **argv){

	if (argc == 2)
	{
		char *temp;
		int len = strlen(argv[1]);

		temp = (char *)malloc(sizeof(char) * (len + 3));
		memcpy(temp, argv[1], len);
		temp[len] = '\r';
		temp[len + 1] = '\n';
		temp[len + 2] = '\0';
		std::cout << spx_http_syntax_start_line_request(temp) << std::endl;
		free (temp);
	}
}
