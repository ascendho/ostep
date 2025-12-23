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

sem_t s; // 用于限流的信号量

// 子线程函数：受信号量限制的临界区操作
void *child(void *arg)
{
    Sem_wait(&s);                               // 获取信号量（若超过限制则阻塞）
    printf("child %lld\n", (long long int)arg); // 打印线程ID
    sleep(1);                                   // 模拟临界区操作
    Sem_post(&s);                               // 释放信号量（允许其他线程进入）
    return NULL;
}

// 主线程：创建多个线程，通过信号量限制并发数
int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "usage: throttle <num_threads> <sem_value>\n");
        exit(1);
    }
    int num_threads = atoi(argv[1]); // 总线程数
    int sem_value = atoi(argv[2]);   // 信号量值（最大并发数）

    Sem_init(&s, sem_value); // 初始化信号量（限制并发数）

    printf("parent: begin\n");
    pthread_t c[num_threads]; // 线程数组

    // 创建所有子线程
    int i;
    for (i = 0; i < num_threads; i++)
        Pthread_create(&c[i], NULL, child, (void *)(long long int)i);

    // 等待所有线程完成
    for (i = 0; i < num_threads; i++)
        Pthread_join(c[i], NULL);

    printf("parent: end\n");
    return 0;
}