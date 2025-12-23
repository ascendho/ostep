#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "common.h"
#include "common_threads.h"

// 跨平台信号量头文件
#ifdef linux
#include <semaphore.h>
#elif __APPLE__
#include "zemaphore.h"
#endif

sem_t s; // 用于主线程等待子线程的信号量

// 子线程函数：休眠2秒后释放信号量
void *child(void *arg)
{
    sleep(2);          // 模拟任务执行
    printf("child\n"); // 子线程完成
    Sem_post(&s);      // 释放信号量（通知主线程）
    return NULL;
}

// 主线程：创建子线程并等待其完成
int main(int argc, char *argv[])
{
    Sem_init(&s, 0); // 初始化信号量为0（初始阻塞）
    printf("parent: begin\n");

    pthread_t c;
    Pthread_create(&c, NULL, child, NULL); // 创建子线程

    Sem_wait(&s);            // 等待子线程信号（阻塞直到子线程post）
    printf("parent: end\n"); // 子线程完成后执行
    return 0;
}