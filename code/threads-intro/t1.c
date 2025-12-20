#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "common.h"
#include "common_threads.h"

// 全局变量：控制循环次数（所有线程共享）
int max;
// 全局共享计数器：volatile关键字确保变量每次从内存读取（避免编译器优化导致的可见性问题）
volatile int counter = 0;

// 线程执行函数：循环递增共享计数器counter
// 参数arg：线程标识字符串（如"A"或"B"）
void *mythread(void *arg)
{
    char *letter = arg;                                // 转换参数为字符串
    int i;                                             // 线程私有变量（每个线程有独立的栈空间，i互不干扰）
    printf("%s: begin [addr of i: %p]\n", letter, &i); // 打印线程开始信息及i的地址（验证私有性）

    // 循环max次，每次递增共享计数器
    for (i = 0; i < max; i++)
    {
        counter = counter + 1; // 共享变量操作（存在竞态条件：多线程同时读写可能导致结果错误）
    }
    printf("%s: done\n", letter);
    return NULL;
}

int main(int argc, char *argv[])
{
    // 检查命令行参数：需传入一个整数（循环次数）
    if (argc != 2)
    {
        fprintf(stderr, "usage: main-first <loopcount>\n");
        exit(1);
    }
    max = atoi(argv[1]); // 将参数转换为整数，作为循环次数

    pthread_t p1, p2; // 线程标识符
    // 打印主线程开始信息：初始counter值和counter的地址（验证共享性）
    printf("main: begin [counter = %d] [%x]\n", counter,
           (unsigned int)&counter);

    // 创建线程A：执行mythread函数
    Pthread_create(&p1, NULL, mythread, "A");
    // 创建线程B：执行mythread函数
    Pthread_create(&p2, NULL, mythread, "B");

    // 等待线程A和B执行完毕
    Pthread_join(p1, NULL);
    Pthread_join(p2, NULL);

    // 打印最终结果：实际counter值和预期值（max*2）
    // 由于counter递增操作非原子性，实际结果可能小于预期值（竞态条件导致）
    printf("main: done\n [counter: %d]\n [should: %d]\n",
           counter, max * 2);
    return 0;
}