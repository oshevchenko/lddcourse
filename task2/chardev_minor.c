/*  chardev_minor.c: Creates char device. Read/Write access.
 *  /dev/chardev   dev read/write device device
 *  /proc/hello_proc -proc readonly device(return last enterred line to any of chardev device)
 *                    Stores only last write does not matter what minor was used to write
 *
 *  Copyright (C) 2001 by Peter Jay Salzman
 *
 *  08/02/2006 - Updated by Rodrigo Rubira Branco <rodrigo@kernelhacking.com>
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <asm/uaccess.h>  /* for put_user */
#include <asm/errno.h>

/*  Prototypes - this would normally go in a .h file */
int init_module(void);
void cleanup_module(void);
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);


#define SUCCESS 0
#define DEVICE_NAME "chardev" /* Dev name as it appears in /proc/devices   */
#define BUF_LEN 80            /* Max length of the message from the device */
#define MINOR_MAX 3

/******************** Procfs part declarations *******************/
static ssize_t proc_device_read(struct file *, char *, size_t, loff_t *);
static ssize_t proc_device_write(struct file *, const char *, size_t, loff_t *);
static int hello_proc_open(struct inode *inode, struct  file *file);

static const struct file_operations hello_proc_fops = {
    .owner = THIS_MODULE,
    .open = hello_proc_open,
    .read = proc_device_read,
    .write = proc_device_write,
    .llseek = seq_lseek,
    .release = single_release,
};

static char *msg_Ptr_proc;
/******************** END Procfs part declarations *******************/


/******************** Chardev part declarations *******************/

/* Global variables are declared as static, so are global within the file. */

static int Major;            /* Major number assigned to our device driver */
static int Device_Open = 0;  /* Is device open?  Used to prevent multiple
                                        access to the device */
static char msg[MINOR_MAX][BUF_LEN];    /* The msg the device will give when asked    */
//static char *msg_Ptr[MINOR_MAX];


static struct file_operations fops = {
  .read = device_read,
  .write = device_write,
  .open = device_open,
  .release = device_release
};

/******************** END Chardev part declarations *******************/


/******************** Chardev Functions *****************/

int init_module(void)
{
   Major = register_chrdev(0, DEVICE_NAME, &fops);

   if (Major < 0) {
     printk ("Registering the character device failed with %d\n", Major);
     return Major;
   }

   printk("<1>I was assigned major number %d.  To talk to\n", Major);
   printk("<1>the driver, create a dev file with\n");
   printk("'mknod /dev/chardev c %d 0'.\n", Major);
   printk("<1>Try various minor numbers.  Try to cat and echo to\n");
   printk("the device file.\n");
   printk("<1>Remove the device file and module when done.\n");

   proc_create("hello_proc", 0, NULL, &hello_proc_fops);
   printk("P5 : Process hello proc created");
   
   return 0;
}


void cleanup_module(void)
{
   /* Unregister the device */
   unregister_chrdev(Major, DEVICE_NAME);
   remove_proc_entry("hello_proc", NULL);
}


/* Methods */

/* Called when a process tries to open the device file, like
 * "cat /dev/mycharfile"
 */
static int device_open(struct inode *inode, struct file *file)
{
   int minor = 0;
   if (Device_Open) return -EBUSY;

   minor = iminor(inode);
   Device_Open++;
   //sprintf(msg,"I already told you %d times Hello world!\n", counter++);
   //msg_Ptr = msg;

   file->private_data = (void*) msg[minor];

   return SUCCESS;
}


/* Called when a process closes the device file */
static int device_release(struct inode *inode, struct file *file)
{
   Device_Open --;     /* We're now ready for our next caller */

   /* Decrement the usage count, or else once you opened the file, you'll
                    never get get rid of the module. */
   return 0;
}


/* Called when a process, which already opened the dev file, attempts to
   read from it.
*/
static ssize_t device_read(struct file *file,
   char *buffer,    /* The buffer to fill with data */
   size_t length,   /* The length of the buffer     */
   loff_t *offset)  /* Our offset in the file       */
{
   char *mPtr;
   /* Number of bytes actually written to the buffer */
   int bytes_read;

   mPtr = (char*) file->private_data;

   bytes_read = strlen(mPtr);
   /* If we're at the end of the message, return 0 signifying end of file */
   if (*mPtr == 0) return 0;

   
   if (copy_to_user(buffer, mPtr, bytes_read))
	return -EPERM;
   mPtr += bytes_read;
   file->private_data = (void*)mPtr;
   printk ("Read %d bytes.\n", bytes_read);
   /* Most read functions return the number of bytes put into the buffer */
   return bytes_read;
}


/*  Called when a process writes to dev file: echo "hi" > /dev/hello */
static ssize_t device_write(struct file *file,
   const char *buff,
   size_t len,
   loff_t *off)
{
   char *mPtr;
   mPtr = (char*) file->private_data;
   msg_Ptr_proc = (char*) file->private_data;
   copy_from_user(mPtr, buff, len);
   printk ("written %ld bytes.\n", len);
   return len;
}
/******************** END Chardev Functions *****************/

/************************ Procfs Functions *********************/
static int hello_proc_show(struct seq_file *m, void *v) {
    seq_printf(m, "P5 : Hello proc!\n");
    return 0;
}

static int hello_proc_open(struct inode *inode, struct  file *file) {
    return single_open(file, hello_proc_show, NULL);
}


/*  Called when a process writes to dev file: echo "hi" > /dev/hello */
static ssize_t proc_device_write(struct file *filp,
   const char *buff,
   size_t len,
   loff_t *off)
{
   printk ("<1>Sorry, this operation isn't supported.\n");
   return -EINVAL;
}

static ssize_t proc_device_read(struct file *file,
	char *buffer, 
        size_t len, 
	loff_t *off)
{
   int bytes_read;
   bytes_read = strlen(msg_Ptr_proc);
   /* If we're at the end of the message, return 0 signifying end of file */
   if (*msg_Ptr_proc == 0) return 0;

   
   if (copy_to_user(buffer, msg_Ptr_proc, bytes_read))
	return -EPERM;
   msg_Ptr_proc += bytes_read;
   printk ("Read %d bytes.\n", bytes_read);
   /* Most read functions return the number of bytes put into the buffer */
   return bytes_read;
}

/*
 * Exit and init for procfs are used in chardev init and exit
*/
/************************ END Procfs Functions *********************/

MODULE_LICENSE("GPL");
