#include <assert.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "common.h"
#include "common_threads.h"

// 共享缓冲区配置（同pc.c）
int max;
int loops;
int *buffer;

int use_ptr = 0;
int fill_ptr = 0;
int num_full = 0;

// 单条件变量：同时处理"空"和"满"的情况（可能低效）
pthread_cond_t cv = PTHREAD_COND_INITIALIZER;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

int consumers = 1;
int verbose = 1;

// 生产者填充数据（同pc.c）
void do_fill(int value)
{
    buffer[fill_ptr] = value;
    fill_ptr = (fill_ptr + 1) % max;
    num_full++;
}

// 消费者取数据（同pc.c）
int do_get()
{
    int tmp = buffer[use_ptr];
    use_ptr = (use_ptr + 1) % max;
    num_full--;
    return tmp;
}

// 生产者线程：使用单条件变量
void *producer(void *arg)
{
    int i;
    for (i = 0; i < loops; i++)
    {
        Mutex_lock(&m);
        // 缓冲区满时等待（单条件变量）
        while (num_full == max)
            Cond_wait(&cv, &m);
        do_fill(i);
        Cond_signal(&cv); // 唤醒等待的线程（可能是生产者或消费者）
        Mutex_unlock(&m);
    }

    // 发送结束标记（同pc.c）
    for (i = 0; i < consumers; i++)
    {
        Mutex_lock(&m);
        while (num_full == max)
            Cond_wait(&cv, &m);
        do_fill(-1);
        Cond_signal(&cv);
        Mutex_unlock(&m);
    }

    return NULL;
}

// 消费者线程：使用单条件变量
void *consumer(void *arg)
{
    int tmp = 0;
    while (tmp != -1)
    {
        Mutex_lock(&m);
        // 缓冲区空时等待（单条件变量）
        while (num_full == 0)
            Cond_wait(&cv, &m);
        tmp = do_get();
        Cond_signal(&cv); // 唤醒等待的线程（可能是生产者或消费者）
        Mutex_unlock(&m);
    }
    return NULL;
}

// main函数（同pc.c）
int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        fprintf(stderr, "usage: %s <buffersize> <loops> <consumers>\n", argv[0]);
        exit(1);
    }
    max = atoi(argv[1]);
    loops = atoi(argv[2]);
    consumers = atoi(argv[3]);

    buffer = (int *)malloc(max * sizeof(int));
    assert(buffer != NULL);
    for (int i = 0; i < max; i++)
    {
        buffer[i] = 0;
    }

    pthread_t pid, cid[consumers];
    Pthread_create(&pid, NULL, producer, NULL);
    for (int i = 0; i < consumers; i++)
    {
        Pthread_create(&cid[i], NULL, consumer, (void *)(long long int)i);
    }
    Pthread_join(pid, NULL);
    for (int i = 0; i < consumers; i++)
    {
        Pthread_join(cid[i], NULL);
    }
    return 0;
}