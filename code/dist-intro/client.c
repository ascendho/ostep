#include <stdio.h> // 提供输入输出函数（如printf、sprintf）
#include "udp.h"   // 自定义UDP通信库，提供UDP相关操作函数（如创建套接字、发送/接收数据等）

#define BUFFER_SIZE (1000) // 定义缓冲区大小，用于存储发送和接收的消息

// 客户端主函数：通过UDP向服务器发送消息并接收回复
int main(int argc, char *argv[])
{
    struct sockaddr_in addrSnd; // 用于存储发送目标（服务器）的地址信息
    struct sockaddr_in addrRcv; // 用于存储接收来源（服务器）的地址信息

    // 创建UDP套接字并绑定到本地20000端口
    // UDP_Open函数在udp.c中实现，返回套接字描述符（类似文件描述符）
    int sd = UDP_Open(20000);

    // 填充服务器的地址信息：目标主机为localhost（本地主机），端口为10000
    // UDP_FillSockAddr函数在udp.c中实现，用于初始化sockaddr_in结构体
    int rc = UDP_FillSockAddr(&addrSnd, "localhost", 10000);

    char message[BUFFER_SIZE];       // 消息缓冲区，用于存放要发送的内容
    sprintf(message, "hello world"); // 将要发送的消息写入缓冲区

    // 打印发送的消息内容
    printf("client:: send message [%s]\n", message);
    // 通过UDP套接字向服务器发送消息
    // UDP_Write函数在udp.c中实现，返回发送的字节数，失败时返回-1
    rc = UDP_Write(sd, &addrSnd, message, BUFFER_SIZE);
    if (rc < 0)
    { // 检查发送是否失败
        printf("client:: failed to send\n");
        exit(1); // 发送失败则退出程序
    }

    // 等待接收服务器的回复
    printf("client:: wait for reply...\n");
    // 通过UDP套接字接收服务器的回复，结果存入message缓冲区
    // UDP_Read函数在udp.c中实现，返回接收的字节数
    rc = UDP_Read(sd, &addrRcv, message, BUFFER_SIZE);
    // 打印接收的回复信息（大小和内容）
    printf("client:: got reply [size:%d contents:(%s)\n", rc, message);
    return 0;
}