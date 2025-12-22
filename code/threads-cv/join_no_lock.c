#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "common.h"
#include "common_threads.h"

pthread_cond_t c = PTHREAD_COND_INITIALIZER;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
int done = 0;

// 子线程：更新状态时未加锁（错误操作）
void *child(void *arg)
{
    printf("child: begin\n");
    sleep(1);
    done = 1; // 未加锁修改共享变量（数据竞争）
    printf("child: signal\n");
    Cond_signal(&c); // 发送信号时未加锁（不符合规范）
    return NULL;
}

int main(int argc, char *argv[])
{
    pthread_t p;
    printf("parent: begin\n");
    Pthread_create(&p, NULL, child, NULL);

    Mutex_lock(&m);
    printf("parent: check condition\n");
    // 循环等待：但子线程修改done时未加锁，可能导致主线程永远等待
    while (done == 0)
    {
        sleep(2);
        printf("parent: wait to be signalled...\n");
        Cond_wait(&c, &m);
    }
    Mutex_unlock(&m);
    printf("parent: end\n");
    return 0;
}