#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/jiffies.h>
#include <linux/uaccess.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/string.h>

MODULE_LICENSE("Dual BSD/GPL");

/* Driver constants */
#define SUCCESS 0
#define DEVICE_NAME "mytraffic"
#define MAX_HZ 9
#define MIN_HZ 1
#define WRITE_BUF_SIZE 2

/* Function declarations */
static int mytraffic_init(void);
static void mytraffic_exit(void);
static int mytraffic_open(struct inode *, struct file *);
static int mytraffic_release(struct inode *, struct file *);
static ssize_t mytraffic_read(struct file *file, char *buf, size_t len, loff_t *offset);
static ssize_t mytraffic_write(struct file *, const char *, size_t, loff_t *);
static void timer_handler(struct timer_list *);

/* Operational modes enum */
enum mytraffic_mode {
	NORMAL,
	FLASHING_RED,
	FLASHING_YELLOW
};

/* Timer structure */
struct timer_entry {
	struct timer_list timer;
	struct list_head list;
};

/* Global driver variables */
static int mytraffic_major = 61;
static LIST_HEAD(timer_list);
static unsigned is_green_on = 0;
static unsigned is_yellow_on = 0;
static unsigned is_red_on = 0;
static unsigned is_ped_present = 0;
static unsigned reset = 0;
static unsigned cycle_rate = MIN_HZ;
static mytraffic_mode current_mode = NORMAL;


/* Character device file operations */
struct file_operations mytraffic_dev_fops = {
owner:
	THIS_MODULE,
read:
	mytraffic_read,
write:
	mytraffic_write,
open:
	mytraffic_open,
release:
	mytraffic_release,
};

/* Declaration of the init and exit functions */
module_init(mytraffic_init);
module_exit(mytraffic_exit);

/* Module init */
static int mytraffic_init(void) {
	int result;

	/* Register device */
	result = register_chrdev(mytraffic_major, DEVICE_NAME, &mytraffic_dev_fops);
	if (result < 0) {
		printk(KERN_ALERT "mytraffic: cannot obtain major number. %d\n", mytraffic_major);
		return result;
	}

	return SUCCESS;
}

/* Module exit */
static void mytraffic_exit(void) {
	struct timer_entry *entry, *tmp;

	/* Unregister device */
	unregister_chrdev(mytraffic_major, DEVICE_NAME);

	/* Delete all active timers and free their memory */
	list_for_each_entry_safe(entry, tmp, &timer_list, list) {
		if (entry->is_active) {
			del_timer_sync(&entry->timer);
		}
		list_del(&entry->list);
		kfree(entry);
	}

	return;
}

/* Handle mytraffic character device file open */
static int mytraffic_open(struct inode *inode, struct file *filp) {
	/* Prevent module from being removed */
	try_module_get(THIS_MODULE);

	return SUCCESS;
}

/* Handle mytraffic character device file close */
static int mytraffic_release(struct inode *inode, struct file *filp) {
	/* Allow module to be removed */
	module_put(THIS_MODULE);

	return SUCCESS;
}

static ssize_t mytraffic_read(struct file *file, char *buf, size_t len, loff_t *offset) {
	return len;
}

/* Handle mytraffic character device file write */
static ssize_t mytraffic_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {
	char *kernel_buf;
	char *token;
	char* kernel_buf_ptr;
	unsigned long seconds = 0;
	struct timer_entry *entry, *tmp;

	/* Prevent buffer overflow and limit copy size */
	size_t len = (count < WRITE_BUF_SIZE) ? count : WRITE_BUF_SIZE;

	/* Allocate kernel buffer */
	kernel_buf = kmalloc(WRITE_BUF_SIZE, GFP_KERNEL);
	if (!kernel_buf) {
		printk(KERN_ALERT "mytraffic: Failed to allocate memory for write buffer.\n");
		return -ENOMEM;
	}

	/* Copy user buffer to kernel buffer */
	if(copy_from_user(kernel_buf, buf, len)) {
		printk(KERN_ALERT "mytraffic: Failed copy user buffer to kernel buffer.\n");
		kfree(kernel_buf);
		return -EFAULT;
	}
	kernel_buf[len] = '\0';

	kernel_buf_ptr = kernel_buf;

	/* If the buffer only contains null character, remove all timers */
	if (kernel_buf[0] == '\0') {
		list_for_each_entry_safe(entry, tmp, &timer_list, list) {
                	if (entry->is_active) {
				mytraffic_active--;
                        	del_timer_sync(&entry->timer);
                	}
                	list_del(&entry->list);
                	kfree(entry);
		}
		
		/* End write */
                kfree(kernel_buf);
                *f_pos += count;
                return count;
	}

	/* Check if changing number of timers supported */
	if (kernel_buf[0] != '0') {
		unsigned new_count = kernel_buf[0] - '0';
		if (new_count < 1 || new_count > MAX_COUNT) {
			printk(KERN_ALERT "mytraffic: The number of supported timers must be between 1 and %d.\n", MAX_COUNT);
			return -EPERM;
		}

		/* Set new count */
		mytraffic_count = new_count;

		/* End write */
		kfree(kernel_buf);
		*f_pos += count;
		return count;
	}

	/* Parse input from this format: '0\n<seconds>\n<message>\n' */
	if (kernel_buf[1] != '\n') {
		printk(KERN_ALERT "mytraffic: Write format error. Expect '1'-'5' for max, or '0\n<seconds>\n<message>\n' to set new timer.\n");
		kfree(kernel_buf);
		return -EINVAL;
	}

	kernel_buf_ptr += 2;

	/* Parse seconds */
	token = strsep(&kernel_buf_ptr, "\n");
	if (!token) {
		printk(KERN_ALERT "Write format error. Expect '1'-'5' for max, or '0\n<seconds>\n<message>\n' to set new timer.\n");
		kfree(kernel_buf);
		return -EINVAL;
	}

	/* Get seconds */
	if (kstrtoul(token, 10, &seconds) != 0) {
		printk(KERN_ALERT "mytraffic: Invalid seconds format.\n");
		kfree(kernel_buf);
		return -EINVAL;
	}

	/* Check seconds bounds */
	if (seconds <= 0 || seconds > MAX_SEC) {
		printk(KERN_ALERT "mytraffic: Timer duration must be between 1 and %u seconds.\n", MAX_SEC);
		return -EPERM;
	}

	/* Parse message */
	token = strsep(&kernel_buf_ptr, "\n");
	if (!token) {
		printk(KERN_ALERT "mytraffic: Write format error. Expect '1'-'5' for max, or '0\n<seconds>\n<message>\n' to set new timer.\n");
		kfree(kernel_buf);
		return -EINVAL;
	}

	/* Check message length */
	if (strlen(token) >= MAX_MSG) {
		printk(KERN_ALERT "mytraffic: Message too long (max %d chars).\n", MAX_MSG - 1);
		kfree(kernel_buf);
		return -EINVAL;
	}

	/* Update timer if message already registered */
	list_for_each_entry(entry, &timer_list, list) {
		if (entry->is_active && strcmp(token, entry->message) == 0) {
			/* Found existing timer, update it */
			mod_timer(&entry->timer, jiffies + msecs_to_jiffies(seconds * 1000));
		
			/* End write */
        		kfree(kernel_buf);
        		*f_pos += count;
        		return count;
		}
	}

	/* If new timer would exceed max, exit */
	if (mytraffic_active >= mytraffic_count) {
		printk(KERN_ALERT "mytraffic: Capacity exceeded. Max active timers is %u.\n", mytraffic_count);
		kfree(kernel_buf);
		return -EPERM;
	}

	/* Check if calling process has registered a signal handler */
	list_for_each_entry(entry, &timer_list, list) {
		if (!entry->is_active && entry->calling_process == current) {
                        /* Found signal handler */
			strncpy(entry->message, token, MAX_MSG - 1);
			entry->message[MAX_MSG - 1] = '\0';
			entry->is_active = 1;
			mytraffic_active++;
                        mod_timer(&entry->timer, jiffies + msecs_to_jiffies(seconds * 1000));

                        /* End write */
                        kfree(kernel_buf);
                        *f_pos += count;
                        return count;
                }
        }

	/* Signal handler was not found */
	printk(KERN_ALERT "mytraffic: A signal handler must be instantiated before registering timer.\n");
	kfree(kernel_buf);
	return -EINVAL;
}

/* Timer callback function */
static void timer_handler(struct timer_list *timer_ptr) {
	/* Get parent structure from the timer_list pointer */
	struct timer_entry *entry = container_of(timer_ptr, struct timer_entry, timer);

	return;
}
