
struct ev_sb_mount {
	char dev_name[PATH_MAX]; // PATH_MAX ?
	char path[PATH_MAX];
	char type[PATH_MAX]; //?
	unsigned int flags;
	unsigned char mnt_opts[PAGE_SIZE];
};

struct ev_sb_umount {
	char mnt_path[PATH_MAX]; // vfsmnt(mount)
	int flags;
};

struct ev_sb_remount {
};

struct ev_sb_kern_mount {
};

struct ev_move_mount {
	char from_path[PATH_MAX];
	char to_path[PATH_MAX];
};

union mount_event {
	struct ev_sb_mount;
	struct ev_sb_umount;
	struct ev_sb_remount;
	struct ev_sb_kern_mount;
	struct ev_move_mount;
}

union event {
};
