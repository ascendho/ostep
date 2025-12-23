#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "common.h"
#include "common_threads.h"

#define PR_STATE_INIT (0) // 线程初始状态

// 线程信息结构体
typedef struct
{
    pthread_t Tid;
    int State;
} pr_thread_t;

/**
 * @brief 创建线程并初始化线程信息
 * 修复：移除sleep，通过同步机制确保顺序，而非依赖延迟
 */
pr_thread_t *PR_CreateThread(void *(*start_routine)(void *))
{
    pr_thread_t *p = malloc(sizeof(pr_thread_t));
    if (p == NULL)
        return NULL;
    p->State = PR_STATE_INIT;
    Pthread_create(&p->Tid, NULL, start_routine, NULL);
    // 移除sleep，避免依赖不可靠的延迟
    return p;
}

/**
 * @brief 等待线程结束
 */
void PR_WaitThread(pr_thread_t *p)
{
    Pthread_join(p->Tid, NULL);
}

pr_thread_t *mThread; // 需主线程初始化的全局指针

// 同步机制：互斥锁和条件变量，确保子线程等待mThread初始化
pthread_mutex_t mtLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t mtCond = PTHREAD_COND_INITIALIZER;
int mtInit = 0; // 标记mThread是否已初始化（0：未初始化，1：已初始化）

/**
 * @brief 子线程主函数：等待mThread初始化后再访问
 * 修复：通过条件变量等待主线程完成mThread的初始化
 */
void *mMain(void *arg)
{
    printf("mMain: begin\n");

    // 等待mThread初始化完成
    Pthread_mutex_lock(&mtLock);
    // 循环等待：防止虚假唤醒
    while (mtInit == 0)
        Pthread_cond_wait(&mtCond, &mtLock); // 释放锁并等待信号
    Pthread_mutex_unlock(&mtLock);

    // 安全访问已初始化的mThread
    int mState = mThread->State;
    printf("mMain: state is %d\n", mState);
    return NULL;
}

/**
 * @brief 主函数：创建线程并通过同步机制确保初始化顺序
 * 修复：使用条件变量通知子线程mThread已初始化
 */
int main(int argc, char *argv[])
{
    printf("ordering: begin\n");
    mThread = PR_CreateThread(mMain); // 创建线程

    // 通知子线程：mThread已初始化
    Pthread_mutex_lock(&mtLock);
    mtInit = 1;                   // 更新状态
    Pthread_cond_signal(&mtCond); // 发送信号唤醒子线程
    Pthread_mutex_unlock(&mtLock);

    PR_WaitThread(mThread); // 等待子线程结束
    printf("ordering: end\n");
    return 0;
}