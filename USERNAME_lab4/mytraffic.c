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
#define INFO_BUF_SIZE 256

/* Function declarations */
static int mytraffic_init(void);
static void mytraffic_exit(void);
static int mytraffic_open(struct inode *, struct file *);
static int mytraffic_release(struct inode *, struct file *);
static ssize_t mytraffic_read(struct file *file, char *buf, size_t len, loff_t *offset);
static ssize_t mytraffic_write(struct file *, const char *, size_t, loff_t *);
static void timer_handler(struct timer_list *);

/* Operational modes enum */
typedef enum {
	NORMAL,
	FLASHING_RED,
	FLASHING_YELLOW
} mytraffic_mode;

/* Timer structure */
struct timer_entry {
	struct timer_list timer;
	struct list_head list;
};

/* Global driver variables */
static int mytraffic_major = 61;
static struct timer_entry *mytraffic_timer = NULL;
static unsigned is_green_on = 0;
static unsigned is_yellow_on = 0;
static unsigned is_red_on = 0;
static unsigned is_ped_present = 0;
static unsigned reset = 0;
static unsigned cycle_index = 0;
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

	/* Create timer */
	mytraffic_timer = kmalloc(sizeof(struct timer_entry), GFP_KERNEL);
	if (!mytraffic_timer) {
		printk(KERN_ALERT "mytraffic: Failed to allocate memory for timer.\n");
		return -ENOMEM;
	}
	timer_setup(&mytraffic_timer->timer, timer_handler, 0);

	return SUCCESS;
}

/* Module exit */
static void mytraffic_exit(void) {

	/* Unregister device */
	unregister_chrdev(mytraffic_major, DEVICE_NAME);

	/* Delete the timer if active and free its memory */
	if (mytraffic_timer) {
		if (timer_pending(&mytraffic_timer->timer)) {
			del_timer_sync(&mytraffic_timer->timer);
		}
		kfree(mytraffic_timer);
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

// if use 'cat /dev/mytraffic', output:
// current_mode
// is_red_on, is_yellow_on, is_red_on
// is_ped_present (only if pedestrian call button is supported)
static ssize_t mytraffic_read(struct file *file, char *buf, size_t len, loff_t *offset) {
	char *info;
	char *pInfo;
	unsigned info_len;
	
	ssize_t ret_len;

	info = kmalloc(INFO_BUF_SIZE, GFP_KERNEL);
	if(!info) return -ENOMEM;
	pInfo = info;

	// first print current operational mode:
	// ("normal", "flashing-red", or "flashing-yellow")
	switch(current_mode){
		case NORMAL:
			break;
		case FLASHING_RED:
			break;
		case FLASHING_YELLOW:
			break;
		default:
			break;
	}

	// next print the current cycle rate (e.g. "1 Hz")

	// next print current status of each light
	// ("red off", "yellow off", "green on")

	return len;
}

/* Handle mytraffic character device file write */
static ssize_t mytraffic_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {
	char *kernel_buf;
	char hz;

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

	kernel_buf[len - 1] = '\0';

	/* Extract and validate frequency value */
	hz = kernel_buf[0] - '0';
	if (hz < MIN_HZ || hz > MAX_HZ) {
		printk(KERN_ALERT "mytraffic: Invalid frequency value.\n");
		kfree(kernel_buf);
		return -EINVAL;
	}

	/* Update cycle rate */
	cycle_rate = hz;

	/* Restart timer with new cycle rate */
	if (mytraffic_timer) {
		mod_timer(&mytraffic_timer->timer, jiffies + msecs_to_jiffies(1 / cycle_rate));
	}

	*f_pos += len;
	kfree(kernel_buf);
	return len;
}

/* Timer callback function */
static void timer_handler(struct timer_list *timer_ptr) {
	/* Get parent structure from the timer_list pointer */
	struct timer_entry *entry = container_of(timer_ptr, struct timer_entry, timer);

	cycle_index++;

	switch(current_mode) {
		case NORMAL:
			/* Normal mode traffic light sequence */
			if (cycle_index < 3) {
				is_green_on = 1;
				is_yellow_on = 0;
				is_red_on = 0;
			} else if (cycle_index == 3) {
				is_green_on = 0;
				is_yellow_on = 1;
				is_red_on = 0;
			} else if (cycle_index > 3 && cycle_index < 6) {
				is_green_on = 0;
				is_yellow_on = 0;
				is_red_on = 1;
			}
			else if (cycle_index >= 6) {
				cycle_index = 0;
				is_green_on = 1;
				is_yellow_on = 0;
				is_red_on = 0;
			}
			break;
		case FLASHING_YELLOW:
			/* Flashing yellow mode */
			is_green_on = 0;
			is_yellow_on = !is_yellow_on;
			is_red_on = 0;
			break;
		case FLASHING_RED:
			/* Flashing red mode */
			is_green_on = 0;
			is_yellow_on = 0;
			is_red_on = !is_red_on;
			break;
	}
		is_green_on = 0;
		is_yellow_on = !is_yellow_on;
		is_red_on = 0;
	}

	

	mod_timer(&entry->timer, jiffies + msecs_to_jiffies(1 / cycle_rate));
	return;
}
