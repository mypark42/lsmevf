#include "lsmevf_internal.h"
#include "lsmevf_user.h"
#include <linux/limits.h>
#include <linux/string.h>
#include <linux/dcache.h>

static int d_path_to(const struct path *path, char *buf, int buflen)
{
	char *p = d_path(path, buf, buflen);
	if (IS_ERR(p)) {
		pr_err("d_path failed: %ld\n", PTR_ERR(p));
		return PTR_ERR(p);
	}
	int n = buf + buflen - p;
	memmove(buf, p, n);
	buf[n - 1] = 0; // ensure null-terminated
	return 0;
}

static int copy_ev_to_buf(union event *ev, char *buf, size_t len)
{
	int err = 0;

	if (len < PATH_MAX * 4) {
		pr_err("buffer too small %zu < %d\n", len, PATH_MAX * 2);
		return -EINVAL;
	}

	switch (ev->type) {
	case move_mount:
		struct ev_move_mount *ev_move = &ev->mount.move_mount;
		struct ev_user_move_mount *ubuf =
			(struct ev_user_move_mount *)buf;

		ubuf->ev_type = ev_move->ev_type;
		err = d_path_to(ev_move->from_path, ubuf->from_path, PATH_MAX);
		if (err)
			return err;
		err = d_path_to(ev_move->to_path, ubuf->to_path, PATH_MAX);
		if (err)
			return err;

		break;
	default:
		pr_err("unknown event type %d\n", ev->type);
		return -EINVAL;
	}
	return 0;
}

int notify_event(union event *ev)
{
	pr_debug("%s\n", __func__);

	struct databuf *dbuf = get_one_databuf(5 * HZ);
	if (IS_ERR(dbuf)) {
		pr_err("get_one_databuf");
		return 0;
	}

	char *buf = (char *)dbuf->ubuf;
	size_t len = dbuf->len;

	int err = copy_ev_to_buf(ev, buf, len);
	if (err) {
		pr_err("copy_ev_to_buf");
		return 0;
	}

	struct replybuf *rbuf = NULL;
	if (is_sync_event(ev)) {
		rbuf = create_replybuf(current->pid);
		dbuf->rbuf = rbuf;
	}

	complete(&dbuf->cpl); // signal that data is ready

	int reply = 0;
	if (rbuf) {
		// wait for reply only for mount events
		reply = read_replybuf(rbuf, 10 * HZ);
		if (reply == -ETIME)
			reply = 0;
		destroy_replybuf(rbuf);
	}

	return reply;
}