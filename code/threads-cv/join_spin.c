#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "common.h"
#include "common_threads.h"

// 共享状态变量：volatile确保多线程可见性
volatile int done = 0;

// 子线程函数：执行任务后更新状态
void *child(void *arg)
{
    printf("child\n"); // 子线程开始
    sleep(5);          // 模拟耗时操作（5秒）
    done = 1;          // 更新状态：任务完成
    return NULL;
}

int main(int argc, char *argv[])
{
    pthread_t p;
    printf("parent: begin\n");

    Pthread_create(&p, NULL, child, NULL);

    // 自旋等待：持续检查状态，不释放CPU（忙等待）
    while (done == 0)
        ; // 空循环，浪费CPU资源

    printf("parent: end\n");
    return 0;
}