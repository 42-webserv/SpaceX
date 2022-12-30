#include <fcntl.h>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <sys/event.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

int
main(void) {

	int pid;
	int status;
	int write_to_cgi[2];
	int read_from_cgi[2];
	pipe(write_to_cgi);
	pipe(read_from_cgi);

	std::string str = "num1=4&num2=8";

	pid = fork();

	if (pid == 0) {

		dup2(write_to_cgi[0], STDIN_FILENO);
		close(write_to_cgi[1]);
		close(read_from_cgi[0]);
		dup2(read_from_cgi[1], STDOUT_FILENO);

		// char filename[100] = "/Library/Frameworks/Python.framework/Versions/3.10/bin/python3";
		char filename[100] = "./cgi_bin/42_cgi_tester";
		// char script[100] = "upload_cgi.py";
		char script[100] = "./YoupiBanane/youpi.bla";
		// char  script[100] = "./cgi_bin/upload_cgi.py";
		char* argv[] = { filename, script, NULL };

		char* envp[] = {
			"REQUEST_METHOD=POST",
			"QUERY_STRING=num1=4&num2=1777",
			"CONTENT_TYPE=application/x-www-form-urlencoded",
			"CONTENT_LENGTH=13",
			"SERVER_PROTOCOL=HTTP/1.1",
			// "PATH_INFO=/directory/youpi.bla",
			"REQUEST_URI=/directory///youpi.bla",
			"PATH_INFO=/directory///youpi.bla",
			"SCRIPT_NAME=/directory/bla",
			"HTTP_HOST=localhost:8080",
			"GATEWAY_INTERFACE=CGI/1.1",
			"REMOTE_ADDR=127.0.0.1",
			"SERVER_SOFTWARE=SPX/1.0",
			"SERVER_PROTOCOL=HTTP/1.1",
			NULL
		};

		execve(filename, argv, envp);
		exit(1);
	} else {
		close(write_to_cgi[0]);
		write(write_to_cgi[1], str.c_str(), str.length());
		close(write_to_cgi[1]);
		close(read_from_cgi[1]);
		std::cout << "parent----------" << std::endl;
		while (waitpid(pid, NULL, 0) != -1) {
			std::cout << "waitpid" << std::endl;
		}
		std::cout << "end of while----------" << std::endl;
		char temp[1024];

		ssize_t i = read(read_from_cgi[0], temp, 1024);
		while (i > 0) {
			temp[i] = '\0';
			printf("%s", temp);
			i = read(read_from_cgi[0], temp, 1024);
		}
		close(read_from_cgi[0]);
	}
}
