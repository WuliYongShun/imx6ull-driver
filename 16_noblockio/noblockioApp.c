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

    struct pollfd fds;
    fd_set readfds;
    struct timeval timeout;

    if(argc != 2){
        printf("Error Usage!\r\n");
        return -1;
    }


    filename = argv[1];

    /* 打开 led 驱动 */
    fd = open(filename, O_RDWR | O_NONBLOCK);
    if(fd < 0){
        printf("file %s open failed!\r\n", filename);
        return -1;
    }

#if 0

    /* 构造结构体 */
    fds.fd = fd;
    fds.events = POLLIN;

    while(1)
    {
        ret = poll(&fds, 1, 500);
        if (ret)
        {
            ret = read(fd, &data, sizeof(data));
            if(ret < 0)
            {
                /* 读取错误 */
            }
            else
            {
                if(data)
                    printf("key value = %d \r\n", data);
            }
        }
        /* 超时 */
        else if(ret == 0)
        {
            /* 超时处理函数 */
        }
        /* 错误处理 */
        else if(ret < 0)
        {
            /* 用户自定义错误处理 */
        }
    }

#endif

    /* 循环读取按键值数据 */
    while (1)
    {
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);

        /* 构造超时时间 */
        timeout.tv_sec = 0;
        timeout.tv_usec = 500000; /* 500ms */

        ret = select(fd + 1, &readfds, NULL, NULL, &timeout);

        switch (ret)
        {
        /* 超时 */
        case 0:
            /* 用户自定义超时处理 */
            break;

        /* 错误 */
        case -1:
            /* 用户自定义超时处理 */
            break;
        
        /* 可以读取数据 */
        default:
            if(FD_ISSET(fd, &readfds))
            {
                ret = read(fd, &data, sizeof(data));
                if (ret < 0)
                {
                    /* 读取错误 */
                }
                else
                {
                    if (data)
                    {
                        printf("key value=%d\r\n", data);
                    } 
                }
            }

            break;
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