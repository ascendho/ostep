#include <assert.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "common.h"
#include "common_threads.h"

// 共享缓冲区配置
int max;     // 缓冲区大小
int loops;   // 生产者生产的元素数量
int *buffer; // 缓冲区数组

// 缓冲区指针和状态
int use_ptr = 0;  // 消费者取数据的位置（循环队列）
int fill_ptr = 0; // 生产者放数据的位置（循环队列）
int num_full = 0; // 缓冲区中已填充的元素数量

// 双条件变量：分别处理"缓冲区空"和"缓冲区满"的情况
pthread_cond_t empty = PTHREAD_COND_INITIALIZER; // 缓冲区空时等待
pthread_cond_t fill = PTHREAD_COND_INITIALIZER;  // 缓冲区满时等待
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;   // 保护共享变量

int consumers = 1; // 消费者数量
int verbose = 1;   // 冗余输出开关（未使用）

// 生产者填充数据到缓冲区
void do_fill(int value)
{
    buffer[fill_ptr] = value;        // 放置数据
    fill_ptr = (fill_ptr + 1) % max; // 循环移动指针
    num_full++;                      // 已填充数量+1
}

// 消费者从缓冲区取数据
int do_get()
{
    int tmp = buffer[use_ptr];     // 取出数据
    use_ptr = (use_ptr + 1) % max; // 循环移动指针
    num_full--;                    // 已填充数量-1
    return tmp;
}

// 生产者线程：生成数据并放入缓冲区
void *producer(void *arg)
{
    int i;
    // 生产loops个数据
    for (i = 0; i < loops; i++)
    {
        Mutex_lock(&m); // 加锁
        // 若缓冲区满，则等待"缓冲区空"信号
        while (num_full == max)
            Cond_wait(&empty, &m);
        do_fill(i);         // 填充数据
        Cond_signal(&fill); // 发送"缓冲区有数据"信号
        Mutex_unlock(&m);   // 解锁
    }

    // 生产结束：为每个消费者发送一个结束标记(-1)
    for (i = 0; i < consumers; i++)
    {
        Mutex_lock(&m);
        while (num_full == max)
            Cond_wait(&empty, &m);
        do_fill(-1); // 放置结束标记
        Cond_signal(&fill);
        Mutex_unlock(&m);
    }

    return NULL;
}

// 消费者线程：从缓冲区取数据并处理
void *consumer(void *arg)
{
    int tmp = 0;
    // 持续取数据，直到收到结束标记(-1)
    while (tmp != -1)
    {
        Mutex_lock(&m); // 加锁
        // 若缓冲区空，则等待"缓冲区有数据"信号
        while (num_full == 0)
            Cond_wait(&fill, &m);
        tmp = do_get();      // 取出数据
        Cond_signal(&empty); // 发送"缓冲区有空位"信号
        Mutex_unlock(&m);    // 解锁
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    // 解析命令行参数：缓冲区大小、生产数量、消费者数量
    if (argc != 4)
    {
        fprintf(stderr, "usage: %s <buffersize> <loops> <consumers>\n", argv[0]);
        exit(1);
    }
    max = atoi(argv[1]);
    loops = atoi(argv[2]);
    consumers = atoi(argv[3]);

    // 初始化缓冲区
    buffer = (int *)malloc(max * sizeof(int));
    assert(buffer != NULL);
    for (int i = 0; i < max; i++)
    {
        buffer[i] = 0;
    }

    // 创建生产者和消费者线程
    pthread_t pid, cid[consumers];
    Pthread_create(&pid, NULL, producer, NULL);
    for (int i = 0; i < consumers; i++)
    {
        Pthread_create(&cid[i], NULL, consumer, (void *)(long long int)i);
    }

    // 等待所有线程结束
    Pthread_join(pid, NULL);
    for (int i = 0; i < consumers; i++)
    {
        Pthread_join(cid[i], NULL);
    }

    return 0;
}