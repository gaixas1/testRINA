#
# Written by Sergio Leon <slgaixas@upc.edu>
#

KDIR=../../linux

ifneq ($(KERNELRELEASE),)

ccflags-y = -Wtype-limits -Inet/rina

obj-m := rmt-deltaq.o

rmt-deltaq-y := ps.o

else

all:
	$(MAKE) -C $(KDIR) M=$$PWD

clean:
	rm -r -f *.o *.ko *.mod.c *.mod.o Module.symvers .*.cmd .tmp_versions modules.order

install:
	$(MAKE) -C $(KDIR) M=$$PWD modules_install

uninstall:
	@echo "This target has not been implemented yet"
	@exit 1

endif
