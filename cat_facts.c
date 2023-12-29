#include<linux/init.h>
#include<linux/fs.h>
#include<linux/uaccess.h>
#include<linux/device.h>
#include<linux/random.h>
#include<linux/cdev.h>
#include<linux/module.h>
#include<linux/printk.h>
#include<linux/version.h>

#define DEVICE_NAME "catfacts"

static const char* const facts[] = {
	"The penalty for killing a cat, 4,000 years ago in Egypt, was death.\n",
	"95% of all cat owners admit they talk to their cats.\n",
	"More cats are left-pawed than right-pawed. Out of 100 cats approximately 40 are left-pawed, 20 are right-pawed, and 40 are ambidextrous.\n",
	"A cat can jump as much as seven times its height.\n",
	"A cat cannot see directly under its nose. This is why the cat cannot seem to find tidbits on the floor.\n",
	"A cat has 230 bones in its body. A human only has 206 bones.\n",
	"A cat has four rows of whiskers.\n",
	"A frightened cat can run at speeds of up to 31 mph (50 km/h), slightly faster than a human sprinter.\n",
	"A cat sees about six times better than a human at night because of the tapetum lucidum, a layer of extra reflecting cells which absorb light.\n",
	"A cat's whiskers, called vibrissae, grow on the cat's face and on the back of its forelegs..\n",
	NULL,
};

static size_t facts_size = 0;

static struct class *device_class;

static unsigned int device_major;
static unsigned int device_num = 1;

static struct cdev catfacts_cdev;

enum { 
	CDEV_NOT_USED = 0, 
	CDEV_EXCLUSIVE_OPEN = 1, 
};

/* Is device open? Used to prevent multiple access to device */ 
static atomic_t already_open = ATOMIC_INIT(CDEV_NOT_USED);

static int cf_device_open(struct inode *inode, struct file *file)
{
	if (atomic_cmpxchg(&already_open, CDEV_NOT_USED, CDEV_EXCLUSIVE_OPEN)) {
		return -EBUSY;
	}

	try_module_get(THIS_MODULE);
	return 0;
}

static ssize_t cf_device_read(struct file *filp, char __user *buffer, size_t length, loff_t *offset)
{
	if (*offset > 0) {
		*offset = 0;
		return 0;
	}

	int bytes_read = 0;
	unsigned int fact_index = get_random_u32() % facts_size;

	const char* fact = facts[fact_index];
	const size_t ll = strlen(fact);

	const size_t to_copy = min(ll, length);
	if (to_copy > 0) {
		if (copy_to_user(buffer, fact, to_copy) != 0) {
			return -EFAULT;
		}

		*offset += to_copy;
		bytes_read += to_copy;
	}

	return bytes_read;
}

static int cf_device_release(struct inode *inode, struct file *file)
{
	atomic_set(&already_open, CDEV_NOT_USED);

	/** 
	 * Decrements the module usage count to allow
	 * freeing memory if nobody is using it.
	 */
	module_put(THIS_MODULE);

	return 0;
}

static struct file_operations fops = {
	.open = cf_device_open,
	.read = cf_device_read,
	.release = cf_device_release,
};

static int __init cat_facts_init(void)
{
	pr_info("**cat_facts module is being initialized...");
	dev_t dev;

	int alloc_ret = -1;
	int cdev_ret = -1;
	alloc_ret = alloc_chrdev_region(&dev, 0, device_num, DEVICE_NAME);

	if (alloc_ret)
		goto error;

	device_major = MAJOR(dev);
	cdev_init(&catfacts_cdev, &fops);
	cdev_ret = cdev_add(&catfacts_cdev, dev, device_num);

	if (cdev_ret)
		goto error;

	/* Compute the facts[] size */
	while (facts[facts_size] != NULL) {
		facts_size++;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
	device_class = class_create(DEVICE_NAME);
#else
	device_class = class_create(THIS_MODULE, DEVICE_NAME);
#endif

	device_create(device_class, NULL, MKDEV(device_major, 0), NULL, DEVICE_NAME);

	return 0;

error:
	if (cdev_ret == 0)
		cdev_del(&catfacts_cdev);
	if (alloc_ret == 0)
		unregister_chrdev_region(dev, device_num);

	return -1;
}

static void __exit cat_facts_exit(void)
{
	pr_info("Goodbye, cat_facts module is exiting...");

	dev_t dev = MKDEV(device_major, 0);

	device_destroy(device_class, dev);
	class_destroy(device_class);

	cdev_del(&catfacts_cdev);
	unregister_chrdev_region(dev, device_num);
}

module_init(cat_facts_init);
module_exit(cat_facts_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("A sample kernel module responding with cat facts"); 

