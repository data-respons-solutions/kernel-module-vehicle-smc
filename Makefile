smc-y := smc_proto.o smc_main.o smc_transaction.o
obj-m := smc.o

SRC := $(shell pwd)

all:
	$(MAKE) -C $(KERNEL_SRC) $(EXTRA) M=$(SRC)

modules_install:
	$(MAKE) -C $(KERNEL_SRC) $(EXTRA) M=$(SRC) modules_install

clean:
	rm -f *.o *~ core .depend .*.cmd *.ko *.mod.c
	rm -f Module.markers Module.symvers modules.order
	rm -rf .tmp_versions Modules.symvers
	