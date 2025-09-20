#include "lsmevf_internal.h"
#include "pr_obj.h"

static struct lsmevf_hook lsmevf_hooks[LSMEVF_HOOK_COUNT];
static struct lsm_static_calls_table *lsmevf_static_calls_table = NULL;


//LSM_HOOK(int, 0, sb_mount, const char *dev_name, const struct path *path, const char *type, unsigned long flags, void *data)
static int lsmevf_sb_mount(const char *dev_name, const struct path *path, const char *type, unsigned long flags, void *data)
{
	pr_info("%s\n", __func__);
	dump_stack();
	return call_orig(sb_mount, dev_name, path, type, flags, data);
}

//LSM_HOOK(int, 0, sb_umount, struct vfsmount *mnt, int flags)
static int lsmevf_sb_umount(struct vfsmount *mnt, int flags)
{
	pr_info("%s\n", __func__);
	dump_stack();

	return call_orig(sb_umount, mnt, flags);
}

//LSM_HOOK(int, 0, sb_remount, struct super_block *sb, void *mnt_opts)
static int lsmevf_sb_remount(struct super_block *sb, void *mnt_opts)
{
	pr_info("%s\n", __func__);
	dump_stack();
	return call_orig(sb_remount, sb, mnt_opts);
}

//LSM_HOOK(int, 0, sb_kern_mount, const struct super_block *sb)
static int lsmevf_sb_kern_mount(const struct super_block *sb)
{
	pr_info("%s\n", __func__);
	dump_stack();
	return call_orig(sb_kern_mount, sb);
}

//LSM_HOOK(int, 0, move_mount, const struct path *from_path,
static int lsmevf_move_mount(const struct path *from_path, const struct path *to_path)
{
	pr_info("%s\n", __func__);
	dump_stack();

	return call_orig(move_mount, from_path, to_path);
}


static void patch_lsm_static_call(struct lsmevf_hook *hook)
{
	struct lsm_static_call *lsm = hook->lsm;

	static_branch_disable(lsm->active);

	__static_call_update(lsm->key, lsm->trampoline, hook->hook.lsm_func_addr);

	static_branch_enable(lsm->active);
}

static void rollback_lsm_static_call(struct lsmevf_hook *hook)
{
	struct lsm_static_call *lsm = hook->lsm;

	static_branch_disable(lsm->active);

	__static_call_update(lsm->key, lsm->trampoline, hook->key.func);

	static_branch_enable(lsm->active);
}

#define LSMEVF_HOOK_INIT(HOOK)	\
	do {			\
		struct lsm_static_call *lsm = lsmevf_static_calls_table->HOOK;			\
		struct lsm_static_call *last = lsm;						\
		for (int i = 0; i < MAX_LSM_COUNT; i++) {					\
			if (lsm->hl) {								\
				pr_lsm_static_call(lsm);					\
				last = lsm;							\
			}									\
			lsm++;									\
		}										\
		lsmevf_hooks[HOOK].lsm = last;							\
		lsmevf_hooks[HOOK].hook.HOOK = lsmevf_ ## HOOK;					\
		memcpy(&lsmevf_hooks[HOOK].key, last->key, sizeof(struct static_call_key));	\
	} while(0);


int lsmevf_hook_init(void)
{
	lsmevf_static_calls_table = (typeof(lsmevf_static_calls_table))lsmevf_kallsyms_name("static_calls_table");
	if (!lsmevf_static_calls_table)
		return -ENOENT;

	FOREACH_HOOK(LSMEVF_HOOK_INIT);

	for(int i = 0; i < LSMEVF_HOOK_COUNT; i++) {
		patch_lsm_static_call(&lsmevf_hooks[i]);
		pr_lsm_static_call(lsmevf_hooks[i].lsm);
	}

	return 0;
}

void lsmevf_hook_exit(void)
{
	for(int i = 0; i < LSMEVF_HOOK_COUNT; i++) {
		rollback_lsm_static_call(&lsmevf_hooks[i]);
		pr_lsm_static_call(lsmevf_hooks[i].lsm);
	}
}
