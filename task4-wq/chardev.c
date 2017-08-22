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
#include <linux/seq_file.h>     /* for seq_file */
#include <linux/semaphore.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <asm/atomic.h>

#define procfs_name "chardev"

#define PROCFS_MAX_SIZE         1024
#define PROCFS_NAME             "chardev"
#define MAX_MINORS 3
#define DRIVER_AUTHOR "Taras Keryk <taras.keryk@gmail.com>"
#define DRIVER_DESC   "chardev sample driver"


MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);	/* Who wrote this module? */
MODULE_DESCRIPTION(DRIVER_DESC);	/* What does this module do */

static void chardev_work_handler(struct work_struct *w);
static void chardev_work_min_handler(struct work_struct *w);
static void chardev_work_max_handler(struct work_struct *w);
static void chardev_work_avg_handler(struct work_struct *w);
static struct workqueue_struct *wq = 0;
static struct workqueue_struct *wq_min = 0;
static struct workqueue_struct *wq_max = 0;
static struct workqueue_struct *wq_avg = 0;
static DECLARE_DELAYED_WORK(chardev_work, chardev_work_handler);
static DECLARE_DELAYED_WORK(chardev_work_min, chardev_work_min_handler);
static DECLARE_DELAYED_WORK(chardev_work_max, chardev_work_max_handler);
static DECLARE_DELAYED_WORK(chardev_work_avg, chardev_work_avg_handler);
static unsigned long onesec;


static DECLARE_WAIT_QUEUE_HEAD(chardev_wait);
#define CHW_BUFFER 10*1024 /*Max length of the message from the device */ 
//* * Global variables are declared as static, so are global within the file. */
int chw_buffer =  CHW_BUFFER;/*buffer size */

module_param(chw_buffer, int, 0); //change chw_buffer size over module parameter


/**
 * This structure hold information about the /proc file
 *
 */
//static struct proc_dir_entry *Our_Proc_File;

/**
 * The buffer used to store character for this module
 *
 */
//static char procfs_buffer[PROCFS_MAX_SIZE];

/**
 * The size of the buffer
 *
 */
//static unsigned long procfs_buffer_size = 0;

/*
 *  Prototypes - this would normally go in a .h file
 */
int init_module(void);
void cleanup_module(void);
static int my_open(struct inode *inode, struct file *file);
static int my_seq_show(struct seq_file *s, void *v);
static void my_seq_stop(struct seq_file *s, void *v);
static void *my_seq_next(struct seq_file *s, void *v, loff_t *pos);
static void *my_seq_start(struct seq_file *s, loff_t *pos);
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);
static unsigned int chardev_poll(struct file *file, poll_table *wait);
static DEFINE_MUTEX(mutex);
//static struct mutex chardev_lock[MAX_MINORS];

int find_min(void);
int find_max(void);
int find_avg(void);


ssize_t procfile_read(struct file *filePointer,char *buffer,
                      size_t buffer_length, loff_t * offset);
//static ssize_t procfile_write(struct file *file, const char *buff,
//                              size_t len, loff_t *off);

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
static char Wmsg[MAX_MINORS][BUF_LEN] = {0};
//static char Wmsg[BUF_LEN];	/* The msg the device will give when asked */
int i, bytes_written, ret;
long int sum, num;
long int nums[10] = {0};
//int cdata_ready = 0;

//char num;
//char nums[10] = {0};


int arrayCount = 0;


static struct cdev_data
{
	int min;
	int max;
	int avg;
	char *buffer;
	int minor;
	int ready;
	atomic_t refcount;
} chardev_data; 

struct cdev_data cdata;

static struct file_operations fops = {
        .read = device_read,
        .write = device_write,
        .open = device_open,
        .release = device_release,
        .poll = chardev_poll
};
/*
static const struct file_operations proc_file_fops = {
    .owner = THIS_MODULE,
    .read  = procfile_read,
    .write  = procfile_write,
};
*/

/**
 * This structure gather "function" that manage the /proc file
 *
 */
static struct file_operations my_file_ops = {
    .owner   = THIS_MODULE,
    .open    = my_open,
    .read    = seq_read,
    .llseek  = seq_lseek,
    .release = seq_release
    //.poll = chardev_poll
};
/**
 * This structure gather "function" to manage the sequence
 *
 */
static struct seq_operations my_seq_ops = {
        .start = my_seq_start,
        .next  = my_seq_next,
        .stop  = my_seq_stop,
        .show  = my_seq_show
};

static unsigned int chardev_poll(struct file *file, poll_table *wait)
{
    pr_info("ready=%d \n", cdata.ready);
    poll_wait(file, &chardev_wait, wait);
    if (cdata.ready==1)
    {
     pr_info("chardev pollwait pollin\n");
        return POLLIN | POLLRDNORM;
    }
    pr_info("chardev pollwait\n");
    return 0;
}


static void
chardev_work_handler(struct work_struct *w)
{
        pr_info("chardev work %u jiffies\n", (unsigned)onesec);


}

static void
chardev_work_min_handler(struct work_struct *w)
{
        int min;

        struct cdev_data *cdev_data_ptr = &cdata;
        
        pr_info("chardev min work %u jiffies\n", (unsigned)onesec);
        //min = find_min();
        cdev_data_ptr->min = find_min();
        pr_info("min=%d \n", cdev_data_ptr->min);
        if(atomic_dec_and_test(&cdev_data_ptr->refcount)){
        	pr_info("chardev min work last %u jiffies\n", (unsigned)onesec);
            cdev_data_ptr->ready = 1;
            pr_info("ready=%d \n", cdata.ready);
            wake_up_interruptible(&chardev_wait);
        }
}
static void
chardev_work_max_handler(struct work_struct *w)
{
        int i;
        int s;
        struct cdev_data *cdev_data_ptr = &cdata;
        cdev_data_ptr->max = find_max();
        pr_info("chardev work max %u jiffies\n", (unsigned)onesec);
		
        //for (i=0;i<chw_buffer;i++)
		//	{s+=(cdev_data_ptr->buffer[i]);	}
		if(atomic_dec_and_test(&cdev_data_ptr->refcount)){
        	pr_info("chardev max work last %u jiffies\n", (unsigned)onesec);
            cdev_data_ptr->ready = 1;
            pr_info("ready=%d \n", cdata.ready);
            wake_up_interruptible(&chardev_wait);
        }
}
static void
chardev_work_avg_handler(struct work_struct *w)
{
        struct cdev_data *cdev_data_ptr = &cdata;
        cdev_data_ptr->avg = find_avg();
        pr_info("chardev work avg %u jiffies\n", (unsigned)onesec);
		if(atomic_dec_and_test(&cdev_data_ptr->refcount)){
        	pr_info("chardev avg work last %u jiffies\n", (unsigned)onesec);
            cdev_data_ptr->ready = 1;
            pr_info("ready=%d \n", cdata.ready);
            wake_up_interruptible(&chardev_wait);
        }
}

int find_min(void)
{
      int min, count;
      //cdev_data_ptr->buff;
      //pr_info("fing_min=%d \n", cdata.buffer[0]);
      min = cdata.buffer[0];
      for (count = 1; count < chw_buffer; count++)
      {
        if(cdata.buffer[count] < min)
        {
            min = cdata.buffer[count];
        }
      }
    return min;
}
int find_max(void)
{
    int max, count;
      //cdev_data_ptr->buff;
      //pr_info("fing_min=%d \n", cdata.buffer[0]);
      max = cdata.buffer[0];
      for (count = 1; count < chw_buffer; count++)
      {
        if(cdata.buffer[count] > max)
        {
            max = cdata.buffer[count];
        }
      }
    return max;
}
int find_avg(void)
{
    int avg, count;
    avg = 0;
      for (count = 1; count < chw_buffer; count++)
      { 
            avg += cdata.buffer[count]; 
      }
      avg = avg/count;
    return avg;
}






/*
 * This function is called when the module is loaded
 */
int init_module(void)
{
           struct proc_dir_entry *entry;
        struct cdev_data *cdev_data_ptr = &cdata;
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
    /*
        Our_Proc_File = proc_create(PROCFS_NAME,0644,NULL,&proc_file_fops);
    	if(NULL==Our_Proc_File) {
        	proc_remove(Our_Proc_File);
        	printk(KERN_ALERT "Error:Could not initialize /proc/%s\n",PROCFS_NAME);
        	return -ENOMEM;
    	}	

    printk(KERN_INFO "/proc/%s created\n", PROCFS_NAME);
    */


    entry = proc_create(PROCFS_NAME, 0, NULL, &my_file_ops);
    if(entry == NULL)
    {
        remove_proc_entry(PROCFS_NAME, NULL);
        printk(KERN_DEBUG "Error: Could not initialize /proc/%s\n", PROCFS_NAME);
        return -ENOMEM;
    }
    //////
/*
 	  onesec = msecs_to_jiffies(1000);
         pr_info("chardev loaded %u jiffies\n", (unsigned)onesec);
        atomic_set(&cdev_data_ptr->refcount, 3);
        if (!wq)
                wq = create_workqueue("chardev");
        if (wq) {
            queue_delayed_work(wq, &chardev_work, onesec);
           	
		}
		if(!wq_min)
			wq_min= create_workqueue("chardev_min");
		if(wq_min){
			queue_delayed_work(wq_min, &chardev_work_min, onesec);
		}
		if(!wq_max)
			wq_max= create_workqueue("chardev_max");
		if(wq_max){
			queue_delayed_work(wq_max, &chardev_work_max, onesec);
		}
		if(!wq_avg)
			wq_avg= create_workqueue("chardev_avg");
		if(wq_avg){
			queue_delayed_work(wq_avg, &chardev_work_avg, onesec);
		}

		*/

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
	//proc_remove(Our_Proc_File);
    //printk(KERN_INFO "/proc/%s removed\n", PROCFS_NAME);
    remove_proc_entry(PROCFS_NAME, NULL);
    printk(KERN_DEBUG "/proc/%s removed\n", PROCFS_NAME);
    if (wq){
    	destroy_workqueue(wq);
    }
    if (wq_min){
    	destroy_workqueue(wq_min);
    }
    if (wq_max){
    	destroy_workqueue(wq_max);
    }
    if (wq_avg){
    	destroy_workqueue(wq_avg);
    }
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
        struct cdev_data *cdev_data_ptr = &cdata;
        
        int minor =0;
        minor = MINOR(inode->i_rdev);

        if (minor >= MAX_MINORS)
                return -ERESTARTSYS;

		//if (mutex_lock_interruptible(&(chardev_lock[minor])))
		//		return -ERESTARTSYS;
			//return -EBUSY;
        
        if (mutex_lock_interruptible(&mutex))
				return -ERESTARTSYS;

        if (Device_Open)
              return -EBUSY;

       
        printk( "open on minor number %d\n", minor);
        //file->private_data = (int*)&minor;
        //cdata.minor=minor;
        cdev_data_ptr->minor = minor;
        //cdev_data_ptr->ready = 0;
        //file->private_data = (struct cdev_data*)&cdata;
        
        Device_Open++;
        sprintf(msg, "I already told you %d times Hello world!\n", counter++);
        msg_Ptr = msg;
        //try_module_get(THIS_MODULE);

        return SUCCESS;
}

/*
 * Called when a process closes the device file.
 */
static int device_release(struct inode *inode, struct file *file)
{
		//int minor =0;
        /*
        minor = MINOR(inode->i_rdev);
		if (minor >= MAX_MINORS)
                return -ERESTARTSYS;
        */
        Device_Open--;          /* We're now ready for our next caller */

        /*
         * Decrement the usage count, or else once you opened the file, you'll
         * never get get rid of the module.
         */
        
        //module_put(THIS_MODULE);
		
		//mutex_unlock(&(chardev_lock[minor]));
		mutex_unlock(&mutex);
        
        return SUCCESS;
}

/*
 * Called when a process, which already opened the dev file, attempts to
 * read from it.
 */
char read_buf[3];
char read_buf_int[3];
static ssize_t device_read(struct file *filp,   /* see include/linux/fs.h   */
                           char *buffer,        /* buffer to fill with data */
                           size_t length,       /* length of the buffer     */
                           loff_t * offset)
{
        /*
         * Number of bytes actually written to the buffer
         */
        int bytes_read = 0;
        int retval = 0;
        int count = 0;
        //char read_buf[3];
        unsigned int minor=0;
        //unsigned int *minor_ptr  = (int *)(filp->private_data);
		struct cdev_data *cdev_data_ptr=&cdata;
		//if (!minor_ptr) return -ERESTARTSYS;
		//if (!((struct cdev_data *)(filp->private_data))) 
		//	return -ERESTARTSYS;
		//minor = *minor_ptr;
		//cdev_data_ptr  = (struct cdev_data *)(filp->private_data);
		minor = cdev_data_ptr->minor;
		printk( "reading on minor number %d\n", minor);
		//if (minor >= MAX_MINORS)
        //        return 0;
        
        printk( "reading on minor number %d\n", minor);


		//memset(msg, 0, BUF_LEN);

		/*
		for (i=0; i<10; i++) {
			sum += nums[i];
		}
		*/
		//sprintf(msg, "Entered data is chardev[%d] = %s\n",minor, cdev_data_ptr->buffer);
        //cdata.ready = 1;
        //sprintf(msg, "%s", Wmsg);
        /*
         * If we're at the end of the message,
         * return 0 signifying end of file
         */

        memset(read_buf, 0, sizeof(read_buf));
        //printk( "open on minor number %d\n", minor);
        //read_buf_int[0] = cdev_data_ptr->min;
        //read_buf_int[1] = cdev_data_ptr->max;
        //read_buf_int[2] = cdev_data_ptr->avg;
        read_buf[0] = cdev_data_ptr->min;
        read_buf[1] = cdev_data_ptr->max;
        read_buf[2] = cdev_data_ptr->avg;
        //cdev_data_ptr->ready = 0;
        
        //count = 3;
        printk(KERN_NOTICE "hello from %s\n min=%d, max=%d, avg=%d \n",__func__, cdev_data_ptr->min, cdev_data_ptr->max, cdev_data_ptr->avg);
        count=3;
        //if (count > 3) count = 3; /* copy 3 bytes to the user */
        retval = copy_to_user(buffer, read_buf, count);
 //       printk( "retval=%d\n", retval);
/*
        while (length && read_buf[count]) {

              
                put_user(read_buf[count], buffer++); 

                length--;
                //bytes_read++;
                count++;
        }
  */     
   //     printk( "length=%d\n", length);
  //      msg_Ptr = read_buf;       
        /*
        if (*msg_Ptr == 0)
                return 0;
            */
//        printk( "length=%d\n", length);
        //while (length && *msg_Ptr) {

 //               printk( "putuser %d\n", *(msg_Ptr));
 //               put_user(read_buf[0], buffer++);
 //               put_user(read_buf[1], buffer++);
//                put_user(read_buf[2], buffer++);

            //    length--;
          //      bytes_read++;
        //}



        
        return count;
        
//        return retval;
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
	printk("chardev write\n");
        //return -EINVAL;
		struct cdev_data *cdev_data_ptr = &cdata;
		unsigned int minor=0;
        

		//if (!((struct cdev_data *)(filp->private_data))) 
		//	return -ERESTARTSYS;
		
		//cdev_data_ptr  = (struct cdev_data *)(filp->private_data);

		minor = cdev_data_ptr->minor;
		
		if (minor >= MAX_MINORS)
                return 0;
        
        printk( "writing on minor number %d\n", minor);
        if(!cdev_data_ptr->buffer)
        {
        	cdev_data_ptr->buffer=kmalloc(chw_buffer,GFP_KERNEL);
        	if(!cdev_data_ptr)
        	{
        		mutex_unlock(&mutex);
        		return -ENOMEM;
        	}
        }
        cdev_data_ptr->ready = 0;
        cdev_data_ptr->min = 0;
        cdev_data_ptr->max = 0;
        cdev_data_ptr->avg = 0;
		memset(cdev_data_ptr->buffer, 0, chw_buffer);
		for (i=0; i<len && i<chw_buffer; i++) {
			get_user(cdev_data_ptr->buffer[i], buff+i);
			printk( "buffer[%d]=%d\n", i,cdev_data_ptr->buffer[i]);
		}
///////////////////////
        onesec = msecs_to_jiffies(1000);
        pr_info("chardev loaded %u jiffies\n", (unsigned)onesec);
        atomic_set(&cdev_data_ptr->refcount, 3);
        if (!wq)
                wq = create_workqueue("chardev");
        if (wq) {
            queue_delayed_work(wq, &chardev_work, onesec);
            
        }
        if(!wq_min)
            wq_min= create_workqueue("chardev_min");
        if(wq_min){
            queue_delayed_work(wq_min, &chardev_work_min, onesec);
        }
        if(!wq_max)
            wq_max= create_workqueue("chardev_max");
        if(wq_max){
            queue_delayed_work(wq_max, &chardev_work_max, onesec);
        }
        if(!wq_avg)
            wq_avg= create_workqueue("chardev_avg");
        if(wq_avg){
            queue_delayed_work(wq_avg, &chardev_work_avg, onesec);
        }

        

////////////////////////














/*
	memset(Wmsg[minor], 0, BUF_LEN);
    //memset(Wmsg, 0, BUF_LEN);

	for (i=0; i<len && i< BUF_LEN; i++) {
		get_user(Wmsg[minor][i], buff+i);
	}
*/
	bytes_written = i;

//////////////////////////
//	 if (!dev->buffer) {  
//	 /* allocate the buffer */  
//	 	dev->buffer = kmalloc(chw_buffer, GFP_KERNEL);  
//	 	if (!dev->buffer) {   
//	 		mutex_unlock(&dev->mutex);   
//	 		return -ENOMEM;  
//	 	} 
//	 }...................... 
//	 printk("Accept %li bytes to %p from %p\n", (long)count, dev->buffer, buf); 
//	 if (copy_from_user(dev->buffer, buf, count)) {  
//	 	mutex_unlock (&dev->mutex);  
//	 		return -EFAULT; 
//	 	}
//////////////////////////








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
 * This function is called at the beginning of a sequence.
 * ie, when:
 *      - the /proc file is read (first time)
 *      - after the function stop (end of sequence)
 *
 */
static void *my_seq_start(struct seq_file *s, loff_t *pos)
{
    //static unsigned long counter = 0;

    /* beginning a new sequence ? */
    if ( *pos == 0 ) {
        /* yes => return a non null value to begin the sequence */
        return Wmsg[0];
    }
    else {
        /* no => it's the end of the sequence, return end to stop reading */
        *pos = 0;
        return NULL;
    }
}

/**
 * This function is called after the beginning of a sequence.
 * It's called untill the return is NULL (this ends the sequence).
 *
 */
static void *my_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
    //unsigned long *tmp_v = (unsigned long *)v;
   // char *Wmsg_ptr = (char *) v;
   // (*Wmsg_ptr)++;
    //(*pos)++;
    return NULL;
}

/**
 * This function is called at the end of a sequence
 *
 */
static void my_seq_stop(struct seq_file *s, void *v)
{
    /* nothing to do, we use a static value in start() */
}

/**
 * This function is called for each "step" of a sequence
 *
 */
static int my_seq_show(struct seq_file *s, void *v)
{
    //loff_t *spos = (loff_t *) v;
    //char *Wmsg_ptr = (char *) v;
    seq_printf(s, "  chardev=%s\n chardev1=%s\n chardev2=%s\n", Wmsg[0],Wmsg[1],Wmsg[2]);
    //seq_printf(s, "  chardev=%s\n chardev1=%s\n chardev2=%s\n", Wmsg[0],Wmsg[1],Wmsg[2]);
    return 0;
}


/**
 * This function is called when the /proc file is open.
 *
 */
static int my_open(struct inode *inode, struct file *file)
{
    return seq_open(file, &my_seq_ops);
};





///////////////////////////////procfs-end//////////////////////////////


