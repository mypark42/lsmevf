#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "lsmevf_user.h"

static char buf[MAX_EVENT_SIZE] = { 0 };

int main(void)
{
	int fd = open("/dev/lsmevf", O_RDONLY);
	if (fd < 0) {
		perror("open");
		return 1;
	}

	while (1) {
		ssize_t n = read(fd, buf, sizeof(buf));
		if (n < 0) {
			perror("read");
			continue;
		}

		union event_user *ev = (union event_user *)buf;

		switch (ev->ev_type) {
		case move_mount:
			struct ev_user_move_mount *ev_move = &ev->move_mount;
			printf("move_mount: from='%s' to='%s'\n",
			       ev->move_mount.from_path,
			       ev->move_mount.to_path);
			break;
		default:
			printf("Unknown event type: %d\n", ev->ev_type);
			break;
		}
	}

	close(fd);
	return 0;
}