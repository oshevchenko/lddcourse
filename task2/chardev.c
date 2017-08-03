/*
 *  chardev.c: Creates a read-only char device that says how many times
 *  you've read from the dev file
 */

#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>	/* for put_user */

/*  
 *  Prototypes - this would normally go in a .h file
 */
int init_module(void);
void cleanup_module(void);
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

#define BUF_LEN 80				/* Max length of the message from the device */
#define DEVICE_NAME "chardev"	/* Dev name as it appears in /proc/devices   */
#define MAX_MINOR 4             /* Maximum files in /dev/                    */
#define NUM_BASE_10 10          /* Number base to use during convert str to l*/
#define PROC_FS_NAME "coolchardev"/* Name for /proc/                         */
#define SUCCESS 0
#define UNSUCCESS -1

/* 
 * Global variables are declared as static, so are global within the file. 
 */

static int Major;					/* Major number assigned to our device driver*/
static int Device_Open = 0;			/* Is device open?  
				 				     * Used to prevent multiple access to device */
static char msg[MAX_MINOR][BUF_LEN];/* The msg the device will give when asked   */
static char *msg_Ptr[MAX_MINOR];

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.read = device_read,
	.write = device_write,
	.open = device_open,
	.release = device_release
};

static struct file_operations proc_fs_fops = {
	.owner = THIS_MODULE,
	.read = device_read
};

struct proc_dir_entry *proc_file_entry;

/*
 * This function is called when the module is loaded
 */
int init_module(void)
{
    Major = register_chrdev(0, DEVICE_NAME, &fops);

	if (Major < 0) {
	  printk(KERN_ALERT "Registering char device failed with %d\n", Major);
	  return Major;
	}

	printk(KERN_INFO "I was assigned major number %d. To talk to\n", Major);
	printk(KERN_INFO "the driver, create a dev file with\n");
	printk(KERN_INFO "'mknod /dev/%s c %d 0'.\n", DEVICE_NAME, Major);
	printk(KERN_INFO "Try various minor numbers. Try to cat and echo to\n");
	printk(KERN_INFO "the device file.\n");
	printk(KERN_INFO "Remove the device file and module when done.\n");

	proc_file_entry = proc_create(PROC_FS_NAME, 0644, NULL, &proc_fs_fops);
	
	if (proc_file_entry == NULL) {
		printk(KERN_ALERT "Error: Could not initialize /proc/%s\n",
		       PROC_FS_NAME);
		return -ENOMEM;
	}

	printk(KERN_INFO "/proc/%s created\n", PROC_FS_NAME);

	return SUCCESS;
}

/*
 * This function is called when the module is unloaded
 */
void cleanup_module(void)
{
	/* 
	 * Unregister the device 
	 */
	unregister_chrdev(Major, DEVICE_NAME);
	printk(KERN_INFO "Device was removed.\n");
	remove_proc_entry(PROC_FS_NAME, NULL);
	printk(KERN_INFO "/proc/%s removed\n", PROC_FS_NAME);
}

/*
 * Methods
 */

/* 
 * Called when a process tries to open the device file, like
 * "cat /dev/mycharfile"
 */
static int device_open(struct inode *inode, struct file *file)
{
	static int minor[MAX_MINOR] = {0, 1, 2, 3};
	int get_minor = 0;

	if (Device_Open == MAX_MINOR)
		return -EBUSY;

	get_minor = MINOR(inode->i_rdev);
	file->private_data = minor + get_minor;

	Device_Open++;

	try_module_get(THIS_MODULE);

	return SUCCESS;
}

/* 
 * Called when a process closes the device file.
 */
static int device_release(struct inode *inode, struct file *file)
{
	Device_Open--;				/* We're now ready for our next caller */

	/* 
	 * Decrement the usage count, or else once you opened the file, you'll
	 * never get get rid of the module. 
	 */
	module_put(THIS_MODULE);

	return 0;
}

/* 
 * Called when a process, which already opened the dev file, attempts to
 * read from it.
 */
static ssize_t device_read(struct file *filp,	/* see include/linux/fs.h   */
			   char *buffer,	/* buffer to fill with data */
			   size_t length,	/* length of the buffer     */
			   loff_t * offset)
{
	int bytes_read = 0;         // Num of bytes actually written to the buf 
	int *minor = 0;                 /* contains minor value      */

	minor = filp->private_data;     // get minor

	/*
	 * If we're at the end of the message, 
	 * return 0 signifying end of file 
	 */
	if (*msg_Ptr[*minor] == 0) {

		msg_Ptr[*minor] = msg[*minor];          /* Return ptr po start pos   */
		return 0;
	}

	/* 
	 * Actually put the data into the buffer 
	 */
	while (length && *msg_Ptr[*minor]) {

		/* 
		 * The buffer is in the user data segment, not the kernel 
		 * segment so "*" assignment won't work.  We have to use 
		 * put_user which copies data from the kernel data segment to
		 * the user data segment. 
		 */
		put_user(*(msg_Ptr[*minor]++), buffer++);

		length--;
		bytes_read++;
	}

	/* 
	 * Most read functions return the number of bytes put into the buffer
	 */
	return bytes_read;
}

/*  
 * Called when a process writes to dev file: echo 123 > /dev/hello 
 */
static ssize_t
device_write(struct file *filp, const char *buff, size_t len, loff_t * off)
{

	int *i = 0;                 /* contains minor value      */
	int bytes_written = 0;		/* bytes write to the buffer */
	int convert_result = 0;     /* result of convertion      */
	long converted_num = 0;      // result of convertion from str

	i = filp->private_data;     // get minor
	msg_Ptr[*i] = msg[*i];      // get ptr to minor related data


	if (len > BUF_LEN) {
		return UNSUCCESS;
	}

	copy_from_user(msg_Ptr[*i], buff, len);

	convert_result = kstrtol(msg[*i], NUM_BASE_10, &converted_num);

	if (convert_result == -ERANGE) {
		printk(KERN_INFO "Entered number is too big or too small.\n");
		bytes_written = UNSUCCESS;
	}
	else if (convert_result == -EINVAL) {
		printk(KERN_INFO "Couldn't convert your number. Try again.\n");
		bytes_written = UNSUCCESS;
	}
	else {
		bytes_written = len;
	}

	return bytes_written;
}
