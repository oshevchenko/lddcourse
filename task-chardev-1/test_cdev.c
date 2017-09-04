#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>

#define DEVNAME "test_cdev"
#define DEVCOUNT 4

#define LOG_PREFIX "TEST chardev: "
#define LOG(level, format, arg...) printk(level LOG_PREFIX format, ## arg)

static uint major;
static int test_cdev_init_data __initdata = 2;
static short test_short = 10;
static int test_int __initdata = 20;
static long test_long = 30;
static char *test_string = "a";

struct cdev test_cdev;

MODULE_AUTHOR("Orest Hera");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Module to test char devices");
MODULE_SUPPORTED_DEVICE("test_cdev");

module_param(major, uint, S_IRUSR);
MODULE_PARM_DESC(major, "Major device number");
module_param(test_short, short, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(test_short, "Writable short param");
module_param(test_int, int, 0000);
module_param_named(test_read_only_long, test_long, long, S_IRUSR);
MODULE_PARM_DESC(test_long, "Read-only long param");
module_param(test_string, charp, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(test_string, "String");

static int test_cdev_open(struct inode *inode, struct file *file)
{
	unsigned minor = iminor(inode);

	LOG(KERN_DEBUG, "open %u\n", minor);

	return 0;
}

static const struct file_operations test_cdev_file_ops = {
	.owner = THIS_MODULE,
	.open = test_cdev_open,
};

static int __init test_cdev_init(void)
{
	int res;
	dev_t devid;

	LOG(KERN_INFO, "module init, data: %d\n", test_cdev_init_data);

	LOG(KERN_INFO, "short: %hd\n", test_short);
	LOG(KERN_INFO, "int: %d\n", test_int);
	LOG(KERN_INFO, "long: %ld\n", test_long);
	LOG(KERN_INFO, "string: %s\n", test_string);

	if (major) {
		devid = MKDEV(major, 0);
		res = register_chrdev_region(devid, DEVCOUNT, DEVNAME);
		if (res) {
			LOG(KERN_ERR, "register region failed: %d\n", res);
			return res;
		}
	} else {
		res = alloc_chrdev_region(&devid, 0, DEVCOUNT, DEVNAME);
		if (res) {
			LOG(KERN_ERR, "alloc region failed: %d\n", res);
			return res;
		}
		major = MAJOR(devid);
		LOG(KERN_INFO, "dynamic major number: %u\n", major);
	}

	cdev_init(&test_cdev, &test_cdev_file_ops);
	test_cdev.owner = THIS_MODULE;

	res = cdev_add(&test_cdev, devid, DEVCOUNT);
	if (res) {
		LOG(KERN_ERR, "dev add failed: %d\n", res);
		unregister_chrdev_region(devid, DEVCOUNT);
		return res;
	}

	return 0;
}

static void __exit test_cdev_exit(void)
{
	kernel_param_lock(THIS_MODULE);
	LOG(KERN_INFO, "goodbye short: %hd\n", test_short);
	LOG(KERN_INFO, "goodbye long: %ld\n", test_long);
	LOG(KERN_INFO, "goodbye string: %s\n", test_string);
	kernel_param_unlock(THIS_MODULE);

	cdev_del(&test_cdev);
	unregister_chrdev_region(MKDEV(major, 0), DEVCOUNT);

	LOG(KERN_INFO, "module deinit\n");
}

module_init(test_cdev_init);
module_exit(test_cdev_exit);
