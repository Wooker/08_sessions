obj-m += solution.o

.PHONY: all app

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

load:
	sudo insmod solution.ko
	sudo dmesg -c

unload:
	sudo rmmod solution.ko
	sudo dmesg -c

node:
	sudo mknod /dev/solution_node c 212 0
	sudo chmod a+rw /dev/solution_node

app:
	gcc -o app app.c
