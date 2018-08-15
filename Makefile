obj-m += emulator.o

# emulator-y := pcicfg.o
# emulator-y += pcibox.o

all:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean
