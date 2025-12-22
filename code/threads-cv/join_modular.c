#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "common.h"
#include "common_threads.h"

// 同步器结构体：封装条件变量、锁和状态（模块化设计）
typedef struct
{
    pthread_cond_t c;  // 条件变量
    pthread_mutex_t m; // 互斥锁
    int done;          // 状态变量：0=未完成，1=已完成
} synchronizer_t;

synchronizer_t s; // 全局同步器实例

// 初始化同步器
void sync_init(synchronizer_t *s)
{
    s->done = 0;       // 初始状态：未完成
    Mutex_init(&s->m); // 初始化锁
    Cond_init(&s->c);  // 初始化条件变量
}

// 发送信号：通知等待线程任务完成
void sync_signal(synchronizer_t *s)
{
    Mutex_lock(&s->m);   // 加锁保护状态
    s->done = 1;         // 更新状态
    Cond_signal(&s->c);  // 发送信号
    Mutex_unlock(&s->m); // 解锁
}

// 等待信号：阻塞直到任务完成
void sync_wait(synchronizer_t *s)
{
    Mutex_lock(&s->m);   // 加锁
    while (s->done == 0) // 循环等待（防虚假唤醒）
        Cond_wait(&s->c, &s->m);
    s->done = 0;         // 重置状态（可复用）
    Mutex_unlock(&s->m); // 解锁
}

// 子线程：执行任务后触发同步信号
void *child(void *arg)
{
    printf("child\n");
    sleep(1);        // 模拟任务
    sync_signal(&s); // 通知主线程
    return NULL;
}

int main(int argc, char *argv[])
{
    pthread_t p;
    printf("parent: begin\n");
    sync_init(&s); // 初始化同步器
    Pthread_create(&p, NULL, child, NULL);
    sync_wait(&s); // 等待子线程完成
    printf("parent: end\n");
    return 0;
}