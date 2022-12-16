#pragma once

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <queue>
#include <stdlib.h>
#include <string>
#include <sys/event.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>
#include <vector>

// change_list, fd, EVFILT_READ OR WRITE<status>, EV_ADD | EV_ENABLE <actions>, 0, 0, udata
void
add_event_change_list(std::vector<struct kevent>& change_list,
					  uintptr_t ident, int64_t filter, uint16_t flags,
					  uint32_t fflags, intptr_t data, void* udata) {
	struct kevent tmp_event;
	EV_SET(&tmp_event, ident, filter, flags, fflags, data, udata);
	change_list.push_back(tmp_event);
};
