#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>    // uint32_t

#include <linux/timer.h>

#include <linux/memblock.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/ioremap.h>

#include <linux/proc_fs.h>

MODULE_AUTHOR("Quelqu'un");
MODULE_DESCRIPTION("Exemple de module");
MODULE_LICENSE("GPL");

long int TIMEOUT = 10;

static volatile uint32_t * portb;

static struct timer_list timer;

void led_on(void);
void led_off(void);
void led_toggle(void);

void led_on(void)
{
    *portb = (1<<24);
}

void led_off(void)
{
    *portb &= ~(1<<24);
}

void led_toggle(void)
{
    if (*portb & (1<<24)) {
        led_off();
    }
    else {
        led_on();
    }
}

static void timer_handler(struct timer_list *t) {

    /* Il faut réarmer le timer si l'on veut un appel périodique */
    mod_timer(&timer,jiffies + TIMEOUT);

    led_toggle();
}

#define BUFFER_SIZE 10
ssize_t fops_write (struct file * file_unused, const char __user * buffer, size_t count, loff_t * ppos_unused)
{
    char recv_buffer[BUFFER_SIZE];

    if (count > BUFFER_SIZE) count = BUFFER_SIZE;

    if (copy_from_user(recv_buffer, buffer, count)) {
        return -EFAULT;
    }

    kstrtol(recv_buffer, 10, &TIMEOUT);

    printk(KERN_INFO "new TIMEOUT is %d\n", TIMEOUT);

    return count;
}

ssize_t fops_read(struct file *file_unused, char __user * buffer, size_t count, loff_t * ppos) {
    int errno=0;
    int copy;

    char tmp[BUFFER_SIZE];
    int len;

    len = scnprintf(tmp, BUFFER_SIZE, "%ld\n", TIMEOUT);

    return simple_read_from_buffer(buffer, count, ppos, tmp, len);
}

static struct proc_ops proc_fops = 
{
    .proc_write = fops_write,
    .proc_read = fops_read
};

static int __init le_module_init(void) {
    printk(KERN_INFO "Hello world!\n");
    
    // Remap PORTB for LED
    portb = ioremap(0xff709000, 4);

    struct proc_dir_entry * proc_period = proc_create (
        "period", 0666,
        NULL,
        &proc_fops);

    printk(KERN_INFO "proc_period = %x\n", proc_period);
    
    led_on();
    timer_setup(&timer, timer_handler, 0);
    mod_timer(&timer, jiffies + TIMEOUT);

    return 0;
}

static void __exit le_module_exit(void) {
    printk(KERN_ALERT "Bye bye...\n");

    timer_delete(&timer);

    remove_proc_entry("period", NULL);

    led_off();
}

module_init(le_module_init);
module_exit(le_module_exit);