// To compile: gcc -o pstack pstack.c
// To run:     ./pstack <操作1> <操作2> ...

// 持久化栈示例：展示如何使用mmap()系统调用实现持久化内存的软件抽象

// 使用说明：
// 1. 需先用'truncate'命令创建初始（空）的备份文件，文件大小必须是系统页大小的倍数
//    示例：
//    prompt> getconf PAGESIZE  # 获取系统页大小（如4096）
//    4096
//    prompt> truncate -s 4096 ps.img  # 创建4096字节的空文件ps.img
//
// 2. 文件结构说明：
//    - 前sizeof(size_t)字节：存储栈中元素的数量（即栈顶指针位置）
//    - 剩余部分：存储栈中的整数元素
//
// 3. 运行示例：
//    prompt> ./pstack 7 13 47 pop  # 压入7、13、47，再弹出47（输出47）
//    prompt> ./pstack pop pop 99   # 弹出13（输出13）、弹出7（输出7），再压入99
//    prompt> ./pstack pop          # 弹出99（输出99）
//
// 4. 特性：一次程序调用中压入的元素会保留到下一次调用，实现栈的持久化
// 5. 可通过hexdump命令查看备份文件内容：hexdump ps.img

#include <assert.h>    // 提供断言宏assert，用于调试时检查条件是否成立
#include <fcntl.h>     // 提供文件操作相关函数（如open）及标志（如O_RDWR）
#include <stdio.h>     // 提供输入输出函数（如printf、fprintf）
#include <stdlib.h>    // 提供标准库函数（如atoi、exit）
#include <string.h>    // 提供字符串操作函数（如strcmp）
#include <unistd.h>    // 提供POSIX系统调用（如close）
#include <sys/mman.h>  // 提供内存映射相关函数（如mmap）及标志（如PROT_READ）
#include <sys/stat.h>  // 提供获取文件状态的函数（如fstat）
#include <sys/types.h> // 提供基本数据类型定义（如size_t）

// 定义持久化栈的结构体
typedef struct
{
    size_t n;    // 栈中当前元素的数量（栈顶索引+1）
    int stack[]; // 柔性数组（C99特性），用于存储栈元素（大小动态取决于文件剩余空间）
} pstack_t;

int main(int argc, char *argv[])
{
    int fd;           // 文件描述符：用于标识打开的备份文件ps.img
    int rc;           // 函数返回值：用于接收系统调用的结果，检查操作是否成功
    struct stat s;    // 文件状态结构体：用于存储ps.img的元信息（如大小）
    size_t file_size; // 文件大小：存储ps.img的字节数，用于内存映射
    pstack_t *p;      // 指向内存映射后的栈结构体：通过该指针操作持久化栈

    // 打开备份文件ps.img，模式为读写（O_RDWR）
    // 若文件不存在则打开失败（需提前用truncate创建）
    fd = open("ps.img", O_RDWR);
    assert(fd > -1); // 断言文件打开成功，失败则程序终止（调试用）

    // 获取文件状态（如大小、权限等），存储到结构体s中
    rc = fstat(fd, &s);
    assert(rc == 0); // 断言获取文件状态成功

    // 将文件大小转换为size_t类型（无符号整数，适合表示大小）
    file_size = (size_t)s.st_size;
    // 断言文件大小至少能容纳栈结构体头部（n的大小），且是int的倍数（保证栈元素内存对齐）
    assert(file_size >= sizeof(pstack_t) && file_size % sizeof(int) == 0);

    // 将文件映射到进程地址空间，实现"内存操作即文件操作"
    // 参数说明：
    // - NULL：让系统自动选择映射的内存地址
    // - file_size：映射的字节数（整个文件）
    // - PROT_READ|PROT_WRITE：映射区域可读写
    // - MAP_SHARED：映射区域的修改会同步到文件（实现持久化的关键）
    // - fd：要映射的文件描述符
    // - 0：映射的文件偏移量（从文件开头开始）
    p = (pstack_t *)mmap(NULL, file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    assert(p != MAP_FAILED); // 断言内存映射成功

    // 处理命令行参数（argv[0]是程序名，从argv[1]开始处理用户输入）
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "pop") == 0)
        { // 若参数是"pop"，执行弹出操作
            if (p->n > 0)
            { // 栈非空时才能弹出
                // 先将元素计数n减1（指向栈顶元素），再打印栈顶元素
                printf("%d\n", p->stack[--p->n]);
            }
        }
        else
        { // 否则视为整数，执行压入操作
            // 检查栈是否有空间：当前结构体大小 + 新增1个元素的大小 <= 文件总大小
            if (sizeof(pstack_t) + (1 + p->n) * sizeof(int) <= file_size)
            {
                // 将字符串参数转换为整数，存入栈顶（当前n的位置），再将n加1
                p->stack[p->n++] = atoi(argv[i]);
            }
        }
    }

    (void)close(fd); // 关闭文件描述符（映射仍有效，修改已通过MAP_SHARED同步到文件）
    return 0;
}