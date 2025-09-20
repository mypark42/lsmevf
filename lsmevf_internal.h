#ifndef LSMEVF_INTERNAL_H
#define LSMEVF_INTERNAL_H

#include <linux/module.h>
#include <linux/lsm_hooks.h>
#include <uapi/linux/lsm.h>

#define ENUM(E) E,

#define FOREACH_HOOK(MACRO)	\
	MACRO(sb_mount)		\
	MACRO(sb_umount)	\
	MACRO(sb_remount)	\
	MACRO(sb_kern_mount)	\
	MACRO(move_mount)

enum lsmevf_hook_def {
	FOREACH_HOOK(ENUM)
	LSMEVF_HOOK_COUNT
};

struct lsmevf_hook {
	struct lsm_static_call *lsm;
	struct static_call_key key;
	union security_list_options hook;
};

#define call_orig(NAME, ...) ({										\
		typeof(lsmevf_ ## NAME) *orig = (typeof(lsmevf_ ## NAME) *)lsmevf_hooks[NAME].key.func;	\
		orig(__VA_ARGS__);									\
	}) 

extern unsigned long (*lsmevf_kallsyms_name)(const char *);

int lsmevf_hook_init(void);
void lsmevf_hook_exit(void);

int lsmevf_dev_init(void);
void lsmevf_dev_exit(void);

#endif

