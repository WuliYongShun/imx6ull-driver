#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
#include "linux/ioctl.h"

/* 定义按键值 */
#define KEY0VALUE   0XF0
#define INVAKEY     0X00

/*
 * @description : main 主程序
 */
int main(int argc, char *argv[])
{
    int fd, ret;
    char *filename;
    unsigned char data;

    if(argc != 2){
        printf("Error Usage!\r\n");
        return -1;
    }


    filename = argv[1];

    /* 打开 led 驱动 */
    fd = open(filename, O_RDWR);
    if(fd < 0){
        printf("file %s open failed!\r\n", argv[1]);
        return -1;
    }

    /* 循环读取按键值数据 */
    while (1)
    {
        ret = read(fd, &data, sizeof(data));

        if (ret < 0)
        {
            /* code */
        }

        /* 数据读取正常 */
        else
        {
            if (data)
            {
                /* 打印读取到数据 */
                printf("key value = %#X\r\n", data);
            }
            
        }
    }


    ret = close(fd);

    if (ret < 0)
    {
        printf("file %s close failed!\r\n", argv[1]);
        return -1;
    }
    
    return 0;
}