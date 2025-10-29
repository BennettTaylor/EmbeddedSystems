#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/uaccess.h>
#include <linux/jiffies.h>
#include <linux/list.h>

MODULE_LICENSE("Dual BSD/GPL");

/* Driver constants */
#define SUCCESS 0
#define DEVICE_NAME "mytimer"
#define MAX_SEC 86400
#define MAX_MSG 129
#define MAX_COUNT 5
#define INFO_BUF_SIZE 1024
#define WRITE_BUF_SIZE 256

/* Timer structure for the linked list */
struct timer_entry {
    char message[MAX_MSG];      /* The message to print on expiration */
    struct timer_list ktimer;   /* The kernel timer */
    struct list_head list;      /* Linked list node */
};

/* Global driver variables */
static LIST_HEAD(timer_list);       /* Head of the timer linked list */
static int mytimer_major = 61;      /* Major number */
static unsigned mytimer_count = 1;  /* Max number of currently supported timers (capacity) */
static unsigned mytimer_active = 0; /* Number of currently active timers */
static unsigned mytimer_is_open = 0;

/* Module function declarations */
static int mytimer_open(struct inode *, struct file *);
static int mytimer_release(struct inode *, struct file *);
static ssize_t mytimer_read(struct file *, char *, size_t, loff_t *);
static ssize_t mytimer_write(struct file *, const char *, size_t, loff_t *);
static int mytimer_init(void);
static void mytimer_exit(void);
static void mytimer_fn(struct timer_list *);
static void create_or_update_timer(unsigned long seconds, const char *message);
static void update_active_count(void);
static void change_max_timers(unsigned long new_max);

/* File access functions structure */
struct file_operations mytimer_fops = {
    .read = mytimer_read,
    .write = mytimer_write,
    .open = mytimer_open,
    .release = mytimer_release
};

/* Init and exit function declarations */
module_init(mytimer_init);
module_exit(mytimer_exit);

// ---------------------------------------------------

/* Module init */
static int mytimer_init(void) {
    int result;

    /* Register device */
    result = register_chrdev(mytimer_major, DEVICE_NAME, &mytimer_fops);
    if (result < 0) {
        printk(KERN_ALERT "mytimer: cannot obtain major number %d\n", mytimer_major);
        return result;
    }

    return SUCCESS;
}

// ---------------------------------------------------

/* Module exit */
static void mytimer_exit(void) {
    struct timer_entry *entry, *tmp;

    /* Unregister device */
    unregister_chrdev(mytimer_major, DEVICE_NAME);

    /* Delete all active timers and free their memory */
    list_for_each_entry_safe(entry, tmp, &timer_list, list) {
        del_timer_sync(&entry->ktimer);
        list_del(&entry->list);
        kfree(entry);
    }
}

// ---------------------------------------------------

/* Handle module file open */
static int mytimer_open(struct inode *inode, struct file *file) {
    /* Check if mytimer is already open */
    if (mytimer_is_open) {
        return -EBUSY;
    }
    
    /* Indicate device is open */
    mytimer_is_open++;
    
    /* Prevent module from being removed */
    try_module_get(THIS_MODULE);

    return SUCCESS;
}

// ---------------------------------------------------

/* Handle module file close */
static int mytimer_release(struct inode *inode, struct file *file) {
    /* Close mytimer */
    mytimer_is_open--;
    
    /* Allow module to be removed */
    module_put(THIS_MODULE);

    return SUCCESS;
}

// ---------------------------------------------------

/* Helper to update the active timer count */
static void update_active_count(void) {
    unsigned count = 0;
    struct timer_entry *entry;
    list_for_each_entry(entry, &timer_list, list) {
        count++;
    }
    mytimer_active = count;
}

// ---------------------------------------------------

/* Handle read from mytimer device file */
static ssize_t mytimer_read(struct file *file, char *buf, size_t len, loff_t *offset) {
    char *info;
    char *pInfo;
    unsigned info_len;
    struct timer_entry *entry;
    ssize_t ret_len;

    info = kmalloc(INFO_BUF_SIZE, GFP_KERNEL);
    if (!info) return -ENOMEM;
    pInfo = info;

    update_active_count();

    /* First print number of timers supported (capacity) */
    pInfo += sprintf(pInfo, "%u\n", mytimer_count);

    /* Print number of active timers */
    pInfo += sprintf(pInfo, "%u\n", mytimer_active);

    /* Print timer info  */
    list_for_each_entry(entry, &timer_list, list) {
        /* Print seconds from expiration */
        unsigned long expires_sec = jiffies_to_msecs(entry->ktimer.expires - jiffies) / 1000;
        pInfo += sprintf(pInfo, "%lu\n", expires_sec);

        /* Print timer message */   
        pInfo += sprintf(pInfo, "%s\n", entry->message); 
    }

    /* Terminate info buffer (not strictly needed for snprintf/sprintf but good for strlen) */
    *pInfo = '\0'; 
    
    /* Check user buffer size */
    info_len = strlen(info);

    if (*offset > info_len) {
        kfree(info);
        return 0;
    }

    ret_len = info_len - *offset;
    if (len < ret_len) {
        ret_len = len;
    }

    /* Transfering data to user space */
    if (copy_to_user(buf, info + *offset, ret_len))
    {
        kfree(info);
        return -EFAULT;
    }
    
    *offset += ret_len;
    kfree(info);
    return ret_len;
}

// ---------------------------------------------------

/* Helper to change the max number of timers (capacity) */
static void change_max_timers(unsigned long new_max) {
    struct timer_entry *entry, *tmp;
    
    if (new_max < 1 || new_max > MAX_COUNT) {
        printk(KERN_ALERT "mytimer: new max must be between 1 and %d\n", MAX_COUNT);
        return;
    }

    /* Delete extra timers if capacity is reduced */
    list_for_each_entry_safe(entry, tmp, &timer_list, list) {
        update_active_count();
        if (mytimer_active > new_max) {
            del_timer_sync(&entry->ktimer);
            list_del(&entry->list);
            kfree(entry);
        } else {
            /* Once active count is <= new_max, stop deleting */
            break; 
        }
    }
    mytimer_count = (unsigned)new_max;
}

// ---------------------------------------------------

/* Helper to create or update a timer */
static void create_or_update_timer(unsigned long seconds, const char *message) {
    struct timer_entry *entry;
    
    if (seconds == 0 || seconds > MAX_SEC) {
        printk(KERN_ALERT "mytimer: Timer duration must be between 1 and %u seconds\n", MAX_SEC);
        return;
    }

    /* Search for an existing timer with the same message */
    list_for_each_entry(entry, &timer_list, list) {
        if (strcmp(message, entry->message) == 0) {
            /* Found existing timer, update it */
            mod_timer(&entry->ktimer, jiffies + msecs_to_jiffies(seconds * 1000));
            return;
        }
    }
    
    /* Timer not found, create a new one */
    update_active_count();
    if (mytimer_active >= mytimer_count) {
        printk(KERN_ALERT "mytimer: Capacity exceeded. Max active timers is %u\n", mytimer_count);
        return;
    }

    /* Allocate and initialize new timer_entry */
    entry = kmalloc(sizeof(*entry), GFP_KERNEL);
    if (!entry) {
        printk(KERN_ALERT "mytimer: Failed to allocate memory for new timer\n");
        return;
    }
    
    /* Copy message and set up the timer */
    strncpy(entry->message, message, MAX_MSG - 1);
    entry->message[MAX_MSG - 1] = '\0'; 
    
    timer_setup(&entry->ktimer, mytimer_fn, 0);
    mod_timer(&entry->ktimer, jiffies + msecs_to_jiffies(seconds * 1000));
    
    /* Add to the list */
    list_add_tail(&entry->list, &timer_list);
}

// ---------------------------------------------------

/* Handle write to mytimer device file */
static ssize_t mytimer_write(struct file *file, const char *buf, size_t count, loff_t *offset) {
    char *kernel_buf;
    char *start_ptr;
    char *token;
    unsigned long seconds = 0;
    
    /* Prevent buffer overflow and limit copy size */
    size_t len = (count < WRITE_BUF_SIZE) ? count : WRITE_BUF_SIZE;
    
    kernel_buf = kmalloc(WRITE_BUF_SIZE, GFP_KERNEL);
    if (!kernel_buf) return -ENOMEM;

    /* Copy user buffer to kernel buffer */
    if(copy_from_user(kernel_buf, buf, len)) {
        kfree(kernel_buf);
        return -EFAULT;
    }
    kernel_buf[len] = '\0'; /* Ensure null-termination */

    start_ptr = kernel_buf;
    
    /* Look for the command and arguments separated by a newline, or space, 
       but for simplicity based on your original code, let's look for a single string. */
       
    // --- Custom parsing based on your original logic ---
    if (kernel_buf[0] != '0' && kernel_buf[0] != '\n') {
        // Assume this is a single digit for changing max count (1-5)
        unsigned long new_max = kernel_buf[0] - '0';
        change_max_timers(new_max);
        kfree(kernel_buf);
        *offset += count; // Pretend we consumed the full write
        return count;
    }
    
    // Skip the count part if it was '0' or just a newline to start timer config
    if (kernel_buf[0] == '0' && len > 1 && kernel_buf[1] == '\n') {
        start_ptr += 2; // Skip "0\n"
    } else if (kernel_buf[0] == '\n') {
        start_ptr += 1; // Skip "\n"
    } else {
        // If it's not a max-change or a timer config start, we can't process it.
        printk(KERN_ALERT "mytimer: Write format error. Expect '1'-'5' for max, or '0\\n<seconds>\\n<message>\\n'\n");
        kfree(kernel_buf);
        return -EINVAL;
    }
    
    // Parse seconds (up to the next newline)
    token = strsep(&start_ptr, "\n");
    if (!token) {
        printk(KERN_ALERT "mytimer: Write format error. Missing seconds.\n");
        kfree(kernel_buf);
        return -EINVAL;
    }

    if (kstrtoul(token, 10, &seconds) != 0) {
        printk(KERN_ALERT "mytimer: Invalid seconds format.\n");
        kfree(kernel_buf);
        return -EINVAL;
    }

    // Parse message (up to the next newline)
    token = strsep(&start_ptr, "\n");
    if (!token) {
        printk(KERN_ALERT "mytimer: Write format error. Missing message.\n");
        kfree(kernel_buf);
        return -EINVAL;
    }
    
    if (strlen(token) >= MAX_MSG) {
        printk(KERN_ALERT "mytimer: Message too long (max %d chars).\n", MAX_MSG - 1);
        kfree(kernel_buf);
        return -EINVAL;
    }

    create_or_update_timer(seconds, token);
    
    kfree(kernel_buf);
    *offset += count;
    return count;
}

// ---------------------------------------------------

/* Timer expiration handler */
static void mytimer_fn(struct timer_list *timer_ptr) {
    /* Get parent structure from the timer_list pointer */
    struct timer_entry *entry = container_of(timer_ptr, struct timer_entry, ktimer);
    
    pr_info("%s\n", entry->message);
    
    /* Remove the entry from the list and free memory */
    list_del(&entry->list);
    kfree(entry);
    
    update_active_count();
}
