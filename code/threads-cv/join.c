#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "common.h"         // 提供时间相关函数
#include "common_threads.h" // 封装线程操作的宏（带锁、条件变量等）

// 条件变量：用于线程间等待/通知
pthread_cond_t c = PTHREAD_COND_INITIALIZER;
// 互斥锁：保护共享变量和条件变量操作的原子性
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
// 共享状态变量：标记子线程是否完成
int done = 0;

// 子线程函数：执行任务后更新状态并通知主线程
void *child(void *arg)
{
    printf("child\n"); // 子线程开始执行
    sleep(1);          // 模拟耗时操作（1秒）
    Mutex_lock(&m);    // 加锁保护共享变量
    done = 1;          // 更新状态：任务完成
    Cond_signal(&c);   // 发送信号通知主线程（条件满足）
    Mutex_unlock(&m);  // 解锁
    return NULL;
}

int main(int argc, char *argv[])
{
    pthread_t p;               // 线程标识符
    printf("parent: begin\n"); // 主线程开始

    // 创建子线程
    Pthread_create(&p, NULL, child, NULL);

    Mutex_lock(&m); // 加锁：准备检查共享状态
    // 循环等待：防止虚假唤醒（spurious wakeup）
    while (done == 0)
    {
        // 等待条件变量：自动释放锁，被唤醒时重新获取锁
        Cond_wait(&c, &m);
    }
    Mutex_unlock(&m); // 解锁

    printf("parent: end\n"); // 主线程结束
    return 0;
}