#include <fcntl.h>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <stdio.h>
#include <string>
#include <sys/event.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

typedef std::vector<struct kevent> event_list_t;

#define MAX_EVENT_LIST 10

void
add_change_list(event_list_t& change_list, uintptr_t ident, int64_t filter,
				uint16_t flags, uint32_t fflags, intptr_t data,
				void* udata) {
	struct kevent tmp_event;
	EV_SET(&tmp_event, ident, filter, flags, fflags, data, udata);
	change_list.push_back(tmp_event);
}

int
main(void) {

	event_list_t  change_list;
	struct kevent event_list[MAX_EVENT_LIST];
	int			  kq;
	int			  fd = open("a", O_RDONLY);

	kq = kqueue();

	int			   event_len;
	struct kevent* cur_event;

	static int l = 0;
	add_change_list(change_list, fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
	add_change_list(change_list, fd, EVFILT_TIMER, EV_ADD, NOTE_SECONDS, 1, NULL);

	while (true) {
		event_len = kevent(kq, &change_list.front(), change_list.size(), event_list,
						   MAX_EVENT_LIST, NULL);

		if (event_len == -1) {
			std::cout << "event_len error" << std::endl;
		}
		change_list.clear();

		for (int i = 0; i < event_len; ++i) {
			cur_event = &event_list[i];

			switch (cur_event->filter) {
			case EVFILT_READ: {
				char buf[20];

				if (l == 0) {
					// close(cur_event->ident);
					// open("b", O_RDONLY);
					// printf("flags: %d\n", cur_event->flags);
					// printf("fflags: %d\n", cur_event->fflags);
					int n_read = read(cur_event->ident, buf, 20);
					write(STDOUT_FILENO, buf, n_read);
					write(STDOUT_FILENO, "asdf", 4);
				} else {
					int n_read = read(cur_event->ident, buf, 20);
					write(STDOUT_FILENO, buf, n_read);
				}
				break;
			}
			case EVFILT_WRITE:
				break;
			case EVFILT_PROC:
				break;
			case EVFILT_TIMER: {
				close(cur_event->ident);
				fd = open("b", O_RDONLY);
				add_change_list(change_list, fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
			}
			// TODO: timer
			break;
			}
		}
		// add_change_list(change_list, )
	}
	return 0;
}