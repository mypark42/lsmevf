#ifndef LSMEVF_USER_H
#define LSMEVF_USER_H

#include "lsmevf_def.h"

#define PATH_MAX 4096 // Maximum path length including null terminator
#define MAX_EVENT_SIZE (PATH_MAX * 4) // Maximum size for event data

struct ev_user_move_mount {
	enum lsmevf_hook_def ev_type;
	char from_path[PATH_MAX];
	char to_path[PATH_MAX];
};

union event_user {
	enum lsmevf_hook_def ev_type;
	struct ev_user_move_mount move_mount;
	// other user event types can be added here
};

#endif