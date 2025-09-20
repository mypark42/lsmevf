#include <linux/module.h>
#include <linux/lsm_hooks.h>
#include <uapi/linux/lsm.h>
#include <linux/kallsyms.h>
#include <linux/kprobes.h>

#include "lsmevf_internal.h"

int lsmevf_init(void);
void lsmevf_exit(void);

// GOAL0: read static table: [sb_mount][0..=MAX_LSM_COUNT]
// GOAL1: 

unsigned long (*lsmevf_kallsyms_name)(const char *) = NULL;

static int kallsyms_init(void)
{
	struct kprobe kp = {0};
	kp.symbol_name = "kallsyms_lookup_name";
	int err = register_kprobe(&kp);
	if (err) {
		pr_err("register_kprobe failed: %s\n", kp.symbol_name);
		return err;
	}
	pr_info("Success getting kallsyms_lookup_name symbol address.\n");
	lsmevf_kallsyms_name = (typeof(lsmevf_kallsyms_name))kp.addr;

	unregister_kprobe(&kp);
	return 0;
}


void lsmevf_exit(void)
{
	pr_debug("%s\n", __func__);

	lsmevf_hook_exit();

	return;
}

int lsmevf_init(void)
{
	pr_info("%s\n", __func__);

	int err = 0;
	err = kallsyms_init();
	if (err) {
		pr_err("No symbol");
		goto out;
	}

	err = lsmevf_dev_init();
	if (err) {
		pr_err("lsmevf_dev_init");
		goto out;
	}

	err = lsmevf_hook_init();
	if (err) {
		pr_err("lsmevf_hook_init");
		goto out;
	}

	
	pr_info("lsmevf initialized!\n");
out:
	return err;
}

module_init(lsmevf_init);
module_exit(lsmevf_exit);
MODULE_LICENSE("GPL");
