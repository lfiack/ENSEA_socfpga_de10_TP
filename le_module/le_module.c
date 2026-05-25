#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>    // uint32_t

#include <linux/memblock.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/ioremap.h>

MODULE_AUTHOR("Quelqu'un");
MODULE_DESCRIPTION("Exemple de module");
MODULE_LICENSE("GPL");

static volatile uint32_t * p;

static int __init le_module_init(void) {
    printk(KERN_INFO "Hello world!\n");
    
    p = ioremap(0xff709000, 4);
    *p = (1<<24);

    return 0;
}

static void __exit le_module_exit(void) {
    printk(KERN_ALERT "Bye bye...\n");

    *p = 0;
}

module_init(le_module_init);
module_exit(le_module_exit);