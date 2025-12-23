#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "common.h"
#include "common_threads.h"

// 两个全局互斥锁：模拟可能引发死锁的资源
pthread_mutex_t L1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t L2 = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief 线程1：先获取L1，再尝试获取L2
 * 死锁成因之一：锁的获取顺序与线程2相反
 */
void *thread1(void *arg)
{
    printf("t1: begin\n");
    printf("t1: try to acquire L1...\n");
    Pthread_mutex_lock(&L1); // 获取锁L1
    printf("t1: L1 acquired\n");
    printf("t1: try to acquire L2...\n");
    // 若此时L2被线程2持有，线程1会阻塞等待
    Pthread_mutex_lock(&L2);
    printf("t1: L2 acquired\n");
    // 释放锁（正常流程，死锁时不会执行到此处）
    Pthread_mutex_unlock(&L1);
    Pthread_mutex_unlock(&L2);
    return NULL;
}

/**
 * @brief 线程2：先获取L2，再尝试获取L1
 * 死锁成因之一：锁的获取顺序与线程1相反，形成循环等待
 */
void *thread2(void *arg)
{
    printf("                           t2: begin\n");
    printf("                           t2: try to acquire L2...\n");
    Pthread_mutex_lock(&L2); // 获取锁L2
    printf("                           t2: L2 acquired\n");
    printf("                           t2: try to acquire L1...\n");
    // 若此时L1被线程1持有，线程2会阻塞等待
    Pthread_mutex_lock(&L1);
    printf("                           t2: L1 acquired\n");
    // 释放锁（正常流程，死锁时不会执行到此处）
    Pthread_mutex_unlock(&L1);
    Pthread_mutex_unlock(&L2);
    return NULL;
}

/**
 * @brief 主函数：创建两个线程，演示死锁
 * 死锁条件：互斥、持有并等待、不可剥夺、循环等待均满足时触发
 */
int main(int argc, char *argv[])
{
    if (argc != 1)
    {
        fprintf(stderr, "usage: main\n");
        exit(1);
    }
    pthread_t p1, p2;
    printf("main: begin\n");
    Pthread_create(&p1, NULL, thread1, NULL); // 创建线程1
    Pthread_create(&p2, NULL, thread2, NULL); // 创建线程2
    // 死锁发生时，线程无法结束，join会一直阻塞
    Pthread_join(p1, NULL);
    Pthread_join(p2, NULL);
    printf("main: end\n"); // 死锁时不会打印
    return 0;
}