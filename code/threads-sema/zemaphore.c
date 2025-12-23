#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "common.h"
#include "common_threads.h"
#include "zemaphore.h"

Zem_t s; // 定义全局信号量

// 子线程函数：休眠4秒后释放信号量
void *child(void *arg)
{
    sleep(4);          // 模拟耗时操作
    printf("child\n"); // 子线程完成
    Zem_post(&s);      // 释放信号量（V操作）
    return NULL;
}

// 主线程：初始化信号量，创建子线程，等待子线程完成
int main(int argc, char *argv[])
{
    Zem_init(&s, 0); // 初始化信号量为0（初始无资源）
    printf("parent: begin\n");

    pthread_t c;
    Pthread_create(&c, NULL, child, NULL); // 创建子线程

    Zem_wait(&s);            // 等待信号量（P操作）：子线程未完成则阻塞
    printf("parent: end\n"); // 子线程完成后执行
    return 0;
}