#定义变量

KERNELDIR := /home/shun/linux/linux/linux-imx-rel_imx_4.1.15_2.1.0_ga_shun
CURRENT_PATH := $(shell pwd)
obj-m := chrdevbase.o
build: kernel_modules

#目标文件

kernel_modules:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules


#伪目标
.PHONY: clean

clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean

