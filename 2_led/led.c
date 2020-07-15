#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>

#include <linux/errno.h>
#include <linux/gpio.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

/* 主设备号 */
#define LED_MAJOR 200

/* 设备名字 */
#define LED_NAME "led.c"

/* 关灯 */
#define LEDOFF	0

/*  开灯*/
#define LEDON	1


/* 寄存器物理地址 */
#define CCM_CCGR1_BASE			(0X020C406C)
#define SW_MUX_GPIO1_IO04_BASE	(0X020E006C)
#define SW_PAD_GPIO1_IO04_BASE	(0X020E02F8)
#define GPIO1_DR_BASE			(0X0209C000)
#define GPIO1_GDIR_BASE			(0X0209C004)


/* 映射后的寄存器虚拟地址指针 */
static void __iomem *IMX6U_CCM_CCGR1;
static void __iomem *SW_MUX_GPIO1_IO04;
static void __iomem *SW_PAD_GPIO1_IO04;
static void __iomem *GPIO1_DR;
static void __iomem *GPIO1_GDIR;




/*
 * @description: LED 打开/关闭
 * @param - sta : LEDON(0) 打开 LED， LEDOFF(1) 关闭 LED
 * @return: 无
 * */
void led_switch(u8 sta)
{
	u32 val = 0;
	if(sta == LEDON)
	{
		val = readl(GPIO1_DR);
		val &= ~(1 << 4);
		writel(val, GPIO1_DR);
	}else if (sta == LEDOFF)
	{
		val = readl(GPIO1_DR);
		val|= (1 << 4);
		writel(val, GPIO1_DR);
	}
}

/*
 * @description: 打开设备
 * @param – inode : 传递给驱动的 inode
 * @param - filp : 设备文件， file 结构体有个叫做 private_data 的成员变量
 * 					一般在 open 的时候将 private_data 指向设备结构体。
 * @return: 0 成功;其他 失败
 * */
static int led_open(struct inode *inode, struct file *filp)
{
	printk("led is open\r\n");
	return 0;
}


/*
 * 从设备读取数据
 * */
static ssize_t led_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
	return 0;
}


/*
 * 写数据向设备
 * */
static ssize_t led_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
	int retvalue = 0;
	unsigned char databuf[1];
	unsigned char ledstat;
	
	/* 接收用户空间传递给内核的数据  */
	retvalue = copy_from_user(databuf, buf, cnt);

	if(retvalue <= 0)
	{
		printk("kernel write failed!\r\n");
		return -EFAULT;
	}
	
	ledstat = databuf[0]; /* 获取状态值 */

	if(ledstat == LEDON){
		led_switch(LEDON); /* 打开 LED 灯 */
	}else if (ledstat == LEDOFF)
	{
		led_switch(LEDOFF); /* 关闭 LED 灯 */
	}

	return 0;
}

/*
 * 关闭释放设备
 * */
static int led_release(struct inode *node, struct file *filp)
{
	printk("led release\r\n");

	return 0;
}

/*
 * 设备操作函数结构体
 * */
static struct file_operations led_fops = {
	.owner = THIS_MODULE,
	.open = led_open,
	.read = led_read,
	.write = led_write,
	.release = led_release,
};



/* 驱动入口函数 */
static int __init led_init(void)
{
	/* 入口函数具体内容 */
	int retvalue = 0;
	u32 val = 0;


	/* 初始化 LED */
	/* 1、寄存器地址映射 */
	IMX6U_CCM_CCGR1 = ioremap(CCM_CCGR1_BASE, 4);
	SW_MUX_GPIO1_IO04 = ioremap(SW_MUX_GPIO1_IO04_BASE, 4);
	SW_PAD_GPIO1_IO04 = ioremap(SW_PAD_GPIO1_IO04_BASE, 4);
	GPIO1_DR = ioremap(GPIO1_DR_BASE, 4);
	GPIO1_GDIR = ioremap(GPIO1_GDIR_BASE, 4);

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


	/* 注册字符串设备驱动 */
	retvalue = register_chrdev(LED_MAJOR, LED_NAME, &led_fops);

	if(retvalue < 0)
	{
		printk("led driver register failed\r\n");
		return -EIO;
	}

	printk("led_init()\r\n");

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


	/* 出口函数具体内容 */
	unregister_chrdev(LED_MAJOR, LED_NAME);
	printk("chrdevbase_exit()\r\n");

}

/* 将上边两个函数指定为驱动入口和出口函数  */
module_init(led_init);
module_exit(led_exit);


/*
 * LISENSE 信息
 * */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yshun");
