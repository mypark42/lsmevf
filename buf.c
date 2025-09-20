#include "lsmevf_internal.h"
#include <linux/semaphore.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/sched.h>
#include <linux/completion.h>
#include <linux/err.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/rwsem.h>

struct bufpool {
	struct semaphore databuf_count;
	spinlock_t list_lock;
	struct list_head databuf_head;
	struct list_head replybuf_head;
};

struct bufpool bpool;

static void put_pages(struct page **pages, unsigned long nr_pages)
{
	for (unsigned long i = 0; i < nr_pages; i++) {
		put_page(pages[i]);
	}
}

static void *__map_ubuf_at_kernel(char __user *ubuf, struct page **pages,
				  unsigned long nr_pages)
{
	long pinned = 0;

	down_read(&current->mm->mmap_lock);
	pinned = get_user_pages((unsigned long)ubuf, nr_pages, FOLL_WRITE,
				pages);
	up_read(&current->mm->mmap_lock);

	if (pinned < 0) {
		pr_err("get_user_pages failed: %ld\n", pinned);
		return ERR_PTR(pinned);
	}

	if (pinned < nr_pages) {
		pr_err("get_user_pages pinned less pages: %ld < %lu\n", pinned,
		       nr_pages);
		put_pages(pages, pinned);
		return ERR_PTR(-EFAULT);
	}

	void *kbuf = vmap(pages, nr_pages, VM_MAP, PAGE_KERNEL);
	if (!kbuf) {
		pr_err("vmap failed\n");
		put_pages(pages, nr_pages);
		return ERR_PTR(-ENOMEM);
	}

	pr_debug("Mapped %lu pages for user buffer %px at kernel address %px\n",
		 nr_pages, ubuf, kbuf);

	return kbuf;
}

static void *map_ubuf_at_kernel(char __user *ubuf, size_t len,
				struct page ***pages_out)
{
	pr_debug("%s\n", __func__);

	unsigned long nr_pages = (len + PAGE_SIZE - 1) / PAGE_SIZE;
	struct page **pages =
		kmalloc_array(nr_pages, sizeof(struct page *), GFP_KERNEL);
	if (!pages) {
		pr_err("kmalloc_array failed\n");
		return ERR_PTR(-ENOMEM);
	}

	void *kbuf = __map_ubuf_at_kernel(ubuf, pages, nr_pages);
	if (IS_ERR(kbuf)) {
		pr_err("map_ubuf_at_kernel");
		kfree(pages);
		return kbuf;
	}

	*pages_out = pages;
	return kbuf;
}

static void unmap_ubuf_from_kernel(void *kbuf, struct page **pages,
				   unsigned long nr_pages)
{
	vunmap(kbuf);
	put_pages(pages, nr_pages);
}

struct databuf *create_databuf(char __user *ubuf, size_t len)
{
	pr_debug("%s\n", __func__);

	struct databuf *dbuf = kmalloc(sizeof(*dbuf), GFP_KERNEL);
	if (!dbuf)
		return ERR_PTR(-ENOMEM);

	struct page **pages = NULL;
	void *kbuf = map_ubuf_at_kernel(ubuf, len, &pages);
	if (IS_ERR(kbuf)) {
		pr_err("map_ubuf_at_kernel");
		kfree(dbuf);
		return ERR_PTR(PTR_ERR(kbuf));
	}

	dbuf->reader = current;
	dbuf->writer = NULL;
	dbuf->ubuf = kbuf;
	dbuf->len = len;
	dbuf->pages = pages;
	dbuf->rbuf = NULL; // No replybuf by default
	init_completion(&dbuf->cpl);
	INIT_LIST_HEAD(&dbuf->list);

	// Add databuf to bufpool's databuf list
	spin_lock(&bpool.list_lock);
	list_add_tail(&dbuf->list, &bpool.databuf_head);
	spin_unlock(&bpool.list_lock);

	up(&bpool.databuf_count);

	return dbuf;
}

void destroy_databuf(struct databuf *dbuf)
{
	pr_debug("%s\n", __func__);
	unsigned long nr_pages = (dbuf->len + PAGE_SIZE - 1) / PAGE_SIZE;
	unmap_ubuf_from_kernel(dbuf->ubuf, dbuf->pages, nr_pages);
	list_del(&dbuf->list);
	kfree(dbuf->pages);
	kfree(dbuf);
}

struct databuf *get_one_databuf(unsigned long timeout)
{
	pr_debug("%s\n", __func__);

	if (down_timeout(&bpool.databuf_count, timeout)) {
		return ERR_PTR(-ETIME); // Timeout occurred
	}

	spin_lock(&bpool.list_lock);
	if (list_empty(&bpool.databuf_head)) {
		spin_unlock(&bpool.list_lock);
		panic("Inconsistent state: dbuf_count > 0 but bpool is empty");
	}

	struct databuf *dbuf =
		list_first_entry(&bpool.databuf_head, struct databuf, list);
	list_del(&dbuf->list);
	INIT_LIST_HEAD(&dbuf->list); // Remove from list
	spin_unlock(&bpool.list_lock);

	dbuf->writer = current;

	return dbuf;
}

void *read_databuf(struct databuf *dbuf, unsigned long timeout)
{
	pr_debug("%s\n", __func__);

	if (dbuf->reader != current) {
		return ERR_PTR(-EPERM); // Only reader can read
	}

	// Wait for data to be available
	if (wait_for_completion_timeout(&dbuf->cpl, timeout) == 0)
		return ERR_PTR(-ETIME); // Timeout occurred

	return dbuf->ubuf;
}

int read_replybuf(struct replybuf *rbuf, unsigned long timeout)
{
	if (rbuf->reader != current->pid)
		return -EPERM; // Only writer can read reply

	// Wait for reply to be available
	if (wait_for_completion_timeout(&rbuf->cpl, timeout) == 0)
		return -ETIME; // Timeout occurred

	return rbuf->reply; // -EPERM or 0
}

struct replybuf *create_replybuf(pid_t reader)
{
	struct replybuf *rbuf;

	rbuf = kmalloc(sizeof(*rbuf), GFP_KERNEL);
	if (!rbuf) {
		return ERR_PTR(-ENOMEM);
	}

	rbuf->reader = reader;
	init_completion(&rbuf->cpl);
	rbuf->reply = -EPERM; // Default to -EPERM
	INIT_LIST_HEAD(&rbuf->list);

	// Add replybuf to bufpool's replybuf list
	spin_lock(&bpool.list_lock);
	list_add(&rbuf->list, &bpool.replybuf_head);
	spin_unlock(&bpool.list_lock);

	return rbuf;
}

void destroy_replybuf(struct replybuf *rbuf)
{
	spin_lock(&bpool.list_lock);
	list_del(&rbuf->list);
	spin_unlock(&bpool.list_lock);
	kfree(rbuf);
}

void lsmevf_bufpool_init(void)
{
	sema_init(&bpool.databuf_count, 0);
	spin_lock_init(&bpool.list_lock);
	INIT_LIST_HEAD(&bpool.databuf_head);
	INIT_LIST_HEAD(&bpool.replybuf_head);
}

void lsmevf_bufpool_exit(void)
{
	// TODO: Cleanup all buf in the pool
}