#include <stdio.h>
#include <pthread.h>
// 引入封装的线程工具函数（含断言检查）
#include "common_threads.h"

// 线程执行函数：接收整数参数，打印并返回该参数+1的值
// 参数arg：以void*传递的整数（实际为long long int类型）
void *mythread(void *arg)
{
    // 将void*参数转换为long long int（避免类型转换警告）
    long long int value = (long long int)arg;
    // 打印接收的参数值
    printf("%lld\n", value);
    // 返回参数值+1（通过void*传递整数返回值）
    return (void *)(value + 1);
}

int main(int argc, char *argv[])
{
    pthread_t p;          // 线程标识符
    long long int rvalue; // 用于存储线程返回值

    // 创建线程：执行mythread函数，传递参数100（转换为void*）
    // Pthread_create是封装宏，内部调用pthread_create并检查返回值
    Pthread_create(&p, NULL, mythread, (void *)100);

    // 等待线程执行完毕，获取返回值（存储到rvalue中）
    // Pthread_join封装了pthread_join并检查错误
    Pthread_join(p, (void **)&rvalue);

    // 打印线程返回的结果
    printf("returned %lld\n", rvalue);
    return 0;
}