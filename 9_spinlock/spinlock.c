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
#include <linux/of_gpio.h>

#define GPIOLED_CNT       1           /* 设备号个数 */
#define GPIOLED_NAME      "gpioled"   /* 名字 */

#define LEDON               1           /* 开灯 */
#define LEDOFF              0           /* 关灯 */

/* gpioled 设备结构体 */
struct gpioled_dev
{
    dev_t devid;            /* 设备号 */
    struct cdev cdev;       /* cdev */
    struct class *class;    /* 类 */
    struct device *device;  /* 设备 */
    int major;              /* 主设备号 */
    int minor;              /* 次设备号 */
	struct device_node *nd; /* 设备节点 */
    int led_gpio;           /* led所使用额GPIO编号 */
    int dev_stats;          /* 设备状态， 0，设备未使用;>0,设备已经被使用 */
    spinlock_t lock;        /* 自旋锁 */
};

struct gpioled_dev gpioled; /* led设备 */

/*
 *  @description : 打开设备
 */
static int led_open(struct inode *inode, struct file *filp)
{
    unsigned long flags;
    filp->private_data = &gpioled;  /* 设置私有数据 */

    /* 上锁 */
    spin_lock_irqsave(&gpioled.lock, flags);

    /* 如果设备被使用了 */
    if (gpioled.dev_stats)
    {
        spin_unlock_irqrestore(&gpioled.lock, flags); /* 解锁 */
        return -EBUSY;
    }
    /* 如果设备没有打开，那么就标记已经打开了 */
    gpioled.dev_stats++;
    
    /* 解锁 */
    spin_unlock_irqrestore(&gpioled.lock, flags);

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
    struct gpioled_dev *dev = filp->private_data;

    retvalue = copy_from_user(databuf, buf, cnt);
    if (retvalue < 0)
    {
        printk("kernel write failed!\r\n");
        return -EFAULT;
    }

    ledstat = databuf[0];

    if (ledstat == LEDON)
    {
        gpio_set_value(dev->led_gpio, 0);
        // led_switch(LEDON);
    }
    else if (ledstat == LEDOFF)
    {
        gpio_set_value(dev->led_gpio, 1);
        // led_switch(LEDOFF);
    }
    return 0;
}


/*
 * @description : 关闭/释放设备
 */
static int led_release(struct inode *inode, struct file *filp)
{
    unsigned long flags;
    struct gpioled_dev *dev = filp->private_data;

    /* 关闭驱动文件的时候将 dev_stats 减 1 */
    spin_lock_irqsave(&dev->lock, flags); /* 上锁 */
    if (dev->dev_stats)
    {
        dev->dev_stats--;
    }
    spin_unlock_irqrestore(&dev->lock, flags);/* 解锁 */

    return 0;
}

/* 设备操作函数 */
static struct file_operations gpioled_fops = {
    .owner = THIS_MODULE,
    .open = led_open,
    .read = led_read,
    .write = led_write,
    .release = led_release,    
};

/*
 * @description : 驱动入口函数
 */
static int __init led_init(void)
{
    /* 入口函数具体内容 */
	int ret;

    /* 初始化自旋锁 */
    spin_lock_init(&gpioled.lock);


	/* 获取设备树中的属性数据 */
	/* 1、获取设备节点： shunled */
	gpioled.nd = of_find_node_by_path("/shunled");
	
	if(gpioled.nd == NULL) {
		printk("shunled node can not found!\r\n");
		return -EINVAL;
	} else {
		printk("shunled node has been found!\r\n");
	}

    /* 2、 获取设备树中的 gpio 属性，得到 LED 所使用的 LED 编号 */
    gpioled.led_gpio = of_get_named_gpio(gpioled.nd, "led-gpio", 0);
    if(gpioled.led_gpio < 0)
    {
        printk("can't get led-gpio");
        return -EINVAL;
    }

    printk("led-gpio num = %d\r\n", gpioled.led_gpio);

    /* 3、设置 GPIO1_IO04 为输出，并且输出高电平，默认关闭 LED 灯 */
    ret = gpio_direction_output(gpioled.led_gpio, 1);
    if (ret < 0)
    {
        printk("can't set gpio!\r\n");
    }


    /* 注册字符设备驱动 */
    /* 1、创建设备号 */
    if (gpioled.major) /* 定义设备号执行 */
    {
        gpioled.devid = MKDEV(gpioled.major, 0);
        register_chrdev_region(gpioled.devid, GPIOLED_CNT, GPIOLED_NAME);
    }
    else /* 没有定义设备号执行 */
    {
        /*申请设备号*/
        alloc_chrdev_region(&gpioled.devid, 0, GPIOLED_CNT, GPIOLED_NAME);

        /* 获取主设备号 */
        gpioled.major = MAJOR(gpioled.devid);

        /* 获取次设备号 */
        gpioled.minor = MINOR(gpioled.devid);
    }
    printk("gpioled major=%d,minor=%d\r\n",gpioled.major, gpioled.minor);

    /* 2.初始化cdev */
    gpioled.cdev.owner = THIS_MODULE;
    cdev_init(&gpioled.cdev, &gpioled_fops);

    /* 3.添加一个cdev */
    cdev_add(&gpioled.cdev, gpioled.devid, GPIOLED_CNT);

    /* 4.创建类 */    
    gpioled.class = class_create(THIS_MODULE, GPIOLED_NAME);
    if (IS_ERR(gpioled.class))
    {
        return PTR_ERR(gpioled.class);
    }
    
    /* 5、创建设备 */
    gpioled.device = device_create(gpioled.class, NULL, gpioled.devid, NULL, GPIOLED_NAME);
    if (IS_ERR(gpioled.device))
    {
        return PTR_ERR(gpioled.device);
    }
    
    return 0;
}


/* 驱动出口函数 */
static void __exit led_exit(void)
{
	/* 注销字符设备 */
    cdev_del(&gpioled.cdev);/* 删除 cdev */
    unregister_chrdev_region(gpioled.devid, GPIOLED_CNT);

    device_destroy(gpioled.class, gpioled.devid);
    class_destroy(gpioled.class);

}


/* 将上边两个函数指定为驱动入口和出口函数  */
module_init(led_init);
module_exit(led_exit);


/* LISENSE 信息 */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yshun");







