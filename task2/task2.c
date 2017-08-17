#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/mutex.h>
#include <linux/stacktrace.h>
#include <linux/slab.h>
#include <linux/mutex.h>

#include <asm/uaccess.h>

MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Creates char device and calculates hash of the echoed text into it.");

static uint major_num;
module_param(major_num, uint, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(major_num, "Pass desirable major number for device.");

#define PROG_NAME "task2"
#define TAG PROG_NAME ": "

DEFINE_MUTEX(init_mutex);
DEFINE_MUTEX(out_buf_mutex);

static int actual_major_num = -1;

struct task_backtrace
{
    int pid;
    char name[256];
    struct stack_trace trace;
};

static const uint bt_frames_count = 32;
static struct task_backtrace* last_bt;

static const size_t out_buf_size = 4096;
static char* out_buf;

const char* get_backtrace_string(struct task_backtrace* bt, size_t* buf_len)
{
    static int last_pid = 0;
    int len = 0;

    if (!bt || !out_buf || !bt->pid)
        return NULL;

    printk(KERN_DEBUG TAG "%s last_pid=%d pid=%d trace.entries=%d (0x%p)\n",
           __FUNCTION__, last_pid, bt->pid, bt->trace.nr_entries, bt->trace.entries);

    if (last_pid != bt->pid && bt->trace.nr_entries > 0)
    {
        last_pid = bt->pid;
        len = snprintf(out_buf, out_buf_size, "Backtrace for '%s' [%d]:\n", bt->name, bt->pid);
        if (len >= out_buf_size)
            return NULL;
        len = snprint_stack_trace(out_buf + len, out_buf_size - len, &bt->trace, 2);
        strcat(out_buf, "\n");
        printk(KERN_DEBUG TAG "%s snprint_stack_trace: len=%d\n", __FUNCTION__, len);
    }

    *buf_len = strlen(out_buf);

    return out_buf;
}

int file_open(struct inode* node, struct file* filp)
{
    return 0;
}

int file_release(struct inode* node, struct file* filp)
{
    return 0;
}

ssize_t file_read(struct file* filp, char __user* buf, size_t count, loff_t* pos)
{
    const char* bt_buf;
    size_t bt_buf_len = 0;
    int retval;

    retval = mutex_lock_interruptible(&out_buf_mutex);
    if (retval != 0)
    {
        printk(KERN_NOTICE TAG "Mutex lock interrupted\n");
        return retval;
    }

    printk(KERN_DEBUG TAG "%s: last_bt=0x%p\n", __FUNCTION__, last_bt);

    bt_buf = get_backtrace_string(last_bt, &bt_buf_len);
    if (*pos >= bt_buf_len)
    {
        printk(KERN_DEBUG TAG "End of buffer for pid=%d\n", last_bt->pid);
        goto finish;
    }

    if (count + *pos >= bt_buf_len)
        count = bt_buf_len - *pos - 1;

    printk(KERN_DEBUG TAG "out_buf_size=%lu pos=%lld count=%lu\n", bt_buf_len, *pos, count);

    if (copy_to_user(buf, out_buf + *pos, count) != 0)
    {
        printk(KERN_ERR TAG "Failed copying data to user space\n");
        retval = -EFAULT;
        goto finish;
    }

    *pos += count;
    retval = count;

    printk(KERN_DEBUG TAG "Copied to user: pos=%lld count=%lu\n", *pos, count);

finish:
    mutex_unlock(&out_buf_mutex);

    return retval;
}

ssize_t file_write(struct file* filp, const char __user* buf, size_t count, loff_t* pos)
{
    int retval = mutex_lock_interruptible(&out_buf_mutex);
    if (retval != 0)
        return retval;

    last_bt->pid = current->pid;
    last_bt->trace.nr_entries = 0;
    get_task_comm(last_bt->name, current);
    save_stack_trace_tsk(current, &last_bt->trace);

    printk(KERN_DEBUG TAG "Saved backtrace of the task pid=%d (%s)\n", current->pid, current->comm);

    mutex_unlock(&out_buf_mutex);

    return count; /* succeed, to avoid retrial */
}

struct file_operations fops = {
    .read    = file_read,
    .write   = file_write,
    .open    = file_open,
    .release = file_release,
    .owner   = THIS_MODULE
};

// __init/__exit ommits function if compiled into kernel
static int __init my_module_init(void)
{
    int result = mutex_lock_interruptible(&init_mutex);
    if (result != 0)
        return result;

    if (actual_major_num >= 0)
    {
        printk(KERN_NOTICE TAG "Module already initialized. major=%d\n",
               actual_major_num);
        mutex_unlock(&init_mutex);
        return 0;
    }

    printk(KERN_DEBUG TAG "Initializing module. major=%u\n", major_num);

    out_buf = kcalloc(1, out_buf_size, GFP_KERNEL);
    if (!out_buf)
    {
        printk(KERN_ERR TAG "No memory for out buffer\n");
        mutex_unlock(&init_mutex);
        return -ENOMEM;
    }
    printk(KERN_DEBUG TAG "Created memory buffer (%lu bytes) 0x%p\n", out_buf_size, out_buf);

    last_bt = kcalloc(1, sizeof(struct task_backtrace), GFP_KERNEL);
    if (!last_bt)
    {
        printk(KERN_ERR TAG "No memory for last_bt struct\n");
        mutex_unlock(&init_mutex);
        return -ENOMEM;
    }
    last_bt->trace.nr_entries = 0;
    last_bt->trace.max_entries = bt_frames_count;
    last_bt->trace.skip = 0;
    last_bt->trace.entries = kcalloc(bt_frames_count, sizeof(unsigned long), GFP_KERNEL);
    if (!last_bt->trace.entries)
    {
        printk(KERN_ERR TAG "No memory for stack frames\n");
        mutex_unlock(&init_mutex);
        return -ENOMEM;
    }
    printk(KERN_DEBUG TAG "Created backtrace struct 0x%p with %u frames\n",
           last_bt, bt_frames_count);

    actual_major_num = register_chrdev(major_num, PROG_NAME, &fops);
    mutex_unlock(&init_mutex);

    if (actual_major_num < 0)
    {
        printk(KERN_ERR TAG "Unable to register device for /dev/" PROG_NAME " with major=%d\n",
               major_num);
        return actual_major_num;
    }

    printk(KERN_DEFAULT TAG "Registered device for /dev/" PROG_NAME " with major=%d\n",
           actual_major_num);
    return 0;
}

static void __exit my_module_exit(void)
{
    printk(KERN_DEFAULT TAG "Goodbye Kernel. Freeing resources.\n");

    kfree(out_buf);
    kfree(last_bt->trace.entries);
    kfree(last_bt);

    if (actual_major_num > 0)
        unregister_chrdev(actual_major_num, PROG_NAME);
}

module_init(my_module_init)
module_exit(my_module_exit)
