#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>

#define LOG_PREFIX "TEST chardev: "
#define LOG(level, format, arg...) printk(level LOG_PREFIX format, ## arg)

static int test_cdev_init_data __initdata = 2;
static short test_short = 10;
static int test_int __initdata = 20;
static long test_long = 30;
static char *test_string = "a";

module_param(test_short, short, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
module_param(test_int, int, 0000);
module_param(test_long, long, S_IRUSR);
module_param(test_string, charp, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

static int __init test_cdev_init(void)
{
	LOG(KERN_INFO, "module init, data: %d\n", test_cdev_init_data);

	LOG(KERN_INFO, "short: %hd\n", test_short);
	LOG(KERN_INFO, "int: %d\n", test_int);
	LOG(KERN_INFO, "long: %ld\n", test_long);
	LOG(KERN_INFO, "string: %s\n", test_string);

	return 0;
}

static void __exit test_cdev_exit(void)
{
	kernel_param_lock(THIS_MODULE);
	LOG(KERN_INFO, "goodbye short: %hd\n", test_short);
	LOG(KERN_INFO, "goodbye long: %ld\n", test_long);
	LOG(KERN_INFO, "goodbye string: %s\n", test_string);
	kernel_param_unlock(THIS_MODULE);

	LOG(KERN_INFO, "module deinit\n");
}

module_init(test_cdev_init);
module_exit(test_cdev_exit);
