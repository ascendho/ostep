#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "common.h"
#include "common_threads.h"

#define PR_STATE_INIT (0) // 线程初始状态

// 线程信息结构体：存储线程ID和状态
typedef struct
{
    pthread_t Tid;
    int State;
} pr_thread_t;

/**
 * @brief 创建线程并初始化线程信息
 * 顺序问题：创建线程后立即返回，但线程可能在主线程初始化mThread前开始执行
 */
pr_thread_t *PR_CreateThread(void *(*start_routine)(void *))
{
    pr_thread_t *p = malloc(sizeof(pr_thread_t));
    if (p == NULL)
        return NULL;
    p->State = PR_STATE_INIT; // 初始化状态
    // 创建线程（新线程可能立即执行mMain）
    Pthread_create(&p->Tid, NULL, start_routine, NULL);
    // 注释掉sleep会增加顺序问题概率：新线程可能在主线程赋值mThread前访问它
    sleep(1);
    return p;
}

/**
 * @brief 等待线程结束
 */
void PR_WaitThread(pr_thread_t *p)
{
    Pthread_join(p->Tid, NULL);
}

pr_thread_t *mThread; // 全局指针：指向线程信息（需主线程初始化）

/**
 * @brief 子线程主函数：访问mThread->State
 * 顺序问题：可能在mThread被主线程初始化前访问，导致未定义行为
 */
void *mMain(void *arg)
{
    printf("mMain: begin\n");
    // 访问mThread（若主线程尚未赋值，则为野指针）
    int mState = mThread->State;
    printf("mMain: state is %d\n", mState); // 可能崩溃或输出错误值
    return NULL;
}

/**
 * @brief 主函数：创建线程并赋值mThread
 * 顺序问题：子线程可能在mThread赋值前执行，导致访问未初始化指针
 */
int main(int argc, char *argv[])
{
    printf("ordering: begin\n");
    // 创建线程（子线程可能立即执行mMain）
    mThread = PR_CreateThread(mMain);
    PR_WaitThread(mThread); // 等待子线程结束
    printf("ordering: end\n");
    return 0;
}