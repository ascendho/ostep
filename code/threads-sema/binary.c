#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "common.h"
#include "common_threads.h"

// 跨平台信号量头文件（Linux用semaphore.h，Apple用zemaphore.h）
#ifdef linux
#include <semaphore.h>
#elif __APPLE__
#include "zemaphore.h"
#endif

sem_t mutex;              // 二进制信号量（互斥锁）
volatile int counter = 0; // 共享计数器（需保护）

// 子线程函数：对计数器累加1000万次（需互斥）
void *child(void *arg)
{
    int i;
    for (i = 0; i < 10000000; i++)
    {
        Sem_wait(&mutex); // 加锁（P操作）：确保同一时间只有一个线程修改counter
        counter++;        // 临界区操作：修改共享变量
        Sem_post(&mutex); // 解锁（V操作）
    }
    return NULL;
}

// 主线程：创建两个子线程，等待完成后输出结果
int main(int argc, char *argv[])
{
    Sem_init(&mutex, 1); // 初始化互斥锁（值为1：二进制信号量）

    pthread_t c1, c2;
    Pthread_create(&c1, NULL, child, NULL); // 创建线程1
    Pthread_create(&c2, NULL, child, NULL); // 创建线程2

    Pthread_join(c1, NULL); // 等待线程1完成
    Pthread_join(c2, NULL); // 等待线程2完成

    // 预期结果为20000000（两线程各加1000万）
    printf("result: %d (should be 20000000)\n", counter);
    return 0;
}