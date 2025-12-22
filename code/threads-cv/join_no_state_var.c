#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "common.h"
#include "common_threads.h"

pthread_cond_t c = PTHREAD_COND_INITIALIZER;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

// 子线程：仅发送信号，无状态记录
void *child(void *arg)
{
    printf("child: begin\n");
    Mutex_lock(&m);
    printf("child: signal\n");
    Cond_signal(&c); // 发送信号
    Mutex_unlock(&m);
    return NULL;
}

int main(int argc, char *argv[])
{
    pthread_t p;
    printf("parent: begin\n");
    Pthread_create(&p, NULL, child, NULL);

    sleep(2); // 主线程休眠2秒（子线程可能已发送信号）
    printf("parent: wait to be signalled...\n");
    Mutex_lock(&m);
    // 无状态变量判断，直接等待：若信号已提前发送，则永久阻塞
    Cond_wait(&c, &m);
    Mutex_unlock(&m);
    printf("parent: end\n");
    return 0;
}