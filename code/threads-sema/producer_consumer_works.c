#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <pthread.h>
#include <stdlib.h>

#include "common.h"
#include "common_threads.h"

// 跨平台信号量头文件
#ifdef linux
#include <semaphore.h>
#elif __APPLE__
#include "zemaphore.h"
#endif

int max;     // 缓冲区大小
int loops;   // 生产者生产次数
int *buffer; // 共享缓冲区

int use = 0;  // 消费者读取位置（索引）
int fill = 0; // 生产者写入位置（索引）

sem_t empty; // 空缓冲区数量（初始为max）
sem_t full;  // 满缓冲区数量（初始为0）
sem_t mutex; // 缓冲区互斥锁

#define CMAX (10)  // 最大消费者数量
int consumers = 1; // 实际消费者数量

// 生产者填充缓冲区
void do_fill(int value)
{
    buffer[fill] = value;
    fill++;
    if (fill == max)
        fill = 0; // 循环缓冲区
}

// 消费者获取缓冲区数据
int do_get()
{
    int tmp = buffer[use];
    use++;
    if (use == max)
        use = 0; // 循环缓冲区
    return tmp;
}

// 生产者线程：生产数据并放入缓冲区，最后发送结束信号
void *producer(void *arg)
{
    int i;
    // 生产loops个数据
    for (i = 0; i < loops; i++)
    {
        Sem_wait(&empty); // 等待空缓冲区（P(empty)）
        Sem_wait(&mutex); // 加锁保护缓冲区
        do_fill(i);       // 写入数据
        Sem_post(&mutex); // 解锁
        Sem_post(&full);  // 增加满缓冲区（V(full)）
    }

    // 发送结束信号（每个消费者一个-1）
    for (i = 0; i < consumers; i++)
    {
        Sem_wait(&empty);
        Sem_wait(&mutex);
        do_fill(-1); // -1表示消费结束
        Sem_post(&mutex);
        Sem_post(&full);
    }

    return NULL;
}

// 消费者线程：从缓冲区取数据，直到收到-1
void *consumer(void *arg)
{
    int tmp = 0;
    while (tmp != -1)
    {                                                 // 循环消费直到结束信号
        Sem_wait(&full);                              // 等待满缓冲区（P(full)）
        Sem_wait(&mutex);                             // 加锁保护缓冲区
        tmp = do_get();                               // 读取数据
        Sem_post(&mutex);                             // 解锁
        Sem_post(&empty);                             // 增加空缓冲区（V(empty)）
        printf("%lld %d\n", (long long int)arg, tmp); // 打印消费数据
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        fprintf(stderr, "usage: %s <buffersize> <loops> <consumers>\n", argv[0]);
        exit(1);
    }
    max = atoi(argv[1]);       // 缓冲区大小
    loops = atoi(argv[2]);     // 生产次数
    consumers = atoi(argv[3]); // 消费者数量
    assert(consumers <= CMAX);

    buffer = (int *)malloc(max * sizeof(int)); // 初始化缓冲区
    assert(buffer != NULL);
    int i;
    for (i = 0; i < max; i++)
        buffer[i] = 0;

    // 初始化信号量
    Sem_init(&empty, max); // 空缓冲区数=max
    Sem_init(&full, 0);    // 满缓冲区数=0
    Sem_init(&mutex, 1);   // 互斥锁

    pthread_t pid, cid[CMAX];
    Pthread_create(&pid, NULL, producer, NULL); // 创建生产者
    // 创建消费者
    for (i = 0; i < consumers; i++)
    {
        Pthread_create(&cid[i], NULL, consumer, (void *)(long long int)i);
    }

    // 等待所有线程完成
    Pthread_join(pid, NULL);
    for (i = 0; i < consumers; i++)
    {
        Pthread_join(cid[i], NULL);
    }
    return 0;
}