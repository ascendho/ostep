#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
// 引入封装的线程工具函数（含断言检查）
#include "common_threads.h"

// 定义线程输入参数结构体：传递给线程的多个参数
typedef struct
{
    int a; // 输入参数a
    int b; // 输入参数b
} myarg_t;

// 定义线程返回值结构体：线程执行后返回的多个结果
typedef struct
{
    int x; // 返回结果x
    int y; // 返回结果y
} myret_t;

// 线程执行函数：接收myarg_t参数，处理后返回myret_t结果
// 参数arg：输入参数（myarg_t*类型）
void *mythread(void *arg)
{
    // 将输入参数转换为myarg_t*类型
    myarg_t *args = (myarg_t *)arg;
    // 打印输入参数a和b的值
    printf("args %d %d\n", args->a, args->b);

    // 动态分配返回值结构体（必须用malloc，避免栈内存随线程结束释放）
    myret_t *rvals = malloc(sizeof(myret_t));
    assert(rvals != NULL); // 检查内存分配是否成功

    // 设置返回值内容
    rvals->x = 1;
    rvals->y = 2;

    // 返回动态分配的结构体指针（主线程需负责释放）
    return (void *)rvals;
}

int main(int argc, char *argv[])
{
    pthread_t p;    // 线程标识符
    myret_t *rvals; // 用于接收线程返回的结果结构体
    // 初始化输入参数结构体：a=10，b=20
    myarg_t args = {10, 20};

    // 创建线程：执行mythread函数，传递args作为参数
    Pthread_create(&p, NULL, mythread, &args);

    // 等待线程执行完毕，获取返回的结果结构体指针
    Pthread_join(p, (void **)&rvals);

    // 打印线程返回的结果x和y
    printf("returned %d %d\n", rvals->x, rvals->y);
    // 释放线程动态分配的内存（避免内存泄漏）
    free(rvals);

    return 0;
}