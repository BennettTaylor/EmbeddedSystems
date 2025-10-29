#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/fcntl.h>
#include <linux/jiffies.h>
#include <linux/uaccess.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/string.h>
#include <linux/seq_file.h>

MODULE_LICENSE("Dual BSD/GPL");

/* Driver constants */
#define SUCCESS 0
#define DEVICE_NAME "mytimer"
#define MAX_SEC 86400
#define MAX_MSG 129
#define MAX_COUNT 2
#define WRITE_BUF_SIZE 256

/* Function declarations */
static int mytimer_init(void);
static void mytimer_exit(void);
static int mytimer_dev_open(struct inode *, struct file *);
static int mytimer_dev_release(struct inode *, struct file *);
static ssize_t mytimer_dev_write(struct file *, const char *, size_t, loff_t *);
static int mytimer_dev_fasync(int, struct file *, int);
static int mytimer_proc_open(struct inode *, struct file *);
static int mytimer_proc_show(struct seq_file *, void *);
static void timer_handler(struct timer_list *);

/* Timer structure */
struct timer_entry {
	int is_active;
	char message[MAX_MSG];
	struct task_struct *calling_process;
	struct fasync_struct *async_queue;
	struct timer_list timer;
	struct list_head list;
};

/* Global driver variables */
static int mytimer_major = 61;
static LIST_HEAD(timer_list);
static struct proc_dir_entry *proc_entry;
static long unsigned mytimer_load_time;
static unsigned mytimer_count = 1;
static unsigned mytimer_active = 0;

/* Character device file operations */
struct file_operations mytimer_dev_fops = {
owner:
	THIS_MODULE,
write:
	mytimer_dev_write,
open:
	mytimer_dev_open,
release:
	mytimer_dev_release,
fasync:
	mytimer_dev_fasync
};

/* Proc file operations */
static const struct file_operations mytimer_proc_fops = {
owner:
	THIS_MODULE,
open:
	mytimer_proc_open,
read:
	seq_read,
llseek:
	seq_lseek,
release:
	single_release,
};

/* Declaration of the init and exit functions */
module_init(mytimer_init);
module_exit(mytimer_exit);

/* Module init */
static int mytimer_init(void) {
	int result;

	/* Mark when module was loaded */
	mytimer_load_time = jiffies;

	/* Register device */
	result = register_chrdev(mytimer_major, DEVICE_NAME, &mytimer_dev_fops);
	if (result < 0) {
		printk(KERN_ALERT "mytimer: cannot obtain major number. %d\n", mytimer_major);
		return result;
	}

	/* Create proc entry */
	proc_entry = proc_create(DEVICE_NAME, 0644, NULL, &mytimer_proc_fops);
	if (proc_entry == NULL) {
		printk(KERN_INFO "mytimer: Couldn't create proc entry.\n");
		return -ENOMEM;
        }

	return SUCCESS;
}

/* Module exit */
static void mytimer_exit(void) {
	struct timer_entry *entry, *tmp;

	/* Unregister device */
	unregister_chrdev(mytimer_major, DEVICE_NAME);

	/* Remove proc entry */
	remove_proc_entry(DEVICE_NAME, NULL);

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

/* Handle mytimer character device file open */
static int mytimer_dev_open(struct inode *inode, struct file *filp) {
	/* Prevent module from being removed */
	try_module_get(THIS_MODULE);

	return SUCCESS;
}

/* Handle mytimer character device file close */
static int mytimer_dev_release(struct inode *inode, struct file *filp) {
	/* Allow module to be removed */
	module_put(THIS_MODULE);

	return SUCCESS;
}

/* Handle mytimer character device file write */
static ssize_t mytimer_dev_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {
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
		printk(KERN_ALERT "mytimer: Failed to allocate memory for write buffer.\n");
		return -ENOMEM;
	}

	/* Copy user buffer to kernel buffer */
	if(copy_from_user(kernel_buf, buf, len)) {
		printk(KERN_ALERT "mytimer: Failed copy user buffer to kernel buffer.\n");
		kfree(kernel_buf);
		return -EFAULT;
	}
	kernel_buf[len] = '\0';

	kernel_buf_ptr = kernel_buf;

	/* If the buffer only contains null character, remove all timers */
	if (kernel_buf[0] == '\0') {
		list_for_each_entry_safe(entry, tmp, &timer_list, list) {
                	if (entry->is_active) {
				mytimer_active--;
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
			printk(KERN_ALERT "mytimer: The number of supported timers must be between 1 and %d.\n", MAX_COUNT);
			return -EPERM;
		}

		/* Set new count */
		mytimer_count = new_count;

		/* End write */
		kfree(kernel_buf);
		*f_pos += count;
		return count;
	}

	/* Parse input from this format: '0\n<seconds>\n<message>\n' */
	if (kernel_buf[1] != '\n') {
		printk(KERN_ALERT "mytimer: Write format error. Expect '1'-'5' for max, or '0\n<seconds>\n<message>\n' to set new timer.\n");
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
		printk(KERN_ALERT "mytimer: Invalid seconds format.\n");
		kfree(kernel_buf);
		return -EINVAL;
	}

	/* Check seconds bounds */
	if (seconds <= 0 || seconds > MAX_SEC) {
		printk(KERN_ALERT "mytimer: Timer duration must be between 1 and %u seconds.\n", MAX_SEC);
		return -EPERM;
	}

	/* Parse message */
	token = strsep(&kernel_buf_ptr, "\n");
	if (!token) {
		printk(KERN_ALERT "mytimer: Write format error. Expect '1'-'5' for max, or '0\n<seconds>\n<message>\n' to set new timer.\n");
		kfree(kernel_buf);
		return -EINVAL;
	}

	/* Check message length */
	if (strlen(token) >= MAX_MSG) {
		printk(KERN_ALERT "mytimer: Message too long (max %d chars).\n", MAX_MSG - 1);
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
	if (mytimer_active >= mytimer_count) {
		printk(KERN_ALERT "mytimer: Capacity exceeded. Max active timers is %u.\n", mytimer_count);
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
			mytimer_active++;
                        mod_timer(&entry->timer, jiffies + msecs_to_jiffies(seconds * 1000));

                        /* End write */
                        kfree(kernel_buf);
                        *f_pos += count;
                        return count;
                }
        }

	/* Signal handler was not found */
	printk(KERN_ALERT "mytimer: A signal handler must be instantiated before registering timer.\n");
	kfree(kernel_buf);
	return -EINVAL;
}

/* Register new signal handler */
static int mytimer_dev_fasync(int fd, struct file *filp, int mode) {
	/* Create new timer entry */
	struct timer_entry *entry;
	int ret = 0;

	/* Clean up if file is closed */
	if (!mode) {
		/* Find correct entry */
		list_for_each_entry(entry, &timer_list, list) {
                	if (!entry->is_active && entry->calling_process == current) {
				ret = fasync_helper(fd, filp, mode, &entry->async_queue);
				return ret;
                	}
        	}

		return ret;
	}
	
	/* Don't allocate a new entry count would be exceeded, check for matching message later  */
	if (mytimer_active >= mytimer_count) {
		return SUCCESS;
	}

	/* Allocate new entry */
	entry = kmalloc(sizeof(*entry), GFP_KERNEL);
	if (!entry) {
		printk(KERN_ALERT "mytimer: Failed to allocate memory for new timer.\n");
		return -ENOMEM;
	}

	/* Set up timer entry */
	entry->calling_process = current;
	timer_setup(&entry->timer, timer_handler, 0);
	entry->is_active = 0;
	entry->async_queue = NULL;
	list_add_tail(&entry->list, &timer_list);

	/* Set up async queue */
	return fasync_helper(fd, filp, mode, &entry->async_queue);
}

/* Handle proc entry open */
static int mytimer_proc_open(struct inode *inode, struct file *filp) {
	return single_open(filp, mytimer_proc_show, NULL);
}

/* Format proc entry output */
static int mytimer_proc_show(struct seq_file *m, void *v) {
	struct timer_entry *entry, *tmp;	
	
	/* Print module information */
	seq_printf(m, "%s\n", DEVICE_NAME);
	seq_printf(m, "%u\n", jiffies_to_msecs(jiffies - mytimer_load_time));
	seq_printf(m, "%u\n", mytimer_active);
	seq_printf(m, "%u\n", mytimer_count);

	/* Print active entries */
	list_for_each_entry_safe(entry, tmp, &timer_list, list) {
		if (entry->is_active) {
			seq_printf(m, "%d\n", entry->calling_process->pid);
			seq_printf(m, "%s\n", entry->calling_process->comm);
			seq_printf(m, "%u\n", jiffies_to_msecs(entry->timer.expires - jiffies) / 1000);
			seq_printf(m, "%s\n", entry->message);
		}
	}

	return SUCCESS;
}

/* Timer callback function */
static void timer_handler(struct timer_list *timer_ptr) {
	/* Get parent structure from the timer_list pointer */
	struct timer_entry *entry = container_of(timer_ptr, struct timer_entry, timer);

	/* Send SIGIO signal to timer calling process */
	if (entry->async_queue) {
		entry->is_active = 0;
		kill_fasync(&entry->async_queue, SIGIO, POLL_IN);
	}

	/* Remove timer entry */
	mytimer_active--;
	if(&entry->list) {
		list_del(&entry->list);
	} else {
		printk(KERN_ALERT "mytimer: List not found in timer callback.\n");
	}

	if (entry) {
		kfree(entry);
	} else {
		printk(KERN_ALERT "mytimer: Entry not found in timer callback.\n");
	}

	return;
}
