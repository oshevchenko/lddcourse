#include <linux/cdev.h>				/* Structure cdev 				*/
#include <linux/fs.h>				/* File read/write and so on... */
#include <linux/kdev_t.h>			/* For MAJOR/MINOR macros		*/
#include <linux/kernel.h>			/* printfk 						*/
#include <linux/module.h>
#include <linux/mutex.h>			/* mutual exclusion 			*/
#include <asm/uaccess.h>			/* copy from/to user space		*/

#include "chardev.h"				/* chardev definitions			*/

static unsigned int major;			/* Registered major number		*/

static struct file_operations chardev_fops = {
	.owner = THIS_MODULE,
	.open = chardev_open,
	.read = chardev_read,
	.release = chardev_release,
	.write = chardev_write,
};

static struct chardev_dev chardev_device[MAX_NUM_DEV];

/*
 * Open dev file
 */

int chardev_open(struct inode *inode, struct file *file)
{
	struct chardev_dev *dev;

	dev = container_of(inode->i_cdev, struct chardev_dev, cdev);
	file->private_data = dev;

	try_module_get(THIS_MODULE);
	return 0;
}

/*
 * Close /dev/ file
 */
int chardev_release(struct inode *inode, struct file *file)
{
	module_put(THIS_MODULE);
	return 0;
}

/*
 * Read /dev/ file
 */
ssize_t chardev_read(struct file *filp, char *buffer,
						size_t length, loff_t * offset)
{
	size_t bytes_left, bytes_read;
	struct chardev_dev *dev = filp->private_data;

	if (mutex_lock_interruptible(&dev->mutex)) {
		bytes_read = -ERESTARTSYS;
		goto out;
	}

	bytes_left = dev->size - *offset;

	if (length < bytes_left) {
		bytes_read = length;
	}
	else if (length > bytes_left) {
		bytes_read = bytes_left;
	}
	else {
		bytes_read = 0;
	}

	copy_to_user(buffer, dev->data + *offset, bytes_read);
	*offset += bytes_read;

out:
	mutex_unlock(&dev->mutex);

	return bytes_read;
}

/*
 * Write to /dev/ file
 */

ssize_t chardev_write(struct file *filp, const char *buffer,
						size_t length, loff_t * offset)
{
	size_t bytes_written;
	struct chardev_dev *dev = filp->private_data;

	if (mutex_lock_interruptible(&dev->mutex)) {
		bytes_written = -ERESTARTSYS;
		goto out;
	}

	if (length + *offset >= (MAX_BUF_SIZE - 1)) {
		bytes_written = -1;
		goto out;
	}

	if (copy_from_user(dev->data + *offset, buffer, length)) {
		bytes_written = -EFAULT;
		goto out;
	}
	else {
		bytes_written = length;
		if (*offset) {
			dev->size += length;
		}
		else {
			dev->size = length;
		}
	}

	/* In case if this is the last part to store 					*/
	*(dev->data + dev->size) = 0;

out:
	mutex_unlock(&dev->mutex);

	return bytes_written;
}

int chardev_init_module(void)
{
	dev_t dev;
	int i, result;

	result = alloc_chrdev_region(&dev, CHARDEV_MINOR, MAX_NUM_DEV, DEVICE_NAME);
	major = MAJOR(dev);

	if (result < 0) {
	  printk(KERN_ALERT "Can't register range of devices: %d.\n", result);
	  return major;
	}
	else {
		printk(KERN_ALERT "Registered major: %d\n", major);
		printk(KERN_INFO "'mknod /dev/%s c %d 0'.\n", DEVICE_NAME, major);
	}

	memset(chardev_device, 0, MAX_NUM_DEV * sizeof(struct chardev_dev));

	/* Initialize each device */
	for (i = 0; i < MAX_NUM_DEV; i++) {
		mutex_init(&chardev_device[i].mutex);
		chardev_setup_cdev(&chardev_device[i], i);
	}

	return 0;
}

void chardev_cleanup_module(void)
{
	int i;

	unregister_chrdev_region(MKDEV(major, 0), MAX_NUM_DEV);

	for (i = 0; i < MAX_NUM_DEV; i++) {
		cdev_del(&chardev_device[i].cdev);
		printk(KERN_INFO "Device %d were unregistered.\n", i);
	}
}

static void chardev_setup_cdev(struct chardev_dev *dev, int i)
{
	int result;
	dev_t dev_num;

	cdev_init(&dev->cdev, &chardev_fops);
	dev_num = MKDEV(major, i);

	result = cdev_add(&dev->cdev, dev_num, 1);
	if (result < 0) {
		printk(KERN_INFO "Can't add chardev device %d.\n", i);
	}
}

module_init(chardev_init_module);
module_exit(chardev_cleanup_module);
