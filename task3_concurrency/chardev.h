#define DEVICE_NAME "chardev"		/* Device name in /dev/			*/
#define MAX_BUF_SIZE 40				/* Buffer length				*/
#define MAX_NUM_DEV 4               /* chardev0 .. chardev3			*/
#define CHARDEV_MINOR 0				/* MINOR						*/

struct chardev_dev {
	uint8_t data[MAX_BUF_SIZE];		/* Written data					*/
	size_t size;					/* Num of stored bytes in data	*/
	struct cdev cdev;				/* Char device structure 		*/
	struct mutex mutex;				/* Mutual exclusion semaphore	*/
};

int chardev_open(struct inode *inode, struct file *file);
int chardev_release(struct inode *inode, struct file *file);
ssize_t chardev_read(struct file *, char *, size_t, loff_t *);
ssize_t chardev_write(struct file *, const char *, size_t, loff_t *);
static void chardev_setup_cdev(struct chardev_dev *, int);