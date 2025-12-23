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

// 读写锁结构体
typedef struct _rwlock_t
{
    sem_t writelock; // 写锁：排他访问
    sem_t lock;      // 保护readers计数的互斥锁
    int readers;     // 当前读者数量
} rwlock_t;

// 初始化读写锁
void rwlock_init(rwlock_t *lock)
{
    lock->readers = 0;
    Sem_init(&lock->lock, 1);      // 初始化互斥锁（保护readers）
    Sem_init(&lock->writelock, 1); // 初始化写锁（排他）
}

// 获取读锁
void rwlock_acquire_readlock(rwlock_t *lock)
{
    Sem_wait(&lock->lock); // 加锁保护readers
    lock->readers++;       // 读者数量+1
    // 第一个读者获取写锁（阻止写操作）
    if (lock->readers == 1)
        Sem_wait(&lock->writelock);
    Sem_post(&lock->lock); // 解锁
}

// 释放读锁
void rwlock_release_readlock(rwlock_t *lock)
{
    Sem_wait(&lock->lock); // 加锁保护readers
    lock->readers--;       // 读者数量-1
    // 最后一个读者释放写锁（允许写操作）
    if (lock->readers == 0)
        Sem_post(&lock->writelock);
    Sem_post(&lock->lock); // 解锁
}

// 获取写锁（排他）
void rwlock_acquire_writelock(rwlock_t *lock)
{
    Sem_wait(&lock->writelock); // 获取写锁（阻止所有读写）
}

// 释放写锁
void rwlock_release_writelock(rwlock_t *lock)
{
    Sem_post(&lock->writelock); // 释放写锁
}

int read_loops;  // 读者循环次数
int write_loops; // 写者循环次数
int counter = 0; // 共享计数器

rwlock_t mutex; // 全局读写锁

// 读者线程：循环读取计数器
void *reader(void *arg)
{
    int i;
    int local = 0;
    for (i = 0; i < read_loops; i++)
    {
        rwlock_acquire_readlock(&mutex); // 获取读锁
        local = counter;                 // 读取计数器
        rwlock_release_readlock(&mutex); // 释放读锁
        printf("read %d\n", local);
    }
    printf("read done: %d\n", local);
    return NULL;
}

// 写者线程：循环修改计数器
void *writer(void *arg)
{
    int i;
    for (i = 0; i < write_loops; i++)
    {
        rwlock_acquire_writelock(&mutex); // 获取写锁
        counter++;                        // 修改计数器
        rwlock_release_writelock(&mutex); // 释放写锁
    }
    printf("write done\n");
    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "usage: rwlock readloops writeloops\n");
        exit(1);
    }
    read_loops = atoi(argv[1]);  // 读者循环次数
    write_loops = atoi(argv[2]); // 写者循环次数

    rwlock_init(&mutex); // 初始化读写锁

    pthread_t c1, c2;
    Pthread_create(&c1, NULL, reader, NULL); // 创建读者
    Pthread_create(&c2, NULL, writer, NULL); // 创建写者

    Pthread_join(c1, NULL); // 等待读者完成
    Pthread_join(c2, NULL); // 等待写者完成

    printf("all done\n");
    return 0;
}