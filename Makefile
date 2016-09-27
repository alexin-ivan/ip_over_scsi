MOD=ip_over_scsi
obj-m := $(MOD).o
KVERSION ?= $(shell uname -r)

all:
	$(MAKE) -C /lib/modules/$(KVERSION)/build M=$(PWD) modules

clean:
	$(MAKE) -C /lib/modules/$(KVERSION)/build M=$(PWD) clean

help:
	$(MAKE) -C /lib/modules/$(KVERSION)/build M=$(PWD) help

install:
	insmod $(MOD).ko

uninstall:
	rmmod $(MOD)
