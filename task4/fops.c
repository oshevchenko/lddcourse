#include "task4.h"

#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>
#include <linux/poll.h>
#include <linux/wait.h>

static int actual_major_num = -1;

struct number_work_t
{
    struct work_struct work;
    void (*process)(struct number_work_t* wk);
    int result;
    int count;
};

static struct number_work_t *min_wk, *avg_wk, *max_wk;
static wait_queue_head_t result_queue, write_queue;
static atomic_t jobs_left;

static char* in_buf;
static size_t in_buf_size;
static size_t in_data_size;
static atomic_t in_buf_rc;
//DEFINE_MUTEX(in_buf_mutex);

static char out_buf[3];
//DEFINE_MUTEX(out_buf_mutex);


static void work_func(struct work_struct* wk)
{
    struct number_work_t* work = container_of(wk, struct number_work_t, work);
    work->process(work);

    if (atomic_dec_and_test(&in_buf_rc))
    {
        wake_up_interruptible(&write_queue);
    }

    if (atomic_dec_and_test(&jobs_left))
    {
        out_buf[0] = min_wk->result;
        out_buf[1] = avg_wk->result / avg_wk->count;
        out_buf[2] = max_wk->result;

        wake_up_interruptible(&result_queue);
    }
}

static void process_min(struct number_work_t* wk)
{
    size_t i;
    for (i = 0; i < in_data_size; ++i)
    {
        if (wk->result > in_buf[i])
            wk->result = in_buf[i];
    }
}

static void process_avg(struct number_work_t* wk)
{
    size_t i;
    for (i = 0; i < in_data_size; ++i)
    {
        // int32 can sum up to 16777216 bytes to calculate their average
        wk->result += in_buf[i];
    }
    wk->count += in_data_size;
}

static void process_max(struct number_work_t* wk)
{
    size_t i;
    for (i = 0; i < in_data_size; ++i)
    {
        if (wk->result < in_buf[i])
            wk->result = in_buf[i];
    }
}

ssize_t file_read(struct file* filp, char __user* buf, size_t count, loff_t* pos)
{
    int retval = 0;

    // Wait for all workers to complete
    wait_event_interruptible(result_queue, atomic_read(&jobs_left) == 0);

    if (*pos >= sizeof(out_buf))
    {
        printk(KERN_DEBUG TAG "End of buffer\n");
        return 0;
    }

    if (count + *pos >= sizeof(out_buf))
        count = sizeof(out_buf) - *pos - 1;

    /*retval = mutex_lock_interruptible(&out_buf_mutex);
    if (retval != 0)
    {
        printk(KERN_NOTICE TAG "Mutex lock interrupted\n");
        return retval; // -EINTR
    }*/

    if (copy_to_user(buf, out_buf, count))
    {
        printk(KERN_NOTICE TAG "Failed copying buffer to user space\n");
        return -ERESTARTSYS;
    }

    //mutex_unlock(&out_buf_mutex);

    *pos += count;
    retval = count;

    printk(KERN_DEBUG TAG "Copied to user: pos=%lld count=%lu [%d,%d,%d]\n",
           *pos, count, (int)out_buf[0], (int)out_buf[1], (int)out_buf[2]);

    return retval;
}

ssize_t file_write(struct file* filp, const char __user* buf, size_t count, loff_t* pos)
{
    size_t copied_bytes;
    //int retval = mutex_lock_interruptible(&in_buf_mutex);
    //if (retval != 0)
    //    return retval;

    atomic_add(3, &jobs_left);

    printk(KERN_DEBUG TAG "file_write: jobs_left=%d in_buf_rc=%d\n",
           atomic_read(&jobs_left), atomic_read(&in_buf_rc));

    // Wait for all jobs to finish their previous work on last portion of data
    wait_event_interruptible(write_queue, atomic_read(&in_buf_rc) == 0);

    atomic_add(3, &in_buf_rc);

    copied_bytes = in_data_size = min(in_buf_size, count);
    copy_from_user(in_buf, buf, copied_bytes);

    printk(KERN_DEBUG TAG "file_write: in_buf_rc=%d copied=%lu\n",
           atomic_read(&in_buf_rc), copied_bytes);

    *pos += copied_bytes;

    // use system queue for jobs
    if (!schedule_work(&min_wk->work))
    {
        printk(KERN_ERR TAG "Work min already in work queue.\n");
    }
    if (!schedule_work(&avg_wk->work))
    {
        printk(KERN_ERR TAG "Work avg already in work queue.\n");
    }
    if (!schedule_work(&max_wk->work))
    {
        printk(KERN_ERR TAG "Work max already in work queue.\n");
    }

    //mutex_unlock(&in_buf_mutex);

    return copied_bytes; /* succeed, to avoid retrial */
}

unsigned int file_poll(struct file* filp, struct poll_table_struct* poll_table)
{
    unsigned int mask = 0;

    poll_wait(filp, &result_queue, poll_table);

    mask |= POLLIN | POLLRDNORM;	/* readable */
    mask |= POLLOUT | POLLWRNORM;	/* writable */

    return mask;
}

int init_device(unsigned int major_num, size_t in_buf_sz)
{
    struct file_operations fops = {
        .owner   = THIS_MODULE,
        .read    = file_read,
        .write   = file_write,
        .poll    = file_poll
    };

    in_buf_size = in_buf_sz;
    in_buf = kmalloc(in_buf_size, GFP_KERNEL);
    if (!in_buf)
    {
        printk(KERN_ERR TAG "No memory for input buffer\n");
        return -ENOMEM;
    }
    printk(KERN_DEBUG TAG "Created input buffer (%lu bytes) 0x%p\n", in_buf_size, in_buf);

    actual_major_num = register_chrdev(major_num, PROG_NAME, &fops);
    if (actual_major_num < 0)
    {
        printk(KERN_ERR TAG "Unable to register device for /dev/" PROG_NAME " with major=%d\n",
               major_num);
        return -ERESTARTSYS;
    }

    atomic_set(&jobs_left, 0);
    atomic_set(&in_buf_rc, 0);

    init_waitqueue_head(&result_queue);
    init_waitqueue_head(&write_queue);

    // Alloc zeroed memory for structs
    min_wk = kcalloc(3, sizeof(struct number_work_t), GFP_KERNEL);
    avg_wk = min_wk + 1;
    max_wk = min_wk + 2;
    if (!min_wk)
    {
        printk(KERN_ERR TAG "No memory for work structs\n");
        return -ENOMEM;
    }

    INIT_WORK(&min_wk->work, work_func);
    min_wk->process = process_min;
    INIT_WORK(&avg_wk->work, work_func);
    avg_wk->process = process_avg;
    INIT_WORK(&max_wk->work, work_func);
    max_wk->process = process_max;

    printk(KERN_DEFAULT TAG "Registered device for /dev/" PROG_NAME " with major=%d\n",
           actual_major_num);

    return 0;
}

void deinit_device(void)
{
    kfree(in_buf);
    kfree(min_wk);

    if (actual_major_num >= 0)
    {
        unregister_chrdev(actual_major_num, PROG_NAME);
    }
}
