#include <fcntl.h>
#include <string>
#include <sys/event.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <map>
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

	pid = fork();

	if (pid == 0) {
		sleep(2);
		exit(1);
	} else {
		change_events(change_list, pid, EVFILT_PROC, EV_ADD | EV_ONESHOT, NOTE_EXIT, 0, a);
	}
	kevent(kq, &change_list[0], change_list.size(), NULL, 0, NULL);

	/* main loop */
	int			   new_events;
	struct kevent* curr_event;
	timespec	   time;

	printf("size: %d\n", change_list.size());
	printf("pid: %d\n", pid);

	int fd = open("asdf", O_RDWR | O_APPEND, 0644);

	fcntl(fd, F_SETFL, O_NONBLOCK);

	time.tv_sec	 = 3;
	time.tv_nsec = 0;

	change_list.clear();

	char buf[BUF_SIZE];

	change_events(change_list, 6, EVFILT_TIMER, EV_ADD | EV_ONESHOT, NOTE_SECONDS, 2, (void*)a);

	while (1) {
		/*  apply changes and return new events(pending events) */
		new_events = kevent(kq, &change_list[0], change_list.size(), event_list, 8, &time);

		if (new_events == 0) {
			printf("new_event: %d\n", new_events);
			// change_events(change_list, 6, EVFILT_TIMER, EV_ONESHOT, NOTE_SECONDS, 3, (void*)a);
		}

		for (int i = 0; i < new_events; ++i) {
			curr_event = &event_list[i];

			switch (curr_event->filter) {
			case EVFILT_READ:
				write(STDOUT_FILENO, "read\n", 5);
				read(fd, buf, BUF_SIZE);
				change_events(change_list, fd, EVFILT_WRITE, EV_ENABLE, 0, 0, (void*)a);
				/* code */
				break;

			case EVFILT_WRITE:
				write(STDOUT_FILENO, "write: ", 7);
				write(STDOUT_FILENO, buf, BUF_SIZE);
				write(STDOUT_FILENO, "\n", 1);
				change_events(change_list, fd, EVFILT_WRITE, EV_DISABLE, 0, 0, (void*)a);
				// write(fd, buf, BUF_SIZE);
				/* code */
				break;

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
