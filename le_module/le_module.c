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

MODULE_AUTHOR("Quelqu'un");
MODULE_DESCRIPTION("Exemple de module");
MODULE_LICENSE("GPL");

#define TIMEOUT 10

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

static int __init le_module_init(void) {
    printk(KERN_INFO "Hello world!\n");
    
    portb = ioremap(0xff709000, 4);
    
    led_on();
    timer_setup(&timer, timer_handler, 0);
    mod_timer(&timer, jiffies + TIMEOUT);

    return 0;
}

static void __exit le_module_exit(void) {
    printk(KERN_ALERT "Bye bye...\n");

    timer_delete(&timer);
    led_off();
}

module_init(le_module_init);
module_exit(le_module_exit);