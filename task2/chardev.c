/*
 *  chardev.c: Creates a read-only char device that says how many times
 *  you've read from the dev file
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>        /* for put_user */
#include <linux/proc_fs.h>
#include "chardev.h"

#define procfs_name "chardev"

#define PROCFS_MAX_SIZE         1024
#define PROCFS_NAME             "chardev"
#define MAX_MINORS 3

/**
 * This structure hold information about the /proc file
 *
 */
static struct proc_dir_entry *Our_Proc_File;

/**
 * The buffer used to store character for this module
 *
 */
static char procfs_buffer[PROCFS_MAX_SIZE];

/**
 * The size of the buffer
 *
 */
static unsigned long procfs_buffer_size = 0;

/*
 *  Prototypes - this would normally go in a .h file
 */
int init_module(void);
void cleanup_module(void);
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

ssize_t procfile_read(struct file *filePointer,char *buffer,
                      size_t buffer_length, loff_t * offset);
static ssize_t procfile_write(struct file *file, const char *buff,
                              size_t len, loff_t *off);

#define SUCCESS 0
#define DEVICE_NAME "chardev"   /* Dev name as it appears in /proc/devices   */
#define BUF_LEN 80              /* Max length of the message from the device */

/*
 * Global variables are declared as static, so are global within the file.
 */

static int Major;               /* Major number assigned to our device driver */
//static int Minor;
static int Device_Open = 0;     /* Is device open?
                                 * Used to prevent multiple access to device */
static char msg[BUF_LEN];       /* The msg the device will give when asked */
static char *msg_Ptr;
static char Wmsg[MAX_MINORS][BUF_LEN];	/* The msg the device will give when asked */
int i, bytes_written, ret;
long int sum, num;
long int nums[10] = {0};

//char num;
//char nums[10] = {0};


int arrayCount = 0;



static struct file_operations fops = {
        .read = device_read,
        .write = device_write,
        .open = device_open,
        .release = device_release
};

static const struct file_operations proc_file_fops = {
    .owner = THIS_MODULE,
    .read  = procfile_read,
    .write  = procfile_write,
};




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
    //////
        Our_Proc_File = proc_create(PROCFS_NAME,0644,NULL,&proc_file_fops);
    	if(NULL==Our_Proc_File) {
        	proc_remove(Our_Proc_File);
        	printk(KERN_ALERT "Error:Could not initialize /proc/%s\n",PROCFS_NAME);
        	return -ENOMEM;
    	}	

    printk(KERN_INFO "/proc/%s created\n", PROCFS_NAME);
    //////

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
	proc_remove(Our_Proc_File);
    printk(KERN_INFO "/proc/%s removed\n", PROCFS_NAME);
    unregister_chrdev(Major, DEVICE_NAME);
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
        static int counter = 0;
        int minor =0;

        if (Device_Open)
                return -EBUSY;

        minor = MINOR(inode->i_rdev);
        printk( "open on minor number %d\n", minor);
        file->private_data = (int*)&minor;
        Device_Open++;
        sprintf(msg, "I already told you %d times Hello world!\n", counter++);
        msg_Ptr = msg;
        try_module_get(THIS_MODULE);

        return SUCCESS;
}

/*
 * Called when a process closes the device file.
 */
static int device_release(struct inode *inode, struct file *file)
{
        Device_Open--;          /* We're now ready for our next caller */

        /*
         * Decrement the usage count, or else once you opened the file, you'll
         * never get get rid of the module.
         */
        module_put(THIS_MODULE);

        return SUCCESS;
}

/*
 * Called when a process, which already opened the dev file, attempts to
 * read from it.
 */
static ssize_t device_read(struct file *filp,   /* see include/linux/fs.h   */
                           char *buffer,        /* buffer to fill with data */
                           size_t length,       /* length of the buffer     */
                           loff_t * offset)
{
        /*
         * Number of bytes actually written to the buffer
         */
        int bytes_read = 0;
        unsigned int minor=0;
        unsigned int *minor_ptr  = (int *)(filp->private_data);
		minor = *minor_ptr;
		if (minor >= MAX_MINORS)
                return 0;

         printk( "reading on minor number %d\n", minor);


		//memset(msg, 0, BUF_LEN);

		/*
		for (i=0; i<10; i++) {
			sum += nums[i];
		}
		*/
		sprintf(msg, "Entered data is = %s\n", Wmsg[minor]);
        /*
         * If we're at the end of the message,
         * return 0 signifying end of file
         */
        if (*msg_Ptr == 0)
                return 0;

        /*
         * Actually put the data into the buffer
         */
        while (length && *msg_Ptr) {

                /*
                 * The buffer is in the user data segment, not the kernel
                 * segment so "*" assignment won't work.  We have to use
                 * put_user which copies data from the kernel data segment to
                 * the user data segment.
                 */
                put_user(*(msg_Ptr++), buffer++);

                length--;
                bytes_read++;
        }

        /*
         * Most read functions return the number of bytes put into the buffer
         */
        return bytes_read;
}

/*
 * Called when a process writes to dev file: echo "hi" > /dev/hello
 */
static ssize_t device_write(struct file *filp,
			    const char *buff,
			    size_t len,
			    loff_t * off)
{
        //printk(KERN_ALERT "Sorry, this operation isn't supported.\n");
        //return -EINVAL;

		unsigned int minor=0;
        unsigned int *minor_ptr  = (int *)(filp->private_data);
		minor = *minor_ptr;
		
		if (minor >= MAX_MINORS)
                return 0;
        
        printk( "writing on minor number %d\n", minor);

	memset(Wmsg[minor], 0, BUF_LEN);

	for (i=0; i<len && i< BUF_LEN; i++) {
		get_user(Wmsg[minor][i], buff+i);
	}

	bytes_written = i;
	//num = 0;
	//ret = kstrtol (Wmsg, 10, &num);
/*
	if (ret != 0) {
		printk(KERN_ALERT "Didn't recognize the number you entered\n");
	}
	else {
		nums[arrayCount++] = num;
		if (arrayCount == 10) arrayCount = 0;
	}

*/
	return bytes_written;






}


///////////////////////////////procfs//////////////////////////////////


/**
 * This function is called then the /proc file is read
 *
 */
ssize_t procfile_read(struct file *filePointer,char *buffer,
                      size_t buffer_length, loff_t * offset)
{
    int ret=0;
/*
    unsigned int minor=0;
    unsigned int *minor_ptr  = (int *)(filePointer->private_data);
	if(!(*minor_ptr))
		return 0;
	minor = *minor_ptr;
	
	if (minor >= MAX_MINORS)
        return 0;
*/
    if(strlen(buffer) ==0) {
        printk(KERN_INFO "procfile read %s\n",filePointer->f_path.dentry->d_name.name);
        //ret=copy_to_user(buffer,"HelloWorld!\n",sizeof("HelloWorld!\n"));
        ret=copy_to_user(buffer,Wmsg,sizeof(Wmsg));
       
        ret=sizeof(Wmsg);
    }
    return ret;
}


/**
 * This function is called with the /proc file is written
 *
 */
static ssize_t procfile_write(struct file *file, const char *buff,
                              size_t len, loff_t *off)
{
    /*
    unsigned int minor=0;
    unsigned int *minor_ptr  = (int *)(file->private_data);
	minor = *minor_ptr;
	
	if (minor >= MAX_MINORS)
        return 0;
*/
    procfs_buffer_size = len;
    if (procfs_buffer_size > PROCFS_MAX_SIZE)
        procfs_buffer_size = PROCFS_MAX_SIZE;

    if (copy_from_user(procfs_buffer, buff, procfs_buffer_size))
        return -EFAULT;

    procfs_buffer[procfs_buffer_size] = '\0';


    return procfs_buffer_size;
}



/**
 *This function is called when the module is loaded
 *
 */


///////////////////////////////procfs-end//////////////////////////////


