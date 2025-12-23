#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "common.h"
#include "common_threads.h"

#ifdef linux
#include <semaphore.h>
#elif __APPLE__
#include "zemaphore.h"
#endif

typedef struct
{
    int num_loops;
    int thread_id;
} arg_t;

sem_t forks[5];

int left(int p)
{
    return p;
}

int right(int p)
{
    return (p + 1) % 5;
}

// 大部分代码与死锁版本相同，仅修改get_forks函数避免死锁
// 获取叉子（无死锁：最后一个哲学家先拿右叉子）
void get_forks(int p)
{
    if (p == 4)
    {                               // 第5个哲学家（ID=4）特殊处理
        Sem_wait(&forks[right(p)]); // 先拿右叉子
        Sem_wait(&forks[left(p)]);  // 再拿左叉子
    }
    else
    {
        Sem_wait(&forks[left(p)]);  // 其他哲学家先拿左叉子
        Sem_wait(&forks[right(p)]); // 再拿右叉子
    }
}

void put_forks(int p)
{
    Sem_post(&forks[left(p)]);
    Sem_post(&forks[right(p)]);
}

void think()
{
    return;
}

void eat()
{
    return;
}

void *philosopher(void *arg)
{
    arg_t *args = (arg_t *)arg;
    int p = args->thread_id;

    int i;
    for (i = 0; i < args->num_loops; i++)
    {
        think();
        get_forks(p);
        eat();
        put_forks(p);
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "usage: dining_philosophers <num_loops>\n");
        exit(1);
    }
    printf("dining: started\n");

    int i;
    for (i = 0; i < 5; i++)
        Sem_init(&forks[i], 1);

    pthread_t p[5];
    arg_t a[5];
    for (i = 0; i < 5; i++)
    {
        a[i].num_loops = atoi(argv[1]);
        a[i].thread_id = i;
        Pthread_create(&p[i], NULL, philosopher, &a[i]);
    }

    for (i = 0; i < 5; i++)
        Pthread_join(p[i], NULL);

    printf("dining: finished\n");
    return 0;
}