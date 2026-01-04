#ifndef __UDP_h__
#define __UDP_h__
// 头文件保护宏：防止该头文件被重复包含，避免编译错误
// 当第一次包含时，__UDP_h__未定义，执行宏内代码；再次包含时则跳过

//
// 包含必要的系统头文件
//
#include <stdio.h>  // 提供输入输出函数（如printf、fprintf）
#include <stdlib.h> // 提供内存分配、进程退出等函数（如malloc、exit）
#include <unistd.h> // 提供POSIX操作系统API（如close、sleep）
#include <errno.h>  // 提供错误码定义（如errno变量）
#include <string.h> // 提供字符串操作函数（如memcpy、strlen）
#include <netdb.h>  // 提供网络主机信息查询函数（如gethostbyname）
#include <fcntl.h>  // 提供文件控制操作（如文件描述符相关）
#include <assert.h> // 提供断言宏（用于调试时检查条件是否成立）

#include <sys/socket.h> // 提供套接字操作核心函数（如socket、bind、sendto、recvfrom）
#include <sys/wait.h>   // 提供进程等待函数（如wait）
#include <sys/time.h>   // 提供时间相关结构和函数（如gettimeofday）
#include <sys/types.h>  // 提供基本数据类型定义（如pid_t、size_t）

#include <netinet/tcp.h> // 提供TCP相关定义（此处可能用于通用网络常量，虽为UDP但可能复用）
#include <netinet/in.h>  // 提供IPv4地址结构（如sockaddr_in）和协议定义

//
// 函数声明：UDP通信相关接口
//

// 创建UDP套接字并绑定到指定端口
// 参数：port - 要绑定的本地端口号
// 返回值：成功返回套接字描述符（非负整数），失败返回-1
int UDP_Open(int port);

// 关闭UDP套接字
// 参数：fd - 要关闭的套接字描述符
// 返回值：成功返回0，失败返回-1（具体由close系统调用决定）
int UDP_Close(int fd);

// 从UDP套接字读取数据，并获取发送方地址
// 参数：
//   fd - 已打开的UDP套接字描述符
//   addr - 用于存储发送方地址信息的sockaddr_in结构体指针
//   buffer - 接收数据的缓冲区
//   n - 缓冲区的最大容量（最多接收的字节数）
// 返回值：成功返回接收的字节数，失败返回-1
int UDP_Read(int fd, struct sockaddr_in *addr, char *buffer, int n);

// 通过UDP套接字向指定地址发送数据
// 参数：
//   fd - 已打开的UDP套接字描述符
//   addr - 指向目标地址信息（sockaddr_in）的指针
//   buffer - 存储要发送数据的缓冲区
//   n - 要发送的数据字节数
// 返回值：成功返回发送的字节数，失败返回-1
int UDP_Write(int fd, struct sockaddr_in *addr, char *buffer, int n);

// 填充sockaddr_in结构体，设置目标主机地址和端口
// 参数：
//   addr - 指向要填充的sockaddr_in结构体的指针
//   hostName - 目标主机名或IP地址字符串（如"localhost"、"192.168.1.1"）
//   port - 目标端口号
// 返回值：成功返回0，失败返回-1
int UDP_FillSockAddr(struct sockaddr_in *addr, char *hostName, int port);

#endif // __UDP_h__
       // 头文件保护宏结束