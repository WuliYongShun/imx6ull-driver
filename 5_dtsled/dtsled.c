#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#include <linux/of.h>
#include <linux/of_address.h>

#define DTSLED_CNT       1           /* 设备号个数 */
#define DTSLED_NAME      "dtsled" /* 名字 */

#define LEDON               1           /* 开灯 */
#define LEDOFF              0           /* 关灯 */

/* 映射后的寄存器虚拟地址指针 */
static void __iomem *IMX6U_CCM_CCGR1;
static void __iomem *SW_MUX_GPIO1_IO04;
static void __iomem *SW_PAD_GPIO1_IO04;
static void __iomem *GPIO1_DR;
static void __iomem *GPIO1_GDIR;

/* dtsled 设备结构体 */
struct dtsled_dev
{
    dev_t devid;            /* 设备号 */
    struct cdev cdev;       /* cdev */
    struct class *class;    /* 类 */
    struct device *device;  /* 设备 */
    int major;              /* 主设备号 */
    int minor;              /* 次设备号 */
	struct device_node *nd; /* 设备节点 */
};

struct dtsled_dev dtsled; /* led设备 */

void led_switch(u8 sta)
{
    u32 val = 0;
    if (LEDON == sta)
    {
        val = readl(GPIO1_DR);
        val &= ~(1 << 4);
        writel(val, GPIO1_DR);
    }
    else if (LEDOFF == sta)
    {
        val = readl(GPIO1_DR);
        val |= (1 << 4);
        writel(val, GPIO1_DR);
    }
}


static int led_open(struct inode *inode, struct file *filp)
{
    filp->private_data = &dtsled;    /* 设置私有数据 */
    return 0;
}


static ssize_t led_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
    return 0;
}


static ssize_t led_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
    int retvalue;
    unsigned char databuf[1];
    unsigned char ledstat;

    retvalue = copy_from_user(databuf, buf, cnt);
    if (retvalue < 0)
    {
        /* code */
        printk("kernel write failed!\r\n");
        return -EFAULT;
    }

    ledstat = databuf[0];

    if (ledstat == LEDON)
    {
        led_switch(LEDON);
    }
    else if (ledstat == LEDOFF)
    {
        led_switch(LEDOFF);
    }
    return 0;
}


static int led_release(struct inode *inode, struct file *filp)
{
    return 0;
}

/* 设备操作函数 */
static struct file_operations dtsled_fops = {
    .owner = THIS_MODULE,
    .open = led_open,
    .read = led_read,
    .write = led_write,
    .release = led_release,    
};

static int __init led_init(void)
{
    /* 入口函数具体内容 */
	u32 val = 0;
	int ret;
	u32 regdata[14];
	const char *str;
	struct property *proper;

	/* 获取设备树中的属性数据 */
	/* 1、获取设备节点： shunled */
	dtsled.nd = of_find_node_by_path("/shunled");
	
	if(dtsled.nd == NULL) {
		printk("shunled node can not found!\r\n");
		return -EINVAL;
	} else {
		printk("shunled node has been found!\r\n");
	}
	
	/* 2、获取 compatible 属性内容 */
	proper = of_find_property(dtsled.nd, "compatible", NULL);
	if(proper == NULL) {
		printk("compatible property find failed\r\n");
	} else {
		printk("compatible = %s\r\n", (char*)proper->value);
	}
	
	/* 3、获取 status 属性内容 */
	ret = of_property_read_string(dtsled.nd, "status", &str);
	if(ret < 0){
		printk("status read failed!\r\n");
	} else {
		printk("status = %s\r\n",str);
	}
	
	/* 4、获取 reg 属性内容 */
	ret = of_property_read_u32_array(dtsled.nd, "reg", regdata, 10);
	if(ret < 0) {
		printk("reg property read failed!\r\n");
	} else {
		u8 i = 0;
		printk("reg data:\r\n");
		for(i = 0; i < 10; i++)
			printk("%#X ", regdata[i]);
		printk("\r\n");
	}
	
	/* 初始化 LED */
#if 0
	/* 1、寄存器地址映射 */
	IMX6U_CCM_CCGR1 = ioremap(regdata[0], regdata[1]);
	SW_MUX_GPIO1_IO04 = ioremap(regdata[2], regdata[3]);
	SW_PAD_GPIO1_IO04 = ioremap(regdata[4], regdata[5]);
	GPIO1_DR = ioremap(regdata[6], regdata[7]);
	GPIO1_GDIR = ioremap(regdata[8], regdata[9]);
#else
	IMX6U_CCM_CCGR1 = of_iomap(dtsled.nd, 0);
	SW_MUX_GPIO1_IO04 = of_iomap(dtsled.nd, 1);
	SW_PAD_GPIO1_IO04 = of_iomap(dtsled.nd, 2);
	GPIO1_DR = of_iomap(dtsled.nd, 3);
	GPIO1_GDIR = of_iomap(dtsled.nd, 4);
#endif

	/* 2、使能 GPIO1 时钟 */
	val = readl(IMX6U_CCM_CCGR1);
	val &= ~(3 << 26); /* 清除以前的设置 */
	val |= (3 << 26); /* 设置新值 */
	writel(val, IMX6U_CCM_CCGR1);

	/* 3、设置 GPIO1_IO04 的复用功能，将其复用为
	* GPIO1_IO04，最后设置 IO 属性。
	*/
	writel(5, SW_MUX_GPIO1_IO04);

	/* 寄存器 SW_PAD_GPIO1_IO03 设置 IO 属性 */
	writel(0x10B0, SW_PAD_GPIO1_IO04);

	/* 4、设置 GPIO1_IO03 为输出功能 */
	val = readl(GPIO1_GDIR);
	val &= ~(1 << 4); /* 清除以前的设置 */
	val |= (1 << 4); /* 设置为输出 */
	writel(val, GPIO1_GDIR);


	/* 5、默认关闭 LED */
	val = readl(GPIO1_DR);
	val |= (1 << 4);
	writel(val, GPIO1_DR);


    /* 注册字符设备驱动 */
    /* 1、创建设备号 */
    if (dtsled.major) /* 定义设备号执行 */
    {
        dtsled.devid = MKDEV(dtsled.major, 0);
        register_chrdev_region(dtsled.devid, DTSLED_CNT, DTSLED_NAME);
    }
    else /* 没有定义设备号执行 */
    {
        /*申请设备号*/
        alloc_chrdev_region(&dtsled.devid, 0, DTSLED_CNT, DTSLED_NAME);

        /* 获取主设备号 */
        dtsled.major = MAJOR(dtsled.devid);

        /* 获取次设备号 */
        dtsled.minor = MINOR(dtsled.devid);
    }
    printk("dtsled major=%d,minor=%d\r\n",dtsled.major, dtsled.minor);

    /* 2.初始化cdev */
    dtsled.cdev.owner = THIS_MODULE;
    cdev_init(&dtsled.cdev, &dtsled_fops);

    /* 3.添加一个cdev */
    cdev_add(&dtsled.cdev, dtsled.devid, DTSLED_CNT);

    /* 4.创建类 */    
    dtsled.class = class_create(THIS_MODULE, DTSLED_NAME);
    if (IS_ERR(dtsled.class))
    {
        return PTR_ERR(dtsled.class);
    }
    
    /* 5、创建设备 */
    dtsled.device = device_create(dtsled.class, NULL, dtsled.devid, NULL, DTSLED_NAME);
    if (IS_ERR(dtsled.device))
    {
        return PTR_ERR(dtsled.device);
    }
    
    return 0;
}


/* 驱动出口函数 */
static void __exit led_exit(void)
{
	/* 取消映射 */
	iounmap(IMX6U_CCM_CCGR1);
	iounmap(SW_MUX_GPIO1_IO04);
	iounmap(SW_PAD_GPIO1_IO04);
	iounmap(GPIO1_DR);
	iounmap(GPIO1_GDIR);


	/* 注销字符设备 */
    cdev_del(&dtsled.cdev);/* 删除 cdev */
    unregister_chrdev_region(dtsled.devid, DTSLED_CNT);

    device_destroy(dtsled.class, dtsled.devid);
    class_destroy(dtsled.class);

}


/* 将上边两个函数指定为驱动入口和出口函数  */
module_init(led_init);
module_exit(led_exit);


/*
 * LISENSE 信息
 * */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yshun");







