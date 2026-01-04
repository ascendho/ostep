#include "udp.h" // 包含UDP通信相关的函数声明和必要的头文件

// 创建UDP套接字并绑定到本地指定端口，用于监听 incoming 数据包
// 参数：port - 要绑定的本地端口号
// 返回值：成功返回套接字描述符（非负整数），失败返回-1
int UDP_Open(int port)
{
    int fd; // 套接字描述符，用于标识创建的UDP套接字
    // 创建UDP套接字：AF_INET表示IPv4协议族，SOCK_DGRAM表示数据报套接字（UDP）
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        perror("socket"); // 打印创建套接字失败的错误信息
        return 0;         // 返回0表示失败（实际应返回-1，此处可能为代码实现细节）
    }

    // 初始化绑定地址结构
    struct sockaddr_in my_addr;
    bzero(&my_addr, sizeof(my_addr)); // 清零地址结构，避免垃圾值

    my_addr.sin_family = AF_INET;         // 使用IPv4地址族
    my_addr.sin_port = htons(port);       // 将端口号从主机字节序转换为网络字节序（大端）
    my_addr.sin_addr.s_addr = INADDR_ANY; // 绑定到本地所有可用网络接口（0.0.0.0）

    // 将套接字绑定到指定地址和端口
    if (bind(fd, (struct sockaddr *)&my_addr, sizeof(my_addr)) == -1)
    {
        perror("bind"); // 打印绑定失败的错误信息
        close(fd);      // 绑定失败时关闭已创建的套接字
        return -1;      // 返回-1表示绑定失败
    }

    return fd; // 返回成功创建并绑定的套接字描述符
}

// 填充sockaddr_in结构体，用于设置目标主机地址和端口
// 参数：
//   addr - 指向要填充的sockaddr_in结构体的指针
//   hostname - 目标主机名或IP地址字符串（如"localhost"、"192.168.1.1"）
//   port - 目标端口号
// 返回值：成功返回0，失败返回-1
int UDP_FillSockAddr(struct sockaddr_in *addr, char *hostname, int port)
{
    bzero(addr, sizeof(struct sockaddr_in)); // 清零地址结构
    if (hostname == NULL)
    {
        return 0; // 若主机名为NULL，仅清空结构后返回（用于接收时获取发送方地址）
    }

    addr->sin_family = AF_INET;   // 使用IPv4地址族
    addr->sin_port = htons(port); // 端口号转换为网络字节序

    struct in_addr *in_addr;    // 用于存储解析后的IP地址
    struct hostent *host_entry; // 用于存储主机信息（通过主机名解析）
    // 将主机名解析为IP地址信息
    if ((host_entry = gethostbyname(hostname)) == NULL)
    {
        perror("gethostbyname"); // 打印解析失败的错误信息
        return -1;               // 返回-1表示解析失败
    }
    in_addr = (struct in_addr *)host_entry->h_addr; // 获取第一个IP地址
    addr->sin_addr = *in_addr;                      // 设置目标IP地址

    return 0; // 成功填充地址结构
}

// 通过UDP套接字发送数据到指定地址
// 参数：
//   fd - 已创建的UDP套接字描述符
//   addr - 指向目标地址结构（sockaddr_in）的指针
//   buffer - 存储要发送数据的缓冲区
//   n - 要发送的数据字节数
// 返回值：成功返回发送的字节数，失败返回-1（由sendto决定）
int UDP_Write(int fd, struct sockaddr_in *addr, char *buffer, int n)
{
    int addr_len = sizeof(struct sockaddr_in); // 地址结构的长度
    // 发送数据：sendto用于UDP发送，指定目标地址和地址长度
    int rc = sendto(fd, buffer, n, 0, (struct sockaddr *)addr, addr_len);
    return rc; // 返回发送的字节数（sendto的返回值）
}

// 通过UDP套接字接收数据，并获取发送方地址
// 参数：
//   fd - 已创建的UDP套接字描述符
//   addr - 指向用于存储发送方地址结构（sockaddr_in）的指针
//   buffer - 用于存储接收数据的缓冲区
//   n - 缓冲区的最大容量（最多接收的字节数）
// 返回值：成功返回接收的字节数，失败返回-1（由recvfrom决定）
int UDP_Read(int fd, struct sockaddr_in *addr, char *buffer, int n)
{
    int len = sizeof(struct sockaddr_in); // 地址结构长度的初始值
    // 接收数据：recvfrom用于UDP接收，同时获取发送方地址
    int rc = recvfrom(fd, buffer, n, 0, (struct sockaddr *)addr, (socklen_t *)&len);
    // assert(len == sizeof(struct sockaddr_in));  // 验证地址长度（注释掉以避免调试中断）
    return rc; // 返回接收的字节数（recvfrom的返回值）
}

// 关闭UDP套接字
// 参数：fd - 要关闭的套接字描述符
// 返回值：成功返回0，失败返回-1（由close系统调用决定）
int UDP_Close(int fd)
{
    return close(fd); // 调用系统调用关闭套接字
}