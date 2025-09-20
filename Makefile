ifeq ($(IN_KERNEL), y)

obj-$(CONFIG_MISC_EXAMPLE) += example.o
obj-$(CONFIG_MISC_EXAMPLE_TEST) += example_test.o

else

NAME := lsmevf
obj-m := $(NAME).o
SRC := mod.c \
       hook.c \
       dev.c \
       buf.c \
       event.c 
lsmevf-y := $(SRC:.c=.o)

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)


all:
	$(MAKE) M=$(PWD) -C $(KDIR) modules
clean:
	$(MAKE) M=$(PWD) -C $(KDIR) clean
load:
	insmod ./$(NAME).ko
	@echo 'module $(NAME) +p' > /sys/kernel/debug/dynamic_debug/control
unload:
	rmmod ./$(NAME).ko
endif
