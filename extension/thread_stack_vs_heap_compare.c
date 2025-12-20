// 对应章节：threads-api.pdf p4
// To compile: gcc -o thread_stack_vs_heap_compare thread_stack_vs_heap_compare.c -lpthread
// To run:     ./thread_stack_vs_heap_compare

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>

// 定义线程输入参数结构体（两个线程共用）
typedef struct
{
    int a; // 输入参数1
    int b; // 输入参数2
} myarg_t;

// 定义线程返回值结构体（两个线程共用）
typedef struct
{
    int x; // 返回结果1
    int y; // 返回结果2
} myret_t;

// 错误线程：返回栈局部变量指针
void *bad_mythread(void *arg)
{
    myarg_t *args = (myarg_t *)arg;
    printf("\n【错误线程】接收参数：a=%d, b=%d\n", args->a, args->b);

    // 致命错误：线程栈上的局部变量，线程结束后栈空间被回收
    myret_t oops; // ALLOCATED ON STACK: BAD!
    oops.x = 1;
    oops.y = 2;

    printf("【错误线程】准备返回栈变量地址，此时oops.x=%d, oops.y=%d\n", oops.x, oops.y);
    printf("【错误线程】注意：线程结束后，栈变量oops的内存会被系统回收，返回的地址将无效\n");
    // 返回栈局部变量的地址（线程结束后该地址无效）
    return (void *)&oops;
}

// 正确线程：返回堆内存指针
void *good_mythread(void *arg)
{
    myarg_t *args = (myarg_t *)arg;
    printf("\n【正确线程】接收参数：a=%d, b=%d\n", args->a, args->b);

    // 正确：堆上分配内存，生命周期不受线程结束影响
    myret_t *result = (myret_t *)malloc(sizeof(myret_t));
    assert(result != NULL); // 检查内存分配是否成功

    result->x = 1;
    result->y = 2;
    printf("【正确线程】准备返回堆变量地址，此时result->x=%d, result->y=%d\n", result->x, result->y);
    printf("【正确线程】注意：线程结束后，堆变量result的内存仍有效，需主线程手动释放\n");
    // 返回堆内存地址（线程结束后仍有效）
    return (void *)result;
}

int main()
{
    // 1. 定义线程标识符和参数
    pthread_t bad_tid, good_tid; // 错误线程/正确线程的标识符
    myarg_t bad_args, good_args; // 两个线程的输入参数
    void *bad_result_raw;        // 仅接收错误线程的原始返回值（不直接访问）
    myret_t *good_result;        // 接收正确线程的返回值

    // 2. 初始化参数
    bad_args.a = 10;
    bad_args.b = 20; // 错误线程参数
    good_args.a = 30;
    good_args.b = 40; // 正确线程参数

    // 3. 先创建并运行正确线程（确保能看到正确效果）
    int rc = pthread_create(&good_tid, NULL, good_mythread, &good_args);
    assert(rc == 0);
    // 4. 再创建并运行错误线程
    rc = pthread_create(&bad_tid, NULL, bad_mythread, &bad_args);
    assert(rc == 0);

    // 5. 等待正确线程执行完毕，获取并访问有效返回值
    rc = pthread_join(good_tid, (void **)&good_result);
    assert(rc == 0);
    printf("\n====================================\n");
    printf("【主线程】获取正确线程返回值：x=%d, y=%d（结果始终稳定）\n",
           good_result->x, good_result->y);

    // 6. 等待错误线程执行完毕，仅接收返回值（不访问野指针内容）
    rc = pthread_join(bad_tid, &bad_result_raw);
    assert(rc == 0);
    printf("\n【主线程】获取错误线程返回的地址：%p（该地址已无效，禁止访问内容）\n",
           bad_result_raw);
    printf("【主线程】提示：若强行访问该地址的x/y，会触发段错误（非法内存访问）\n");

    // 7. 释放正确线程的堆内存（避免内存泄漏）
    free(good_result);
    good_result = NULL; // 避免野指针

    printf("\n====================================\n");
    printf("完整对比总结：\n");
    printf("1. 错误线程：返回栈地址 → 线程结束后地址无效 → 访问会段错误\n");
    printf("2. 正确线程：返回堆地址 → 线程结束后地址有效 → 访问稳定，需手动free\n");
    return 0;
}