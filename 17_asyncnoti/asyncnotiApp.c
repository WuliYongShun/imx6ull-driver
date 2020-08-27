#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
#include "linux/ioctl.h"

#include "poll.h"
#include "sys/select.h"
#include "sys/time.h"

#include "signal.h"

/* 定义按键值 */
#define KEY0VALUE   0XF0
#define INVAKEY     0X00


static int fd = 0;  /* 文件描述符 */

/*
 * @description : main 主程序
 */
static void sigio_signal_func(int signum)
{
   int err = 0;
   unsigned int keyvalue = 0;

   err = read(fd, &keyvalue, sizeof(keyvalue));

   if (err < 0)
   {
       /* 读取错误 */
   }
   else
   {
       printf("sigio signal! key value=%d\r\n", keyvalue);
   }  
}

/*
 * @description : main 主程序
 */
int main(int argc, char *argv[])
{
    int flags;
    char *filename;
 
    if(argc != 2){
        printf("Error Usage!\r\n");
        return -1;
    }


    filename = argv[1];

    /* 打开 key 驱动 */
    fd = open(filename, O_RDWR);
    if(fd < 0){
        printf("file %s open failed!\r\n", filename);
        return -1;
    }

    /* 设置信号SIGIO的处理函数 */
    signal(SIGIO, sigio_signal_func);

    /* 将当前进程的进程号告诉给内核 */
    fcntl(fd, F_SETOWN, getpid());

    /* 获取当前进程的状态 */
    flags = fcntl(fd, F_GETFD);

    /* 设置进程启用异步通知功能 */
    fcntl(fd, F_SETFL, flags | FASYNC);

    /* 循环读取按键值数据 */
    while (1)
    {
        sleep(2);
    }


    flags = close(fd);

    if (flags < 0)
    {
        printf("file %s close failed!\r\n", argv[1]);
        return -1;
    }
    
    return 0;
}