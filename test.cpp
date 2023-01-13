#include <fcntl.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int
main(void) {

	// int p[2];

	// pipe(p);

	// int pid = fork();
	// if (pid == 0) {
	// 	dup2(p[0], STDIN_FILENO);
	// 	close(p[0]);

	// 	write(p[1], "asdf", 4);
	// 	int buf[10];
	// 	int n_read = read(STDIN_FILENO, buf, 10);

	// } else {
	// 	int buf[10];
	// 	fcntl(p[0], F_SETFL, O_NONBLOCK);
	// 	int n_read = read(p[0], buf, 10);
	// }
}
