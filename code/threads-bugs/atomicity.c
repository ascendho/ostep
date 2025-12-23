#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "common.h"         // 通用工具函数
#include "common_threads.h" // 线程操作封装（带错误检查）

// 进程信息结构体：存储进程ID
typedef struct
{
    int pid;
} proc_t;

// 线程信息结构体：指向进程信息的指针（共享资源）
typedef struct
{
    proc_t *proc_info;
} thread_info_t;

// 全局变量：进程实例和线程信息
proc_t p;
thread_info_t th;
thread_info_t *thd; // 指向线程信息的指针（共享变量）

/**
 * @brief 线程1：检查并使用共享资源proc_info
 * 原子性问题：检查（if条件）和使用（访问proc_info）之间无锁保护，
 * 可能被线程2修改proc_info导致空指针访问
 */
void *thread1(void *arg)
{
    printf("t1: before check\n");
    // 检查共享资源是否有效（无锁保护，存在竞态条件）
    if (thd->proc_info)
    {
        printf("t1: after check\n");
        sleep(2); // 模拟检查后、使用前的时间窗口，给线程2修改机会
        printf("t1: use!\n");
        // 可能访问已被线程2设为NULL的指针，导致未定义行为
        printf("%d\n", thd->proc_info->pid);
    }
    return NULL;
}

/**
 * @brief 线程2：修改共享资源proc_info为NULL
 * 在thread1的检查和使用之间修改共享资源，破坏原子性
 */
void *thread2(void *arg)
{
    printf("                 t2: begin\n");
    sleep(1); // 确保在thread1检查之后、使用之前执行
    printf("                 t2: set to NULL\n");
    thd->proc_info = NULL; // 修改共享资源，导致thread1可能访问空指针
    return NULL;
}

/**
 * @brief 主函数：初始化共享资源并创建线程
 * 演示原子性问题：无锁保护时，共享资源的检查和使用非原子操作导致的错误
 */
int main(int argc, char *argv[])
{
    if (argc != 1)
    {
        fprintf(stderr, "usage: main\n");
        exit(1);
    }
    // 初始化共享资源：proc_info指向p（pid=100）
    thread_info_t t;
    p.pid = 100;
    t.proc_info = &p;
    thd = &t;

    pthread_t p1, p2;
    printf("main: begin\n");
    Pthread_create(&p1, NULL, thread1, NULL); // 创建线程1
    Pthread_create(&p2, NULL, thread2, NULL); // 创建线程2
    // 等待线程结束
    Pthread_join(p1, NULL);
    Pthread_join(p2, NULL);
    printf("main: end\n");
    return 0;
}