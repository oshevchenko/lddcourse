#include "task4.h"

#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Creates char device and calculates avg value of a buffer written into it.");

static uint major_num;
module_param(major_num, uint, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(major_num, "Pass desirable major number for device.");

static uint in_buf_size = 1 * 1024;
module_param(in_buf_size, uint, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(in_buf_size, "Input buffer size.");


// __init/__exit ommits function if compiled into kernel
static int __init my_module_init(void)
{
    printk(KERN_DEBUG TAG "Initializing module. major=%u\n", major_num);
    return init_device(major_num, in_buf_size);
}

static void __exit my_module_exit(void)
{
    printk(KERN_DEFAULT TAG "Goodbye Kernel\n");
    deinit_device();
}

module_init(my_module_init)
module_exit(my_module_exit)
