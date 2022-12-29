#include <fcntl.h>
#include <iostream>
#include <map>
#include <string>
#include <sys/event.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
#include <sstream>


int
main(void) {

	int		pid;
	int		status;
	int		write_to_cgi[2];
	int		read_from_cgi[2];
	pipe(write_to_cgi);
	pipe(read_from_cgi);

	// std::stringstream ss;
	// ss << "-----------------------------735323031399963166993862150\r\n";
	// ss << "Content-Disposition: form-data; name=\"text1\"\r\n";
	// ss << "\r\n";
	// ss << "text default\r\n";
	// ss << "-----------------------------735323031399963166993862150\r\n";
	// ss << "Content-Disposition: form-data; name=\"text2\"\r\n";
	// ss << "\r\n";
	// ss << "aωb\r\n";
	// ss << "-----------------------------735323031399963166993862150\r\n";
	// ss << "Content-Disposition: form-data; name=\"file1\"; filename=\"a.txt\"\r\n";
	// ss << "Content-Type: text/plain\r\n";
	// ss << "\r\n";
	// ss << "Content of a.txt.\r\n";
	// ss << "\r\n";
	// ss << "-----------------------------735323031399963166993862150\r\n";
	// ss << "Content-Disposition: form-data; name=\"file2\"; filename=\"a.html\"\r\n";
	// ss << "Content-Type: text/html\r\n";
	// ss << "\r\n";
	// ss << "<!DOCTYPE html><title>Content of a.html.</title>\r\n";
	// ss << "\r\n";
	// ss << "-----------------------------735323031399963166993862150\r\n";
	// ss << "Content-Disposition: form-data; name=\"file3\"; filename=\"binary\"\r\n";
	// ss << "Content-Type: application/octet-stream\r\n";
	// ss << "\r\n";
	// ss << "aωb\r\n";
	// ss << "-----------------------------735323031399963166993862150--\r\n";
	// std::string str = ss.str();
	// std::cout << str.length() << std::endl;
	std::string str = "num1=1&num2=2";

	pid = fork();

	if (pid == 0) {


		dup2(write_to_cgi[0], STDIN_FILENO);
		dup2(read_from_cgi[1], STDOUT_FILENO);
		close(write_to_cgi[1]);
		close(read_from_cgi[0]);

		// char filename[100] = "/Library/Frameworks/Python.framework/Versions/3.10/bin/python3";
		char filename[100] = "/usr/bin/perl";

		// char script[100] = "upload_cgi.py";
		char script[100] = "add_cgi.pl";
		char* argv[] = {filename, script, NULL};

		// char env1[100] = "CONTENT_TYPE=multipart/form-data; boundary=---------------------------735323031399963166993862150";
		// char env2[100] = "CONTENT_LENGTH=836";
		// char env3[100] = "REQUEST_METHOD=POST";

		char env1[100] = "REQUEST_METHOD=POST";
		char env2[100] = "QUERY_STRING=?num1=1&num2=2";
		char* envp[] = {env1,
		env2,
		// env3,
		// env4,
		NULL};

		execve(filename, argv, envp);
		exit(1);
	} else {
		write(write_to_cgi[1], str.c_str(), str.length());
		close(write_to_cgi[1]);
		close(write_to_cgi[0]);
		close(read_from_cgi[1]);
		// dup2(read_from_cgi[0], STDIN_FILENO);
		std::cout << "parent----------" << std::endl;
		while (waitpid(pid, NULL, 0) != -1){
			std::cout << "waitpid" << std::endl;
		}
		std::cout << "end of while----------" << std::endl;
		char temp[1024];

		ssize_t i = read(read_from_cgi[0], temp, 1024);
		while (i > 0){
			temp[i] = '\0';
			printf("%s", temp);
			i = read(read_from_cgi[0], temp, 1024);
		}
		close(read_from_cgi[0]);
	}
}
