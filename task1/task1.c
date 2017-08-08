#include <linux/init.h>
#include <linux/module.h>

MODULE_LICENSE("Dual BSD/GPL");

static int print_count;
module_param(print_count, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);

static int my_module_init(void)
{
    char buf[11];
    int i = 0;
    for (i = 0; i < print_count && i < sizeof(buf) - 1; i++)
        buf[i] = '!';
    buf[i] = 0;

    printk(KERN_DEFAULT "task1: Hello world Kernel%s\n", buf);
	return 0;
}

static void my_module_exit(void)
{
    printk(KERN_DEFAULT "task1: Goodbye Kernel\n");
}

module_init(my_module_init)
module_exit(my_module_exit)
