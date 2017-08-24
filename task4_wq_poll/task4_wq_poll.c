#include "task4_wq_poll.h"

static unsigned int major;				// Major number						*/

static struct file_operations task4_dev_fops = {
	.owner = THIS_MODULE,
	.open = task4_dev_open,
	.poll = task4_dev_poll,
	.read = task4_dev_read,
	.release = task4_dev_release,
	.write = task4_dev_write,
};

static struct task4_dev *task4_device;

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yaroslav Hrabovskyi <yaroslav.hrabovskyi@gmail.com>");

/*
 * Open /dev/ file.
 */
int task4_dev_open(struct inode *inode, struct file *filp)
{
	struct task4_dev *dev;

	dev = container_of(inode->i_cdev, struct task4_dev, dev);
	filp->private_data = dev;

	if (mutex_lock_interruptible(&dev->mutex)) {
		return -ERESTARTSYS;
	}

	// Allocate memory for data
	if (dev->data == NULL) {
		dev->data = kmalloc(MAX_BUF_SIZE, GFP_KERNEL);
		if (dev->data == NULL) {
			mutex_unlock(&dev->mutex);
			return -ENOMEM;
		}
	}
	dev->num_open_file++;

	mutex_unlock(&dev->mutex);
	printk(KERN_INFO "%s (%i) opened /dev/ file.\n",
			current->comm, current->pid);
	try_module_get(THIS_MODULE);
	return 0;
}

/*
 * Close /dev/ file.
 */
int task4_dev_release(struct inode *inode, struct file *filp)
{
	struct task4_dev *dev;

	dev = container_of(inode->i_cdev, struct task4_dev, dev);
	if (mutex_lock_interruptible(&dev->mutex)) {
		return -ERESTARTSYS;
	}

	if (!(--dev->num_open_file)) {
		kfree(dev->data);
		dev->bytes_written = 0;
		dev->data = NULL;
	}

	mutex_unlock(&dev->mutex);
	printk(KERN_INFO "%s (%i) closed /dev/ file.\n",
			current->comm, current->pid);
	module_put(THIS_MODULE);
	return 0;
}

/*
 * Write to /dev/ file.
 */
ssize_t task4_dev_write(struct file *filp, const char __user *buf,
						size_t len, loff_t *offs)
{
	struct task4_dev *dev;

	dev = filp->private_data;

	if (mutex_lock_interruptible(&dev->mutex)) {
		return -ERESTARTSYS;
	}

	while (dev->bytes_written) {
		//Previous data is being processing
		mutex_unlock(&dev->mutex);
		if (filp->f_flags & O_NONBLOCK) {
			return -EAGAIN;
		}
		if (wait_event_interruptible(dev->wrt_q,
									dev->bytes_written == 0)) {
			return -ERESTARTSYS;
		}
		if (mutex_lock_interruptible(&dev->mutex)) {
			return -ERESTARTSYS;
		}
	}
	// Buffer is ready to take data
	copy_from_user(dev->data, buf, len);
	dev->bytes_written = len;
	atomic_set(&dev->opers_left, 3);
	mutex_unlock(&dev->mutex);

	schedule_work(&dev->find_min_wq);
	schedule_work(&dev->find_max_wq);
	schedule_work(&dev->find_avg_wq);

	printk(KERN_INFO "%s (%i) wrote %lu bytes.\n",
			current->comm, current->pid, len);

	return len;
}
/*
 * Read from /dev/ file.
 */
ssize_t task4_dev_read(struct file *filp, char __user *buf,
						size_t len, loff_t *offs)
{
	char buf_from[3];
	int read_bytes;
	struct task4_dev *dev;

	dev = filp->private_data;
	if (mutex_lock_interruptible(&dev->mutex)) {
		return -ERESTARTSYS;
	}

	while (atomic_read(&dev->opers_left) || (dev->bytes_written == 0)) {
		// Data is not ready or it is absent
		mutex_unlock(&dev->mutex);
		if (filp->f_flags & O_NONBLOCK) {
			return -EAGAIN;
		}

		if (wait_event_interruptible(dev->rd_q,
				(atomic_read(&dev->opers_left) || (dev->bytes_written != 0)))) {
			return -ERESTARTSYS;
		}

		if (mutex_lock_interruptible(&dev->mutex)) {
			return -ERESTARTSYS;
		}
	}

	// Data is ready
	if (len >= 3) {
		buf_from[0] = dev->min_value;
		buf_from[1] = dev->max_value;
		buf_from[2] = dev->avg_value;
		copy_to_user(buf, buf_from, 3);
		dev->bytes_written = 0;
		read_bytes = 3;
	}
	else {
		read_bytes = 0;
	}

	mutex_unlock(&dev->mutex);
	// Wake up writers
	wake_up_interruptible(&dev->wrt_q);

	printk(KERN_INFO "%s (%i) read %d bytes.\n",
			current->comm, current->pid, read_bytes);
	return read_bytes;
}

/*
 * Poll /dev/ file.
 */
unsigned int task4_dev_poll(struct file *filp,
							struct poll_table_struct *poll_table)
{
	struct task4_dev *dev;
	unsigned int mask = 0;

	dev = filp->private_data;
	if (mutex_lock_interruptible(&dev->mutex)) {
		return mask;
	}

	poll_wait(filp, &dev->rd_q, poll_table);
	poll_wait(filp, &dev->wrt_q, poll_table);

	if (!atomic_read(&dev->opers_left) && !dev->bytes_written) {
		mask |= POLLOUT | POLLWRNORM;
	}

	if (!atomic_read(&dev->opers_left) && dev->bytes_written) {
		mask |= POLLIN | POLLRDNORM;
	}

	mutex_unlock(&dev->mutex);
	return mask;
}

/*
 * Called whe insmod is executed.
 */
int task4_init_module(void)
{
	dev_t dev;							// Contains major
	int result;

	// Get major
	result = alloc_chrdev_region(&dev, TASK4_DEV_MINOR,
								MAX_NUM_DEVICES, DEVICE_NAME);

	if (result < 0) {
		printk(KERN_WARNING "Device major was not registered: %d.\n", result);
		return result;
	}
	major = MAJOR(dev);
	printk(KERN_NOTICE "Registered major: %d.\n", major);

	// Allocate memory for device structure
	task4_device = kmalloc(sizeof(struct task4_dev), GFP_KERNEL);
	if (task4_device == NULL) {
		printk(KERN_WARNING "Allocation memory for device structure failed.\n");
		unregister_chrdev_region(MKDEV(major, 0), MAX_NUM_DEVICES);
		return -ENOMEM;
	}

	// Clear device structure data
	memset(task4_device, 0, sizeof(struct task4_dev));

	// Initialize device
	result = task4_setup_dev(task4_device);
	if (result < 0) {
		printk(KERN_WARNING "Device was not added.\n");
		unregister_chrdev_region(MKDEV(major, 0), MAX_NUM_DEVICES);
		kfree(task4_device);
		return 0;
	}

	// Initislize work queues
	INIT_WORK(&task4_device->find_avg_wq, &find_avg_value);
	INIT_WORK(&task4_device->find_max_wq, &find_max_value);
	INIT_WORK(&task4_device->find_min_wq, &find_min_value);

	mutex_init(&task4_device->mutex);
	init_waitqueue_head(&task4_device->wrt_q);
	init_waitqueue_head(&task4_device->rd_q);

	return result;
}

/*
 * Called wehn rmmod is module.
 */
void task4_cleanup_module(void)
{
	cdev_del(&task4_device->dev);
	kfree(task4_device);
	task4_device = NULL;
	unregister_chrdev_region(MKDEV(major, 0), MAX_NUM_DEVICES);
	printk(KERN_INFO "Device was unregistered.\n");
}

static int task4_setup_dev(struct task4_dev *device)
{
	dev_t dev_num;

	cdev_init(&device->dev, &task4_dev_fops);
	dev_num = MKDEV(major, 0);
	return cdev_add(&device->dev, dev_num, 1);
}

/*
 * Find minimum value from values in buffer.
 */
void find_min_value(struct work_struct *wq)
{
	int i;
	unsigned char min;
	unsigned char *buf;

	struct task4_dev *dev = container_of(wq, struct task4_dev, find_min_wq);

	// Lock access to structure
	if (mutex_lock_interruptible(&dev->mutex)) {
		return;
	}

	buf = dev->data;
	min = 255;
	// Find min value
	for (i = 0; i < dev->bytes_written; i++) {
		if (min > buf[i]) {
			min = buf[i];
		}
	}
	dev->min_value = min;

	mutex_unlock(&dev->mutex);

	// If all tasks from work queues are completed
	if (atomic_dec_and_test(&dev->opers_left)) {
		wake_up_interruptible(&dev->rd_q);
	}

	printk(KERN_INFO "%s is called. Bytes processed %u. Value %u found.\n",
					__func__, dev->bytes_written, min);
	return;
}

/*
 * Find maximum value from values in buffer.
 */
void find_max_value(struct work_struct *wq)
{
	int i;
	unsigned char max;
	unsigned char *buf;

	struct task4_dev *dev = container_of(wq, struct task4_dev, find_max_wq);

	// Lock access to structure
	if (mutex_lock_interruptible(&dev->mutex)) {
		return;
	}

	//mdelay(2);
	buf = dev->data;
	max = 0;
	// Find max value
	for (i = 0; i < dev->bytes_written; i++) {
		if (max < buf[i]) {
			max = buf[i];
		}
	}
	dev->max_value = max;

	mutex_unlock(&dev->mutex);

	// If all tasks from work queues are completed
	if (atomic_dec_and_test(&dev->opers_left)) {
		wake_up_interruptible(&dev->rd_q);
	}

	printk(KERN_INFO "%s is called. Bytes processed %u. Value %u found.\n",
					__func__, dev->bytes_written, max);
	return;
}

/*
 * Find avg value from values in buffer.
 */
void find_avg_value(struct work_struct *wq)
{
	unsigned int i, count, sum;
	unsigned char avg;
	unsigned char *buf;

	struct task4_dev *dev = container_of(wq, struct task4_dev, find_avg_wq);

	// Lock access to structure
	if (mutex_lock_interruptible(&dev->mutex)) {
		return;
	}

	buf = dev->data;
	sum = 0, count = 0, avg = 0;

	// Find min value
	if (dev->bytes_written > 0) {
		for (i = 0; i < dev->bytes_written; i++) {
			sum += buf[i];
			count++;
		}
		avg = sum / count;
	}
	dev->avg_value = avg;
	
	mutex_unlock(&dev->mutex);

	// If all tasks from work queues are completed
	if (atomic_dec_and_test(&dev->opers_left)) {
		wake_up_interruptible(&dev->rd_q);
	}

	printk(KERN_INFO "%s is called. Bytes processed %u. Value %u found.\n",
					__func__, dev->bytes_written, avg);
	return;
}

module_init(task4_init_module);
module_exit(task4_cleanup_module);
