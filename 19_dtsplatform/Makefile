KERNELDIR := /mnt/hgfs/EmbeddedDevelopment/linux/driver/linux-imx-rel_imx_4.1.15_2.1.0_ga_shun
CURRENT_PATH := $(shell pwd)

obj-m := leddriver.o

build: kernel_modules

kernel_modules:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean