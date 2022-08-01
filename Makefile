#
# Makefile for Polarfire PCI-Express Endpoint
#

CONFIG_MODULE_SIG=n
WARN    := -W -Wall -Wstrict-prototypes -Wmissing-prototypes
obj-m	 += mpci.o

mpci-objs := mpcie.o mdma.o misrdpc.o

KDIR= /lib/modules/$(shell uname -r)/build
all:
	$(MAKE) $(WARN) -C  $(KDIR) SUBDIRS=$(PWD) modules
clean:
	rm -rf $(wildcard *.o *.ko mo* Mo* *.mod.c )
