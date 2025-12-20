#include <assert.h>
#include <stdio.h>
#include <pthread.h>

// 定义线程参数结构体：用于传递多个整数参数给线程
typedef struct
{
    int a; // 第一个整数参数
    int b; // 第二个整数参数
} myarg_t;

// 线程执行函数：接收myarg_t类型的参数并打印其中的值
// 参数arg：传递给线程的参数（需转换为myarg_t*类型）
void *mythread(void *arg)
{
    // 将void*类型参数转换为myarg_t*，便于访问结构体成员
    myarg_t *args = (myarg_t *)arg;
    // 打印结构体中的a和b的值
    printf("%d %d\n", args->a, args->b);
    return NULL; // 线程执行完毕，返回空
}

int main(int argc, char *argv[])
{
    pthread_t p; // 线程标识符
    // 初始化参数结构体：设置a=10，b=20
    myarg_t args = {10, 20};

    // 创建线程：执行mythread函数，传递args作为参数
    // 调用原生pthread_create并通过assert检查是否创建成功
    int rc = pthread_create(&p, NULL, mythread, &args);
    assert(rc == 0); // 若创建失败，assert触发并终止程序

    // 等待线程p执行完毕（阻塞主线程）
    (void)pthread_join(p, NULL);
    // 主线程打印完成信息
    printf("done\n");
    return 0;
}