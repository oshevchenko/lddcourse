#include <linux/module.h>				/* For all modules */
#include <linux/moduleparam.h>          /* For module_param*/
#include <linux/kernel.h>				/* For KERN_INFO   */
#include <linux/init.h>					/* For macros      */

#define DRIVER_AUTHOR "Yaroslav Hrabovskyi <yaroslav.hrabovskyi@gmail.com>"
#define DRIVER_DESC "Simply hello world device driver module."

static int hello_int = 0;
static long hello_long = 0;
static char *hello_string = "what_a_long_string";
static int hello_array[5] = {5, 2, 3};
static int hello_array_num = 0;

module_param(hello_int, int, 0);
MODULE_PARM_DESC(hello_int, "Integer");
module_param(hello_long, long, 0);
MODULE_PARM_DESC(hello_long, "Long");
module_param(hello_string, charp, 0);
MODULE_PARM_DESC(hello_string, "String");
module_param_array(hello_array, int, &hello_array_num, 0);
MODULE_PARM_DESC(hello_array, "Char array");

static int __init hello_init(void)
{
	int i;

	printk(KERN_INFO "Hello, world!\n");
	printk(KERN_INFO "This is a hello_int parameter value: %d", hello_int);
	printk(KERN_INFO "This is a hello_long parameter value: %ld", hello_long);
	printk(KERN_INFO "This is a string: %s", hello_string);
	for (i = 0; i < (sizeof(hello_array)/sizeof(int)); i++)
	{
		printk(KERN_INFO "%d element in array: %d", i, hello_array[i]);
	}
	printk(KERN_INFO "Number of args in array: %d", hello_array_num);

	return 0;
}

static void __exit hello_exit(void)
{
	printk(KERN_INFO "Good bye, world\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_SUPPORTED_DEVICE("some cool device");
