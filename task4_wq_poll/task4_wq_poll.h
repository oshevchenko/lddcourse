#include <linux/cdev.h>					/* Structure cdev 					*/
#include <linux/fs.h>					/* file_operations structure 		*/
#include <linux/kdev_t.h>				/* For MAJOR/MINOR macros			*/
#include <linux/kernel.h>				/* printfk 							*/
#include <linux/module.h>				/* For module 						*/
#include <linux/mutex.h>				/* mutual exclusion 				*/
#include <linux/poll.h>					/* poll 							*/
#include <linux/rwsem.h>				/* For semaphore					*/
#include <asm/uaccess.h>				/* copy from/to user space			*/

#define DEVICE_NAME "chw"				/* Device name 						*/
#define MAX_BUF_SIZE 10*1024			/* Buffer size    					*/
#define MAX_NUM_DEVICES 1				/* Number of allowed devices 		*/
#define MAX_NUM_WQ 3
#define TASK4_DEV_MINOR 0				/* Minor 							*/

#define MIN_POS  	 0					/* Min position bit or index in arr */
#define MAX_POS  	 1					/* Max position bit or index in arr */
#define MEAN_POS   	 2					/* Mean position bit or idx in arr  */
#define TASK_COMPL 	 0x0007 			/* Task find min, max, mean complete*/

struct min_max_mean {
	unsigned char min;
	unsigned char max;
	unsigned char mean;
	unsigned int stateBmp;				/* State bit map 					*/
	struct mutex mutex;
};

struct task4_dev {
	unsigned int bytes_written;			/* Written bytes to data array		*/
	unsigned char data[MAX_BUF_SIZE];	/* Contains data to process 		*/
	struct min_max_mean result_data;	/* Contains min, max and mean values*/

	struct cdev dev;					/* Device structure					*/
	struct rw_semaphore rw_sem;			/* Read/write semaphore 			*/
	wait_queue_head_t wrt_q;			/* Write queue 						*/

	struct work_struct calc_data_wq[MAX_NUM_WQ];	/* 						*/
	void (*find_value_fn[MAX_NUM_WQ])(struct work_struct *);
};

ssize_t task4_dev_read(struct file *, char __user *, size_t, loff_t *);
ssize_t task4_dev_write(struct file *, const char __user *, size_t, loff_t *);
unsigned int task4_dev_poll(struct file *, struct poll_table_struct *);
int task4_dev_open(struct inode *, struct file *);
int task4_dev_release(struct inode *, struct file *);
static int task4_setup_dev(struct task4_dev *);
void find_min_value(struct work_struct *wq);
void find_max_value(struct work_struct *wq);
void find_mean_value(struct work_struct *wq);
