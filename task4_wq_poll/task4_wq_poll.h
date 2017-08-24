#include <linux/cdev.h>					// Structure cdev
#include <linux/delay.h>
#include <linux/fs.h>					// file_operations structure
#include <linux/kdev_t.h>				// For MAJOR/MINOR macros
#include <linux/kernel.h>				// printfk
#include <linux/module.h>				// For module
#include <linux/mutex.h>				// mutual exclusion
#include <linux/poll.h>					// poll
#include <linux/slab.h>					// kmalloc
#include <asm/atomic.h>					// Atomic operations
#include <asm/uaccess.h>				// copy from/to user space

#define DEVICE_NAME "chw"				// Device name
#define MAX_BUF_SIZE 10*1024			// Buffer size 
#define MAX_NUM_DEVICES 1				// Number of allowed devices
#define TASK4_DEV_MINOR 0				// Minor

struct task4_dev {
	unsigned int bytes_written;			// Written bytes to data array
	unsigned char *data;				// Data to process

	unsigned char avg_value;			// Average value found
	unsigned char min_value;			// Min value found
	unsigned char max_value;			// Max value found

	unsigned int num_open_file;			// Number of opened files
	struct cdev dev;					// Device structure
	struct mutex mutex;					// mutual exclusive lock 
	
	atomic_t opers_left;				// Num of operations left to be executed
	wait_queue_head_t rd_q;				// Read queue
	wait_queue_head_t wrt_q;			// Write queue 

	struct work_struct find_avg_wq;		// wq for find avg work
	struct work_struct find_max_wq;		// wq for find max work
	struct work_struct find_min_wq;		// wq for find min work
};

ssize_t task4_dev_read(struct file *, char __user *, size_t, loff_t *);
ssize_t task4_dev_write(struct file *, const char __user *, size_t, loff_t *);
unsigned int task4_dev_poll(struct file *, struct poll_table_struct *);
int task4_dev_open(struct inode *, struct file *);
int task4_dev_release(struct inode *, struct file *);
static int task4_setup_dev(struct task4_dev *);
void find_min_value(struct work_struct *wq);
void find_max_value(struct work_struct *wq);
void find_avg_value(struct work_struct *wq);
