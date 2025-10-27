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
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>

MODULE_LICENSE("Dual BSD/GPL");

/* Driver constants */
#define SUCCESS 0
#define DEVICE_NAME "mytraffic"
#define MAX_HZ 9
#define MIN_HZ 1
#define WRITE_BUF_SIZE 2
#define INFO_BUF_SIZE 256
#define GRN 44
#define YLW 68
#define RED 67
#define BTN0 26
#define BTN1 46
#define DEBOUNCE_TIME 50 // debounce time for buttons (in ms)

/* Function declarations */
static int mytraffic_init(void);
static void mytraffic_exit(void);
static int mytraffic_open(struct inode *, struct file *);
static int mytraffic_release(struct inode *, struct file *);
static ssize_t mytraffic_read(struct file *file, char *buf, size_t len, loff_t *offset);
static ssize_t mytraffic_write(struct file *, const char *, size_t, loff_t *);
static void timer_handler(struct timer_list *);
static irqreturn_t interrupt_handler(int, void *);
static void debounce_timer_handler(struct timer_list *);

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

/* Debounce timer structure */
static struct timer_list debounce_timer; // for debouncing buttons

/* GPIO structure */
struct gpio {
        unsigned        gpio;
        unsigned long   flags;
        const char      *label;
};

/* Global driver variables */
static unsigned bulb_check = 0; // for extra "lightbulb check"
static int mytraffic_major = 61;
static struct timer_entry *mytraffic_timer = NULL;
static unsigned is_green_on = 0;
static unsigned is_yellow_on = 0;
static unsigned is_red_on = 0;
static unsigned is_ped_present = 0;
static unsigned cycle_index_ped = 0;
static unsigned reset = 0;
static unsigned cycle_index = 0;
static unsigned cycle_rate = MIN_HZ;
static mytraffic_mode current_mode = NORMAL;
static size_t num_led_gpios = 3;
static struct gpio led_gpios[] = {
        { GRN, GPIOF_OUT_INIT_LOW,  "Green LED" },
        { YLW, GPIOF_OUT_INIT_LOW,  "Yellow LED"   },
        { RED, GPIOF_OUT_INIT_LOW,  "Red LED"  }
};
static size_t num_button_gpios = 2;
static struct gpio button_gpios[] = {
        { BTN0, GPIOF_DIR_IN, "Button 0" },
        { BTN1, GPIOF_DIR_IN,  "Button 1" }
};
static unsigned is_button0_pressed = 0;
static unsigned is_button1_pressed = 0;
static void *button0_dev_id = (void *)"button0";
static void *button1_dev_id = (void *)"button1";


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
	int button0_irq;
	int button1_irq;

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
	/* Create debouncer timer */
	timer_setup(&debounce_timer, debounce_timer_handler, 0);

	/* Request LED GPIOs */
	result = gpio_request_array(led_gpios, num_led_gpios);
	if (result < 0) {
		printk(KERN_ALERT "mytraffic: can't obtain LED GPIO pins.\n");
		return result;
	}

	/* Request button GPIOs */
	result = gpio_request_array(button_gpios, num_button_gpios);
	if (result < 0) {
		printk(KERN_ALERT "mytraffic: can't obtain button GPIO pins.\n");
		return result;
	}

	/* Set up button interupt handling */
	button0_irq = gpio_to_irq(button_gpios[0].gpio);
	button1_irq = gpio_to_irq(button_gpios[1].gpio);

	result = request_irq(button0_irq, interrupt_handler, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "button0_irq", button0_dev_id);
	if (result < 0) {
		printk(KERN_ALERT "mytraffic: can't obtain button 0 IRQ.\n");
		return result;
	}

	result = request_irq(button1_irq, interrupt_handler, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "button1_irq", button1_dev_id);
	if (result < 0) {
		printk(KERN_ALERT "mytraffic: can't obtain button 0 IRQ.\n");
		return result;
	}

	is_green_on  = 1;
	is_yellow_on = 0;
	is_red_on    = 0;

	gpio_set_value(led_gpios[0].gpio, is_green_on);
	gpio_set_value(led_gpios[1].gpio, is_yellow_on);
	gpio_set_value(led_gpios[2].gpio, is_red_on);

	mod_timer(&mytraffic_timer->timer, jiffies + msecs_to_jiffies(1000 / cycle_rate));

	return SUCCESS;
}

/* Module exit */
static void mytraffic_exit(void) {
	int button0_irq;
	int button1_irq;

	/* Unregister device */
	unregister_chrdev(mytraffic_major, DEVICE_NAME);

	/* Delete the timer if active and free its memory */
	if (mytraffic_timer) {
		if (timer_pending(&mytraffic_timer->timer)) {
			del_timer_sync(&mytraffic_timer->timer);
		}
		kfree(mytraffic_timer);
	}

	/* Delete the debouncer timer if active and free its memory */
	if(debounce_timer){
		if(timer_pending(&debounce_timer->timer)){
			del_timer_sync(&debounce_timer->timer);
		}
		kfree(debounce_timer);
	}

	/* Release interupt lines */
	button0_irq = gpio_to_irq(button_gpios[0].gpio);
	button1_irq = gpio_to_irq(button_gpios[1].gpio);
	free_irq(button0_irq, button0_dev_id);
	free_irq(button1_irq, button1_dev_id);

	/* Free GPIO arrays */
	gpio_free_array(led_gpios, num_led_gpios);
	gpio_free_array(button_gpios, num_button_gpios);

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
           pInfo += sprintf(pInfo, "Current operational mode: normal\n");
           break;
       case FLASHING_RED:
           pInfo += sprintf(pInfo, "Current operational mode: flashing-red\n");
           break;
       case FLASHING_YELLOW:
           pInfo += sprintf(pInfo, "Current operational mode: flashing-yellow\n");
           break;
   }


   // next print the current cycle rate (e.g. "1 Hz")
   pInfo += sprintf(pInfo, "Current cycle rate: %u\n", cycle_rate);


   // next print current status of each light
   // ("red off", "yellow off", "green on")
   pInfo += sprintf(pInfo, "Current status of each light:\n");
   if(is_red_on){
       pInfo += sprintf(pInfo, "red on\n");
   }else{
       pInfo += sprintf(pInfo, "red off\n");
   }


   if(is_yellow_on){
       pInfo += sprintf(pInfo, "yellow on\n");
   }else{
       pInfo += sprintf(pInfo, "yellow off\n");
   }


   if(is_green_on){
       pInfo += sprintf(pInfo, "green on\n");
   }else{
       pInfo += sprintf(pInfo, "green off\n");
   }
   /* Terminate info buffer (not strictly needed for snprintf/sprintf but good for strlen) */
   *pInfo = '\0';


   /* check user buffer size */
   info_len = strlen(info);
   if(*offset > info_len){
       kfree(info);
       return 0;
   }


   ret_len = info_len - *offset;
   if(len < ret_len){
       ret_len = len;
   }


   if(copy_to_user(buf, info + *offset, ret_len)){
       kfree(info);
       return -EFAULT;
   }


   *offset += ret_len;
   kfree(info);
   return ret_len;
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
		mod_timer(&mytraffic_timer->timer, jiffies + msecs_to_jiffies(1000 / cycle_rate));
	}

	*f_pos += len;
	kfree(kernel_buf);
	return len;
}

/* Timer callback function */
static void timer_handler(struct timer_list *timer_ptr) {
	/* Get parent structure from the timer_list pointer */
	struct timer_entry *entry = container_of(timer_ptr, struct timer_entry, timer);

	if(bulb_check){
		is_green_on = 1;
		is_yellow_on = 1;
		is_red_on = 1;

		gpio_set_value(led_gpios[0].gpio, is_green_on);
		gpio_set_value(led_gpios[1].gpio, is_yellow_on);
		gpio_set_value(led_gpios[2].gpio, is_red_on);

		mod_timer(&entry->timer, jiffies + msecs_to_jiffies(1000 / cycle_rate));
		return;
	}
	
	ycle_index++;

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
				/* Handle predestrian present if necessary */
				if (is_ped_present) {
					is_green_on = 0;
					is_yellow_on = 1;
					is_red_on = 1;
					cycle_index = 4;
					cycle_index_ped++;
					if (cycle_index_ped == 5) {
						is_ped_present = 0;
						cycle_index = 5;
					}
				} else {
					is_green_on = 0;
					is_yellow_on = 0;
					is_red_on = 1;
				}
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

	/* Set LEDs */
	gpio_set_value(led_gpios[0].gpio, is_green_on);
	gpio_set_value(led_gpios[1].gpio, is_yellow_on);
	gpio_set_value(led_gpios[2].gpio, is_red_on);

	/* Schedule next timer */
	mod_timer(&entry->timer, jiffies + msecs_to_jiffies(1000 / cycle_rate));
	return;
}

static irqreturn_t interrupt_handler(int irq, void *dev_id) {
	mod_timer(&debounce_timer, jiffies + msecs_to_jiffies(DEBOUNCE_TIME));
	if (dev_id == button0_dev_id) {
		/* Handle button 0 IRQ */
		if (is_button0_pressed) {
			/* Handling IRQ falling edge */
			is_button0_pressed = 0;
		} else {
			/* Handling IRQ rising edge */
			is_button0_pressed = 1;
			is_ped_present = 0;
			cycle_index_ped = 0;

			/* Change operational mode */
			switch(current_mode) {
				case NORMAL:
					current_mode = FLASHING_YELLOW;
					break;
				case FLASHING_YELLOW:
					current_mode = FLASHING_RED;
					break;
				case FLASHING_RED:
					current_mode = NORMAL;
					break;
			}
		}
	} else if (dev_id == button1_dev_id) {
		/* Handle button 1 IRQ */
		if (is_button1_pressed) {
			/* Handling IRQ falling edge */
			is_button1_pressed = 0;
		} else {
			/* Handling IRQ rising edge */
			is_button1_pressed = 1;
			is_ped_present = 1;
		}
	} else {
		return IRQ_NONE;
	}
	return IRQ_HANDLED;
}

static void debounce_timer_handler(struct timer_list *t){
	int b0 = gpio_get_value(button_gpios[0].gpio) ? 1 : 0;
	int b1 = gpio_get_value(button_gpios[1].gpio) ? 1 : 0;

	/* both buttons pressed */
	if(b0 && b1){
		bulb_check = 1;

		is_red_on = 1;
		is_yellow_on = 1;
		is_green_on = 1;
		
		gpio_set_value(led_gpios[0].gpio, is_green_on);
		gpio_set_value(led_gpios[1].gpio, is_yellow_on);
		gpio_set_value(led_gpios[2].gpio, is_red_on);
		return;
	}

	/* both buttons were released (falling edge) */
	if(!b0 && !b1){
		/* "reset" back to initial state */
		if(bulb_check){
			bulb_check = 0;

			current_mode = NORMAL;
			cycle_rate = 1;
			is_ped_present = 0;
			cycle_index = 0;

			is_green_on = 1;
			is_yellow_on = 0;
			is_red_on = 0;

			gpio_set_value(led_gpios[0].gpio, is_green_on);
			gpio_set_value(led_gpios[1].gpio, is_yellow_on);
			gpio_set_value(led_gpios[2].gpio, is_red_on);

			if(mytraffic_timer){
				mod_timer(&mytraffic_timer->timer, jiffies + msecs_to_jiffies(1000 / cycle_rate));
			}
		}
		return;
	}

	if(bulb_check){
		bulb_check = 0;
	}
}