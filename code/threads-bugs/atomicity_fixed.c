#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "common.h"
#include "common_threads.h"

// 互斥锁：保护共享资源proc_info的访问，确保检查和使用的原子性
pthread_mutex_t proc_info_lock = PTHREAD_MUTEX_INITIALIZER;

// 进程信息结构体
typedef struct
{
    int pid;
} proc_t;

// 线程信息结构体（含共享指针）
typedef struct
{
    proc_t *proc_info;
} thread_info_t;

proc_t p;
thread_info_t th;
thread_info_t *thd; // 共享指针

/**
 * @brief 线程1：使用互斥锁保护共享资源的检查和使用
 * 修复原子性问题：通过加锁确保"检查-使用"操作的原子性
 */
void *thread1(void *arg)
{
    printf("t1: before check\n");
    Pthread_mutex_lock(&proc_info_lock); // 获取锁，独占共享资源
    // 检查和使用在同一临界区，避免被线程2中断
    if (thd->proc_info)
    {
        printf("t1: after check\n");
        sleep(2); // 即使休眠，锁仍持有，线程2无法修改
        printf("t1: use!\n");
        printf("%d\n", thd->proc_info->pid); // 安全访问
    }
    Pthread_mutex_unlock(&proc_info_lock); // 释放锁
    return NULL;
}

/**
 * @brief 线程2：修改共享资源前获取锁，避免与线程1冲突
 */
void *thread2(void *arg)
{
    printf("                 t2: begin\n");
    sleep(1);                            // 等待线程1进入临界区
    Pthread_mutex_lock(&proc_info_lock); // 获取锁（若线程1持有则阻塞）
    printf("                 t2: set to NULL\n");
    thd->proc_info = NULL;                 // 安全修改共享资源
    Pthread_mutex_unlock(&proc_info_lock); // 释放锁
    return NULL;
}

/**
 * @brief 主函数：初始化资源并创建线程
 * 修复方案：通过互斥锁保护共享资源的访问，确保原子操作
 */
int main(int argc, char *argv[])
{
    if (argc != 1)
    {
        fprintf(stderr, "usage: main\n");
        exit(1);
    }
    // 初始化共享资源
    thread_info_t t;
    p.pid = 100;
    t.proc_info = &p;
    thd = &t;

    pthread_t p1, p2;
    printf("main: begin\n");
    Pthread_create(&p1, NULL, thread1, NULL);
    Pthread_create(&p2, NULL, thread2, NULL);
    Pthread_join(p1, NULL);
    Pthread_join(p2, NULL);
    printf("main: end\n");
    return 0;
}