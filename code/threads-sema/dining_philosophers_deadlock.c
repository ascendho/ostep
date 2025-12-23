#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "common.h"
#include "common_threads.h"

// 跨平台信号量头文件
#ifdef linux
#include <semaphore.h>
#elif __APPLE__
#include "zemaphore.h"
#endif

// 线程参数结构体：循环次数和线程ID
typedef struct
{
    int num_loops; // 每个哲学家思考-就餐的循环次数
    int thread_id; // 哲学家ID（0-4）
} arg_t;

sem_t forks[5]; // 5个叉子，每个用信号量表示（值为1：互斥使用）

// 获取左叉子ID（与哲学家ID相同）
int left(int p)
{
    return p;
}

// 获取右叉子ID（循环取余，最后一个哲学家右叉子为0）
int right(int p)
{
    return (p + 1) % 5;
}

// 获取叉子（死锁风险：所有哲学家先拿左再拿右）
void get_forks(int p)
{
    Sem_wait(&forks[left(p)]);  // 拿左叉子
    Sem_wait(&forks[right(p)]); // 拿右叉子（可能死锁）
}

// 释放叉子
void put_forks(int p)
{
    Sem_post(&forks[left(p)]);  // 放左叉子
    Sem_post(&forks[right(p)]); // 放右叉子
}

void think() { return; } // 模拟思考（空实现）
void eat() { return; }   // 模拟就餐（空实现）

// 哲学家线程函数：循环思考->拿叉子->就餐->放叉子
void *philosopher(void *arg)
{
    arg_t *args = (arg_t *)arg;
    int p = args->thread_id;

    int i;
    for (i = 0; i < args->num_loops; i++)
    {
        think();
        get_forks(p); // 可能死锁：所有哲学家同时拿到左叉子，等待右叉子
        eat();
        put_forks(p);
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "usage: dining_philosophers_deadlock <num_loops>\n");
        exit(1);
    }
    printf("dining: started\n");

    // 初始化叉子信号量（每个值为1，互斥使用）
    int i;
    for (i = 0; i < 5; i++)
        Sem_init(&forks[i], 1);

    pthread_t p[5]; // 5个哲学家线程
    arg_t a[5];     // 线程参数
    for (i = 0; i < 5; i++)
    {
        a[i].num_loops = atoi(argv[1]);
        a[i].thread_id = i;
        Pthread_create(&p[i], NULL, philosopher, &a[i]);
    }

    // 等待所有哲学家完成
    for (i = 0; i < 5; i++)
        Pthread_join(p[i], NULL);

    printf("dining: finished\n");
    return 0;
}