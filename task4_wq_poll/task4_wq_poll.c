/*
 * After write bytes to the driver, 3 processes are started
 * (find min, max and mean value among written bytes).
 * 
 * Issues at the moment:
 * 	- read function doesnt act as typical block/nonblock IO
 * 	- no guarantees that result (min, max and mean) will be
 *	  returned to the same user, which wrote data.
 */

#include "task4_wq_poll.h"

static unsigned int major;				/* Major number						*/

static struct file_operations task4_dev_fops = {
	.owner = THIS_MODULE,
	.open = task4_dev_open,
	.poll = task4_dev_poll,
	.read = task4_dev_read,
	.release = task4_dev_release,
	.write = task4_dev_write,
};

static struct task4_dev task4_device;

/*
 * Open /dev/ file
 */
int task4_dev_open(struct inode *inode, struct file *filp)
{
	filp->private_data = container_of(inode->i_cdev, struct task4_dev, dev);

	try_module_get(THIS_MODULE);
	return 0;
}

/*
 * Close /dev/ file
 */
int task4_dev_release(struct inode *inode, struct file *filp)
{
	module_put(THIS_MODULE);
	return 0;
}

/*
 * Write to /dev/ file
 */
ssize_t task4_dev_write(struct file *filp, const char __user *buf, size_t len, loff_t *offs)
{
	int i;
	struct task4_dev *dev = filp->private_data;

	down_write(&dev->rw_sem);
	if (len > MAX_BUF_SIZE) {
		printk(KERN_INFO "Couldn't copy user data: %d bytes.\n", (int)len);
		return -EFAULT;
	}

	/* Previous data isn't handled yet */
	while (dev->bytes_written > 0) {
		up_write(&dev->rw_sem);

		/* Wait while written data will be handled */
		if (wait_event_interruptible(dev->wrt_q, (dev->bytes_written == 0))) {
			return -ERESTARTSYS;
		}
		down_write(&dev->rw_sem);
	}

	copy_from_user(dev->data, buf, len);
	dev->bytes_written = len;
	up_write(&dev->rw_sem);

	for (i = 0; i < MAX_NUM_WQ; i++) {
		schedule_work(&dev->calc_data_wq[i]);
	}

	return len;
}
/*
 * Read from /dev/ file
 */
ssize_t task4_dev_read(struct file *filp, char __user *buf, size_t len, loff_t *offs)
{
	struct task4_dev *dev = filp->private_data;
	down_read(&dev->rw_sem);
	if (mutex_lock_interruptible(&dev->result_data.mutex)) {
		return -ERESTARTSYS;
	}

	buf[MIN_POS] = dev->result_data.min;
	buf[MAX_POS] = dev->result_data.max;
	buf[MEAN_POS] = dev->result_data.mean;

	if (dev->bytes_written > 0) {
		/* Data was read. */
		dev->bytes_written = 0;
		/* Clear state that data is ready */
		dev->result_data.stateBmp = 0;
		len = 3;
	}
	else {
		len = 0;
	}

	mutex_unlock(&dev->result_data.mutex);
	up_read(&dev->rw_sem);
	wake_up_interruptible(&dev->wrt_q);

	return len;
}

/*
 * Poll /dev/ file
 */
unsigned int task4_dev_poll(struct file *filp, struct poll_table_struct *poll_table)
{
	unsigned int mask = 0;

	struct task4_dev *dev = filp->private_data;
	if (mutex_lock_interruptible(&dev->result_data.mutex)) {
		return -ERESTARTSYS;
	}

	if ((dev->result_data.stateBmp & TASK_COMPL) == TASK_COMPL) {
		mask = POLLIN | POLLRDNORM;
	}

	mutex_unlock(&dev->result_data.mutex);
	printk(KERN_INFO "Poll mask %d called.\n", mask);

	return mask;
}

int task4_init_module(void)
{
	dev_t dev, result;
	int i;

	result = alloc_chrdev_region(&dev, TASK4_DEV_MINOR, MAX_NUM_DEVICES, DEVICE_NAME);

	if (result < 0) {
		printk(KERN_INFO "Device was not registered: %d.\n", result);
		return result;
	}

	major = MAJOR(dev);
	printk(KERN_INFO "Registered major: %d\n", major);

	memset(&task4_device, 0, sizeof(struct task4_dev));

	/* Setup functions to use for workqueue */
	task4_device.find_value_fn[MIN_POS] = find_min_value;
	task4_device.find_value_fn[MAX_POS] = find_max_value;
	task4_device.find_value_fn[MEAN_POS] = find_mean_value;
	for (i = 0; i < MAX_NUM_WQ; i++) {
		INIT_WORK(&task4_device.calc_data_wq[i], task4_device.find_value_fn[i]);
	}

	/* Initialize a device */
	result = task4_setup_dev(&task4_device);
	if (result < 0) {
		printk(KERN_ALERT "Device was not added.\n");
	}

	mutex_init(&task4_device.result_data.mutex);
	init_rwsem(&task4_device.rw_sem);

	init_waitqueue_head(&(task4_device.wrt_q));

	return result;
}

void task4_cleanup_module(void)
{
	unregister_chrdev_region(MKDEV(major, 0), MAX_NUM_DEVICES);
	cdev_del(&task4_device.dev);
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
	char min_value;
	unsigned char *buf;

	struct task4_dev *dev = container_of(wq, struct task4_dev, calc_data_wq[0]);

	down_read(&dev->rw_sem);
	buf = dev->data;
	min_value = buf[0];
	for (i = 1; i < dev->bytes_written; i++) {
		if (min_value > buf[i]) {
			min_value = buf[i];
		}
	}

	if (mutex_lock_interruptible(&dev->result_data.mutex)) {
		return;
	}
	dev->result_data.min = min_value;
	/* Operation is completed */
	dev->result_data.stateBmp |= (1U << MIN_POS);
	mutex_unlock(&dev->result_data.mutex);

	up_read(&dev->rw_sem);

	return;
}

/*
 * Find maximum value from values in buffer.
 */
void find_max_value(struct work_struct *wq)
{
	int i;
	char max_value;
	unsigned char *buf;

	struct task4_dev *dev = container_of(wq, struct task4_dev, calc_data_wq[1]);

	down_read(&dev->rw_sem);
	buf = dev->data;
	max_value = buf[0];
	for (i = 1; i < dev->bytes_written; i++) {
		if (max_value < buf[i]) {
			max_value = buf[i];
		}
	}
	if (mutex_lock_interruptible(&dev->result_data.mutex)) {
		return;
	}
	dev->result_data.max = max_value;
	/* Operation is completed */
	dev->result_data.stateBmp |= (1U << MAX_POS);
	mutex_unlock(&dev->result_data.mutex);

	up_read(&dev->rw_sem);

	return;
}

/*
 * Find mean value from values in buffer.
 */
void find_mean_value(struct work_struct *wq)
{
	int i, count = 0;
	long mean_value = 0;
	unsigned char *buf;

	struct task4_dev *dev = container_of(wq, struct task4_dev, calc_data_wq[2]);

	down_read(&dev->rw_sem);
	buf = dev->data;

	for (i = 0; i < dev->bytes_written; i++) {
		mean_value += buf[i];
		count++;
	}

	if (mutex_lock_interruptible(&dev->result_data.mutex)) {
		return;
	}

	if (count > 0) {
		dev->result_data.mean = (unsigned char)(mean_value / count);
	}
	/* Operation is completed */
	dev->result_data.stateBmp |= (1U << MEAN_POS);
	mutex_unlock(&dev->result_data.mutex);

	up_read(&dev->rw_sem);

	return;
}

module_init(task4_init_module);
module_exit(task4_cleanup_module);
