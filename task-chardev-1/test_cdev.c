#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

#define LOG_PREFIX "TEST chardev: "
#define LOG(level, format, arg...) printk(level LOG_PREFIX format, ## arg)

static int test_cdev_init_data __initdata = 2;

static int __init test_cdev_init(void)
{
	LOG(KERN_INFO, "module init, data: %d\n", test_cdev_init_data);
	return 0;
}

static void __exit test_cdev_exit(void)
{
	LOG(KERN_INFO, "module deinit\n");
}

module_init(test_cdev_init);
module_exit(test_cdev_exit);
