#include <stdio.h> // 提供输入输出函数（如printf、sprintf）
#include "udp.h"   // 自定义UDP通信库，提供UDP套接字操作函数（创建、读写等）

#define BUFFER_SIZE (1000) // 定义消息缓冲区大小，用于存储接收和发送的消息

// 服务器主函数：监听UDP端口，接收客户端消息并回复
int main(int argc, char *argv[])
{
    // 创建UDP套接字并绑定到本地10000端口，用于接收客户端消息
    // UDP_Open函数在udp.c中实现，返回套接字描述符（失败时返回-1）
    int sd = UDP_Open(10000);
    assert(sd > -1); // 断言套接字创建成功（失败则程序终止）

    // 无限循环：持续监听客户端消息（服务器通常需要长期运行）
    while (1)
    {
        struct sockaddr_in addr;   // 用于存储发送消息的客户端地址信息
        char message[BUFFER_SIZE]; // 接收缓冲区，用于存放客户端发送的消息

        printf("server:: waiting...\n"); // 提示服务器正在等待消息

        // 从UDP套接字读取客户端发送的消息，结果存入message缓冲区
        // UDP_Read函数在udp.c中实现，返回接收的字节数（失败时可能为-1）
        int rc = UDP_Read(sd, &addr, message, BUFFER_SIZE);
        // 打印接收的消息信息（大小和内容）
        printf("server:: read message [size:%d contents:(%s)]\n", rc, message);

        // 如果成功接收到消息（字节数>0），则向客户端发送回复
        if (rc > 0)
        {
            char reply[BUFFER_SIZE];         // 回复消息缓冲区
            sprintf(reply, "goodbye world"); // 构造回复消息内容

            // 通过UDP套接字向客户端（addr指向的地址）发送回复消息
            // UDP_Write函数在udp.c中实现，返回发送的字节数
            rc = UDP_Write(sd, &addr, reply, BUFFER_SIZE);
            printf("server:: reply\n"); // 提示已发送回复
        }
    }

    return 0; // 理论上不会执行到这里（因无限循环）
}