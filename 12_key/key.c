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
#include <linux/semaphore.h>

#define KEY_CNT       1           /* 设备号个数 */
#define KEY_NAME      "key"       /* 名字 */

/* 定义按键值 */
#define KEY0VALUE     0XF0        /* 按键值 */
#define INVAKEY       0X00        /* 无效的按键值 */


/* key设备结构体 */
struct key_dev
{
    dev_t devid;            /* 设备号 */
    struct cdev cdev;       /* cdev */
    struct class *class;    /* 类 */
    struct device *device;  /* 设备 */
    int major;              /* 主设备号 */
    int minor;              /* 次设备号 */
	struct device_node *nd; /* 设备节点 */
    int key_gpio;           /* key 所使用的 GPIO 编号 */
    atomic_t keyvalue;      /* 按键值 */
};

struct key_dev keydev; /* key设备 */


/*
 * @description : 初始化按键 IO， open 函数打开驱动的时候
 */
static int keyio_init(void)
{
    /* 1、获取设备节点： key */
	keydev.nd = of_find_node_by_path("/key");
	
	if(keydev.nd == NULL) {
		printk("shunled node can not found!\r\n");
		return -EINVAL;
	} else {
		printk("shunled node has been found!\r\n");
	}

    /* 2、 获取设备树中的 gpio 属性，得到 KEY 所使用的 KEY 编号 */
    keydev.key_gpio = of_get_named_gpio(keydev.nd, "key-gpio", 0);
    if(keydev.key_gpio < 0)
    {
        printk("can't get led-gpio");
        return -EINVAL;
    }
    printk("key-gpio num = %d\r\n", keydev.key_gpio);

    /* 初始化 key 所使用的 IO */
    gpio_request(keydev.key_gpio, "key0"); /* 请求 IO */
    gpio_direction_input(keydev.key_gpio); /* 设置为输入 */

    return 0;
}

/*
 * @description : 打开设备
 */
static int key_open(struct inode *inode, struct file *filp)
{
    int ret = 0;

    filp->private_data = &keydev;    /* 设置私有数据 */

    ret = keyio_init(); /* 初始化按键 IO */

    if (ret < 0)
    {
        return ret;
    }
    
    return 0;
}

/*
 * @description : 从设备读取数据
 */
static ssize_t key_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
    int ret = 0;
    unsigned char value;
    struct key_dev *dev = filp->private_data;

    /* key0 按下 */
    if (gpio_get_value(dev->key_gpio) == 0)
    {
        /* 等待按键按下 */
        while(!gpio_get_value(dev->key_gpio));
        atomic_set(&dev->keyvalue, KEY0VALUE);
    }
    else
    {
        atomic_set(&dev->keyvalue, INVAKEY);
    }
    
    value = atomic_read(&dev->keyvalue); /* 保存按键值 */

    ret = copy_to_user(buf, &value, sizeof(value));

    return ret;
}

static int key_release(struct inode *inode, struct file *filp)
{
    return 0;
}

/* 设备操作函数 */
static struct file_operations key_fops = {
    .owner = THIS_MODULE,
    .open = key_open,
    .read = key_read,
    .release = key_release,    
};

/*
 * @description : 驱动入口函数
 */
static int __init mykey_init(void)
{
    /* 初始化原子变量 */
    atomic_set(&keydev.keyvalue, INVAKEY);


    /* 注册字符设备驱动 */
    /* 1、创建设备号 */
    if (keydev.major) /* 定义设备号执行 */
    {
        keydev.devid = MKDEV(keydev.major, 0);
        register_chrdev_region(keydev.devid, KEY_CNT, KEY_NAME);
    }
    else /* 没有定义设备号执行 */
    {
        /*申请设备号*/
        alloc_chrdev_region(&keydev.devid, 0, KEY_CNT, KEY_NAME);

        /* 获取主设备号 */
        keydev.major = MAJOR(keydev.devid);

        /* 获取次设备号 */
        keydev.minor = MINOR(keydev.devid);
    }
    printk("keydev major=%d,minor=%d\r\n",keydev.major, keydev.minor);

    /* 2.初始化cdev */
    keydev.cdev.owner = THIS_MODULE;
    cdev_init(&keydev.cdev, &key_fops);

    /* 3.添加一个cdev */
    cdev_add(&keydev.cdev, keydev.devid, KEY_CNT);

    /* 4.创建类 */    
    keydev.class = class_create(THIS_MODULE, KEY_NAME);
    if (IS_ERR(keydev.class))
    {
        return PTR_ERR(keydev.class);
    }
    
    /* 5、创建设备 */
    keydev.device = device_create(keydev.class, NULL, keydev.devid, NULL, KEY_NAME);
    if (IS_ERR(keydev.device))
    {
        return PTR_ERR(keydev.device);
    }
    
    return 0;
}


/*
 * @description : 驱动出口函数
 */
static void __exit mykey_exit(void)
{
	/* 注销字符设备 */
    cdev_del(&keydev.cdev);/* 删除 cdev */
    unregister_chrdev_region(keydev.devid, KEY_CNT);

    device_destroy(keydev.class, keydev.devid);
    class_destroy(keydev.class);

}


/* 将上边两个函数指定为驱动入口和出口函数  */
module_init(mykey_init);
module_exit(mykey_exit);


/* LISENSE 信息 */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yshun");







