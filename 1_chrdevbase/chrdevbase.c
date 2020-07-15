#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>

/* 住设备号 */
#define CHRDEVBASE_MAJOR 200

/* 设备名字 */
#define CHRDEVBASE_NAME "chrdevbase.c"

/* 读缓冲区 */
static char readbuf[100];

/* 写缓冲区 */
static char writebuf[100];

/* 测试数据 */
static char kerneldata[] = {"kernel data!"};


/*
 * @name: 打开设备
 * @param:inode 传递给驱动的node
 * @param:filp  设备文件， file 结构体有个叫做 private_data 的成员变量一般在 open 的时候将 private_data 指向设备结构体。
 * @return: 0 成功	其他 失败
 * */
static int chrdevbase_open(struct inode *inode, struct file *filp)
{
	printk("chrdevbase is open\r\n");
	return 0;
}


/*
 * 从设备读取数据
 * */
static ssize_t chrdevbase_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
	int retvalue = 0;

	/* 向用户空间发送数据 */
	memcpy(readbuf, kerneldata, sizeof(kerneldata));

	retvalue = copy_to_user(buf, readbuf, cnt);
	if(retvalue == 0)
	{
		printk("kernel senddata is ok\r\n");
	}
	else
	{
		printk("kernel senddata failed\r\n");
	}

	return 0;
}


/*
 * 写数据向设备
 * */
static ssize_t chrdevbase_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
	int retvalue = 0;
	
	/* 接收用户空间传递给内核的数据  */
	retvalue = copy_from_user(writebuf, buf, cnt);

	if(retvalue == 0)
	{
		printk("write data success");
	}
	else{
		printk("writer data failue");
	}

	return 0;
}

/*
 * 关闭释放设备
 * */
static int chrdevbase_release(struct inode *node, struct file *filp)
{
	printk("chrdevbae release\r\n");

	return 0;
}

/*
 * 设备操作函数结构体
 * */
static struct file_operations chrdevbase_fops = {
	.owner = THIS_MODULE,
	.open = chrdevbase_open,
	.read = chrdevbase_read,
	.write = chrdevbase_write,
	.release = chrdevbase_release,
};



/* 驱动入口函数 */
static int __init chrdevbase_init(void)
{
	/* 入口函数具体内容 */
	int retvalue = 0;

	/* 注册字符串设备驱动 */
	retvalue = register_chrdev(CHRDEVBASE_MAJOR, CHRDEVBASE_NAME, &chrdevbase_fops);

	if(retvalue < 0)
	{
		printk("chrdevbase driver register failed\r\n");
	}

	printk("chrdevbase_init()\r\n");


	return 0;
}

/* 驱动出口函数 */
static void __exit chrdevbase_exit(void)
{
	/* 出口函数具体内容 */
	unregister_chrdev(CHRDEVBASE_MAJOR, CHRDEVBASE_NAME);
	printk("chrdevbase_exit()\r\n");

}

/* 将上边两个函数指定为驱动入口和出口函数  */
module_init(chrdevbase_init);
module_exit(chrdevbase_exit);


/*
 * LISENSE 信息
 * */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yshun");
