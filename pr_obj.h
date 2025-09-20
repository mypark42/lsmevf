#ifndef LSMEVF_DEBUG_H
#define LSMEVF_DEBUG_H

# ifdef DEBUG

#include <linux/module.h>
#include <linux/lsm_hooks.h>
#include <uapi/linux/lsm.h>

static inline void pr_static_call_key(struct static_call_key *k)
{
	pr_info("Static Call Key: %px: func: %px, %s: %lx", k, k->func, (k->type & 0x1) ? "sites":"mods", k->type);
}

static inline void pr_trampoline(void *tramp)
{
	pr_info("Trampoline: %px:", tramp);
	for(int i = 0; i < 8; i++) {
		pr_cont(" %02x", ((unsigned char *)tramp)[i]);
	}
}

static inline void pr_security_hook_list(struct security_hook_list *hl)
{
	pr_info("Hook List: %px: scalls: %px, hook: %px, name: %s", hl, hl->scalls, hl->hook.lsm_func_addr, hl->lsmid->name);
}


static inline void pr_lsm_static_call(struct lsm_static_call  *lsm)
{
	pr_info("LSM Static Call: %px", lsm);
	if (lsm->key)
		pr_static_call_key(lsm->key);
	if (lsm->hl)
		pr_security_hook_list(lsm->hl);
	if (lsm->trampoline)
		pr_trampoline(lsm->trampoline);
	//if (lsm->active)

	pr_info("\n");
}

# else /* ! DEBUG */


static inline void pr_static_call_key(struct static_call_key *k)
{
}

static inline void pr_trampoline(void *tramp)
{
}

static inline void pr_security_hook_list(struct security_hook_list *hl)
{
}


static inline void pr_lsm_static_call(struct lsm_static_call  *lsm)
{
}

# endif /* DEBUG */


#endif
