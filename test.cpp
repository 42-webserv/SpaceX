#include <fcntl.h>
#include <iostream>
#include <map>
#include <string>
#include <sys/event.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#define BUF_SIZE 5

void
change_events(std::vector<struct kevent>& change_list, uintptr_t ident,
			  int16_t filter, uint16_t flags, uint32_t fflags,
			  intptr_t data, void* udata) {
	struct kevent temp_event;
	EV_SET(&temp_event, ident, filter, flags, fflags, data, udata);
	change_list.push_back(temp_event);
}

int
main(void) {

	/* init kqueue */
	int kq = kqueue();
	printf("kq: %d\n", kq);

	std::vector<struct kevent> change_list; // kevent vector for changelist
	struct kevent			   event_list[8]; // kevent array for eventlist

	int	  pid;
	int	  status;
	char* a = "udata";
	int	  pipe_fd[2];
	int	  pipe_fd2[2];

	pipe(pipe_fd2);
	pipe(pipe_fd);
	pid = fork();

	if (pid == 0) {
		close(pipe_fd[0]);
		dup2(pipe_fd[1], STDOUT_FILENO);
		close(pipe_fd[1]);
		dup2(pipe_fd[0], STDIN_FILENO);
		sleep(2);
		write(STDOUT_FILENO, "asdf", 4);
		sleep(2);
		exit(1);
	} else {
		close(pipe_fd[1]);
		close(pipe_fd2[0]);
		printf("pipe fds %d, %d, %d, %d\n", pipe_fd[0], pipe_fd[1], pipe_fd2[0], pipe_fd2[1]);
		fcntl(pipe_fd2[1], F_SETFL, O_NONBLOCK);
		change_events(change_list, pipe_fd2[1], EVFILT_WRITE, EV_ADD | EV_ENABLE | EV_EOF, 0, 0, a);
		change_events(change_list, pipe_fd[0], EVFILT_READ, EV_ADD | EV_EOF, 0, 0, a);
		change_events(change_list, pid, EVFILT_PROC, EV_ADD | EV_ONESHOT, NOTE_EXIT, 0, a);
	}
	kevent(kq, &change_list[0], change_list.size(), NULL, 0, NULL);

	/* main loop */
	int			   new_events;
	struct kevent* curr_event;
	timespec	   time;

	printf("size: %d\n", change_list.size());
	printf("pid: %d\n", pid);

	int fd = open("asdf", O_RDONLY | O_APPEND, 0644);

	// fcntl(fd, F_SETFL, O_NONBLOCK);

	// printf("fd: %d\n", fd);

	// change_events(change_list, fd, EVFILT_READ, EV_ADD | EV_EOF, 0, 0, a);

	time.tv_sec	 = 3;
	time.tv_nsec = 0;

	// change_list.clear();

	char buf[BUF_SIZE];

	// change_events(change_list, 6, EVFILT_TIMER, EV_ADD | EV_ONESHOT, NOTE_SECONDS, 2, (void*)a);

	printf("size: %d\n", change_list.size());
	while (1) {
		/*  apply changes and return new events(pending events) */
		new_events = kevent(kq, &change_list[0], change_list.size(), event_list, 8, &time);

		if (new_events == -1) {
			std::cerr << "err event" << std::endl;
		}
		if (new_events == 0) {
			printf("new_event: %d\n", new_events);
			// change_events(change_list, 6, EVFILT_TIMER, EV_ONESHOT, NOTE_SECONDS, 3, (void*)a);
		}

		for (int i = 0; i < new_events; ++i) {
			curr_event = &event_list[i];

			if (curr_event->flags & EV_EOF) {
				if (curr_event->filter == EVFILT_WRITE) {
					std::cout << "write_finished" << std::endl;
					printf("fd: %d\n", curr_event->ident);
					change_events(change_list, curr_event->ident, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
				} else if (curr_event->filter == EVFILT_READ) {
					std::cout << "read_finished" << std::endl;
					printf("fd: %d\n", curr_event->ident);
					change_events(change_list, curr_event->ident, EVFILT_READ, EV_DELETE, 0, 0, NULL);
				} else if (curr_event->filter == EVFILT_PROC) {
					std::cout << "process_finished" << std::endl;
					printf("pid: %d\n", curr_event->ident);
					change_events(change_list, curr_event->ident, EVFILT_PROC, EV_DELETE, 0, 0, NULL);
				}
				// exit(1);
				continue;
			}

			switch (curr_event->filter) {
			case EVFILT_READ: {
				write(STDOUT_FILENO, "read fd: ", 9);
				printf("%d\n", curr_event->ident);
				int n = read(curr_event->ident, buf, BUF_SIZE);
				printf("%d\n", n);
				// change_events(change_list, fd, EVFILT_READ, EV_DISABLE, 0, 0, (void*)a);
				// change_events(change_list, fd, EVFILT_WRITE, EV_ENABLE, 0, 0, (void*)a);
				/* code */
				break;
			}
			case EVFILT_WRITE: {
				std::string a;
				a.assign("a", 10000);
				int n = write(curr_event->ident, a.c_str(), a.size());
				write(STDOUT_FILENO, "write: ", 7);
				printf("%d\n", n);
				// write(STDOUT_FILENO, buf, BUF_SIZE);
				// write(STDOUT_FILENO, "\n", 1);
				// change_events(change_list, fd, EVFILT_READ, EV_ENABLE, 0, 0, (void*)a);
				// change_events(change_list, fd, EVFILT_WRITE, EV_DISABLE, 0, 0, (void*)a);
				// write(fd, buf, BUF_SIZE);
				/* code */
				break;
			}
			case EVFILT_PROC:
				/* code */
				waitpid(curr_event->ident, &status, 0);
				break;

			case EVFILT_TIMER:
				printf("timer!! %s ident: %d\n", (char*)curr_event->udata, curr_event->ident);
				change_events(change_list, 6, EVFILT_TIMER, EV_DELETE, NOTE_SECONDS, 2, (void*)a);
				break;
			}
		}
	}
}
