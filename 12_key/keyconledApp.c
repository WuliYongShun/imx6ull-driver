#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"

/* 定义按键值 */
#define KEY0VALUE   0XF0
#define INVAKEY     0X00

/*
 * @description : main 主程序
 */
int main(int argc, char *argv[])
{
    int fd, ledfd, ret;
    char *filename;
    char *ledname;
    unsigned char keyvalue, ledstate = 0;

    if(argc != 2){
        printf("Error Usage!\r\n");
        return -1;
    }


    filename = argv[1];

    ledname = "/dev/gpioled";

    /* 打开led驱动 */
    ledfd = open(ledname, O_RDWR);
    if(fd < 0){
        printf("led file %s open failed!\r\n", argv[1]);
        return -1;
    }

    /* 打开 key 驱动 */
    fd = open(filename, O_RDWR);
    if(fd < 0){
        printf("key file %s open failed!\r\n", argv[1]);
        return -1;
    }

    /* 循环读取按键值数据 */
    while (1)
    {
        read(fd, &keyvalue, sizeof(keyvalue));

        if (keyvalue == KEY0VALUE)
        {
            printf("KEY0 Press, value = %#X\r\n", keyvalue);/* 按下 */
            ledstate = !ledstate;
            
        }
        ret = write(ledfd, &ledstate, sizeof(ledstate));
        if (ret < 0)
        {
            printf("LED Control Failed!\r\n");
            close(fd);
            return -1;
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