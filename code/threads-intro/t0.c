#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
// 引入通用工具函数和线程封装函数的头文件
#include "common.h"
#include "common_threads.h"

// 线程执行函数：接收字符串参数并打印
// 参数arg：传递给线程的字符串（需强制类型转换）
void *mythread(void *arg)
{
    // 将void*类型的参数转换为char*，并打印字符串
    printf("%s\n", (char *)arg);
    return NULL; // 线程执行完毕，返回空
}

int main(int argc, char *argv[])
{
    // 检查命令行参数：本程序不需要参数，若传入参数则报错
    if (argc != 1)
    {
        fprintf(stderr, "usage: main\n");
        exit(1);
    }

    // 定义两个线程标识符（pthread_t类型用于标识线程）
    pthread_t p1, p2;
    printf("main: begin\n");

    // 创建第一个线程：执行mythread函数，传递参数"A"
    // Pthread_create是封装的宏，内部调用pthread_create并检查返回值（确保创建成功）
    Pthread_create(&p1, NULL, mythread, "A");
    // 创建第二个线程：执行mythread函数，传递参数"B"
    Pthread_create(&p2, NULL, mythread, "B");

    // 等待线程p1执行完毕（阻塞主线程，直到p1结束）
    Pthread_join(p1, NULL);
    // 等待线程p2执行完毕
    Pthread_join(p2, NULL);

    printf("main: end\n");
    return 0;
}