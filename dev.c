#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/kprobes.h>
#include <linux/printk.h>

#include "lsmevf_internal.h"


static ssize_t lsmevf_read(struct file *f, char __user *ubuf, size_t len, loff_t *pos)
{
    pr_debug("lsmevf_read: %px, %ld\n", ubuf, len);

    if (access_ok(ubuf, len) == 0) {
        pr_debug("access_ok failed\n");
        return -EFAULT;
    }

    pr_debug("Create databuf for user buffer\n");
    struct databuf *dbuf = create_databuf(ubuf, len);
    if (IS_ERR(dbuf)) {
        pr_err("create_databuf");
        return PTR_ERR(dbuf);
    }

    pr_debug("Wait for data to be ready");
    void *data = read_databuf(dbuf, 10 * HZ);
    if (IS_ERR(data)) {
        pr_err("read_databuf");
        destroy_databuf(dbuf);
        return PTR_ERR(data);
    }

    pr_debug("Data received: %s\n", (char *)data);

    if (dbuf->rbuf) {
        pr_debug("Send reply to writer\n");
        dbuf->rbuf->reply = 0; // TODO: set proper reply code
        complete(&dbuf->rbuf->cpl);
    }
    
    destroy_databuf(dbuf);
    return 0;
}

struct file_operations lsmevf_fops = {
    .owner = THIS_MODULE,
    .read = lsmevf_read,
};

dev_t lsmevf_devt = 0;
struct cdev lsmevf_cdev;
struct class *lsmevf_cls;
struct device *lsmevf_dev;

static int lsmevf_chrdev_init(void)
{
    int err;

    err = alloc_chrdev_region(&lsmevf_devt, 0, 1, "lsmevf");
    if (err) {
        pr_err("alloc_chrdev_region");
        return err;
    }

    cdev_init(&lsmevf_cdev, &lsmevf_fops);
    lsmevf_cdev.owner = THIS_MODULE;

    err = cdev_add(&lsmevf_cdev, lsmevf_devt, 1);
    if (err) {
        pr_err("cdev_add");
        goto fail_cdev_add;
    }
    return 0;

fail_cdev_add:
    unregister_chrdev_region(lsmevf_devt, 1);
    return err;
}

static void lsmevf_chrdev_exit(void)
{
    cdev_del(&lsmevf_cdev);
    unregister_chrdev_region(lsmevf_devt, 1);
}

static int lsmevf_node_init(void) {

    lsmevf_cls = class_create("lsmevf");
    if (IS_ERR(lsmevf_cls)) {
        pr_err("class_create");
        return PTR_ERR(lsmevf_cls);
    }

    lsmevf_dev = device_create(lsmevf_cls, NULL, lsmevf_devt, NULL, "lsmevf");
    if (IS_ERR(lsmevf_dev)) {
        pr_err("device_create");
        class_destroy(lsmevf_cls);
        return PTR_ERR(lsmevf_dev);
    }

    return 0;
}

static void lsmevf_node_exit(void) {
	device_destroy(lsmevf_cls, lsmevf_devt);
	class_destroy(lsmevf_cls);
}

int lsmevf_dev_init(void)
{
    int err;

    err = lsmevf_chrdev_init();
    if (err) {
        pr_err("lsmevf_chrdev_init");
        return err;
    }

    err = lsmevf_node_init();
    if (err) {
        pr_err("lsmevf_node_init");
        lsmevf_chrdev_exit();
        return err;
    }

    return 0;
}

void lsmevf_dev_exit(void)
{
    lsmevf_node_exit();
    lsmevf_chrdev_exit();
}
