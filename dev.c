#include <linux/highmen.h>

static ssize_t lsmevf_read(struct file *f, char __user *ubuf, size_t len, loff_t *pos)
{
    printk("lsmevf_read\n");
    return 0;
}


struct file_operations lsmevf_fops = {
    .owner = THIS_MODULE,
    .read = lsmevf_read,
};

dev_t lsmevf_dev = 0;
cdev_t lsmevf_cdev;

int lsmevf_chrdev_init(void)
{
    int err;

    err = alloc_chrdev_region(&lsmevf_dev, 0, 1, "lsmevf");
    if (err) {
        pr_err("alloc_chrdev_region");
        return err;
    }

    cdev_init(&lsmevf_cdev, &lsmevf_fops);
    lsmevf_cdev.owner = THIS_MODULE;

    err = cdev_add(&lsmevf_cdev, lsmevf_dev, 1);
    if (err) {
        pr_err("cdev_add");
        goto fail_cdev_add;
    }
    return 0;

fail_cdev_add:
    unregister_chrdev_region(lsmevf_dev, 1);
    return err;
}

void lsm_evf_chrdev_exit(void)
{
    cdev_del(&lsmevf_cdev);
    unregister_chrdev_region(lsmevf_dev, 1);
}


int lsmevf_dev_init(void)
{
    int err;

    err = lsmevf_chrdev_init();
    if (err) {
        pr_err("lsmevf_chrdev_init");
        return err;
    }

    return 0;
}

void lsmevf_dev_exit(void)
{
    lsmevf_chrdev_exit();
}