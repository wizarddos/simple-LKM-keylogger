# List of modules to build
obj-m += rootkit.o
rootkit-objs := keylogger.o init-exit.o hide-seek.o

# Kernel build directory
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

# Targets
all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
