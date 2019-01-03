KERNEL_PATH ?= /lib/modules/$(shell uname -r)/build

obj-m += ftrace_hook.o

ftrace_hook-objs := ./src/ftrace_hook.o ./src/udp.o

all:
	make -C $(KERNEL_PATH) M=$(PWD) modules

clean:
	make -C $(KERNEL_PATH) M=$(PWD) clean
