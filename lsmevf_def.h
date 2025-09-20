#ifndef LSMEVF_DEF_H
#define LSMEVF_DEF_H
#define ENUM(E) E,

#define FOREACH_HOOK_MOUNT(MACRO) \
	MACRO(sb_mount)           \
	MACRO(sb_umount)          \
	MACRO(sb_remount)         \
	MACRO(sb_kern_mount)      \
	MACRO(move_mount)

#define FOREACH_HOOK(MACRO) FOREACH_HOOK_MOUNT(MACRO)

enum lsmevf_hook_def { FOREACH_HOOK(ENUM) LSMEVF_HOOK_COUNT };

#endif // LSMEVF_DEF_H