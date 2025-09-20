#ifndef LSMEVF_INTERNAL_H
#define LSMEVF_INTERNAL_H

#include <linux/module.h>
#include <linux/lsm_hooks.h>
#include <uapi/linux/lsm.h>
#include "lsmevf_def.h"

struct lsmevf_hook {
	struct lsm_static_call *lsm;
	struct static_call_key key;
	union security_list_options hook;
};

extern unsigned long (*lsmevf_kallsyms_name)(const char *);

int lsmevf_hook_init(void);
void lsmevf_hook_exit(void);

int lsmevf_dev_init(void);
void lsmevf_dev_exit(void);

/////////////////////////////////////////////////
/////////////////// EVENT ///////////////////////
/////////////////////////////////////////////////

struct ev_sb_mount {
	enum lsmevf_hook_def ev_type;
	const char *dev_name;
	const struct path *path;
	const char *type;
	unsigned long flags;
	void *data;
};

struct ev_sb_umount {
	enum lsmevf_hook_def ev_type;
	struct vfsmount *mnt;
	int flags;
};

struct ev_sb_remount {
	enum lsmevf_hook_def ev_type;
	struct super_block *sb;
	void *mnt_opts;
};

struct ev_sb_kern_mount {
	enum lsmevf_hook_def ev_type;
	const struct super_block *sb;
};

struct ev_move_mount {
	enum lsmevf_hook_def ev_type;
	const struct path *from_path;
	const struct path *to_path;
};

union mount_event {
	struct ev_sb_mount sb_mount;
	struct ev_sb_umount sb_umount;
	struct ev_sb_remount sb_remount;
	struct ev_sb_kern_mount sb_kern_mount;
	struct ev_move_mount move_mount;
};

union event {
	enum lsmevf_hook_def type;
	union mount_event mount;
	// other event types can be added here
};

int notify_event(union event *ev);

static inline int is_sync_event(union event *ev)
{
	switch (ev->type) {
	case sb_mount:
	case sb_remount: // ?
	case move_mount:
		return 1;
	case sb_kern_mount:
	case sb_umount:
	default:
		return 0;
	}
}

static inline int is_async_event(union event *ev)
{
	return !is_sync_event(ev);
}

/////////////////////////////////////////////////
/////////////////// BUFFER ./////////////////////
/////////////////////////////////////////////////

struct replybuf {
	struct list_head list;
	pid_t reader;
	struct completion cpl;
	int reply; // -EPERM or 0
};

struct databuf {
	struct list_head list;
	struct task_struct *reader;
	struct task_struct *writer;
	struct completion cpl;
	void *ubuf;
	size_t len;
	struct replybuf *rbuf; // NULL if no reply expected

	// user buffer mapping info, for unmapping
	struct page **pages;
};

struct databuf *create_databuf(char __user *ubuf, size_t len);
void destroy_databuf(struct databuf *dbuf);
struct databuf *get_one_databuf(unsigned long timeout);
void *read_databuf(struct databuf *dbuf, unsigned long timeout);
struct replybuf *create_replybuf(pid_t reader);
int read_replybuf(struct replybuf *rbuf, unsigned long timeout);
void destroy_replybuf(struct replybuf *rbuf);
void lsmevf_bufpool_init(void);
void lsmevf_bufpool_exit(void);
#endif
