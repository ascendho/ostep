#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "common.h"         // 包含通用工具函数定义
#include "common_threads.h" // 包含线程操作的封装函数（如Pthread_create等）

// 根据操作系统类型引入信号量头文件
#ifdef linux
#include <semaphore.h> // Linux系统的信号量库
#elif __APPLE__
#include "zemaphore.h" // Apple系统的信号量库（自定义实现）
#endif

// 线程参数结构体：存储每个哲学家线程的循环次数和线程ID
typedef struct
{
    int num_loops; // 每个哲学家的思考-进食循环次数
    int thread_id; // 哲学家的唯一标识（0-4）
} arg_t;

// 全局信号量数组：模拟5个叉子（每个信号量初始值为1，代表资源可用）
sem_t forks[5];
// 打印锁信号量：保证多线程打印输出不混乱（互斥访问打印资源）
sem_t print_lock;

/**
 * @brief 打印指定数量的空格，用于对齐不同哲学家的输出
 * @param s 哲学家ID，每个ID对应固定数量的空格（s*10个）
 * 注：通过获取print_lock保证打印空格的原子性，避免多线程输出交错
 */
void space(int s)
{
    Sem_wait(&print_lock); // 获取打印锁，独占打印资源
    int i;
    for (i = 0; i < s * 10; i++)
        printf(" ");
}

/**
 * @brief 释放打印锁，允许其他线程进行打印操作
 */
void space_end()
{
    Sem_post(&print_lock); // 释放打印锁
}

/**
 * @brief 计算哲学家左手边叉子的索引
 * @param p 哲学家ID（0-4）
 * @return 左手边叉子的索引（与哲学家ID相同）
 */
int left(int p)
{
    return p;
}

/**
 * @brief 计算哲学家右手边叉子的索引
 * @param p 哲学家ID（0-4）
 * @return 右手边叉子的索引（(p+1)%5，环形排列）
 */
int right(int p)
{
    return (p + 1) % 5;
}

/**
 * @brief 哲学家获取叉子的操作（核心：避免死锁的逻辑）
 * @param p 哲学家ID（0-4）
 * 死锁避免策略：最后一个哲学家（ID=4）反转拿叉子顺序，先拿右叉再拿左叉，
 * 打破"每个哲学家都先拿左叉再等右叉"的循环等待条件
 */
void get_forks(int p)
{
    if (p == 4)
    { // 特殊处理哲学家4
        space(p);
        printf("4 try %d\n", right(p));
        space_end();                // 打印尝试拿右叉
        Sem_wait(&forks[right(p)]); // 获取右叉（信号量-1，若不可用则阻塞）
        space(p);
        printf("4 try %d\n", left(p));
        space_end();               // 打印尝试拿左叉
        Sem_wait(&forks[left(p)]); // 获取左叉
    }
    else
    { // 哲学家0-3：正常顺序拿叉
        space(p);
        printf("try %d\n", left(p));
        space_end();               // 打印尝试拿左叉
        Sem_wait(&forks[left(p)]); // 获取左叉
        space(p);
        printf("try %d\n", right(p));
        space_end();                // 打印尝试拿右叉
        Sem_wait(&forks[right(p)]); // 获取右叉
    }
}

/**
 * @brief 哲学家释放叉子的操作
 * @param p 哲学家ID（0-4）
 * 释放顺序：先左后右（与拿取顺序无关，释放资源即可）
 */
void put_forks(int p)
{
    Sem_post(&forks[left(p)]);  // 释放左叉（信号量+1，唤醒等待该叉子的线程）
    Sem_post(&forks[right(p)]); // 释放右叉
}

/**
 * @brief 模拟哲学家思考的过程（此处简化为空操作，实际可添加延迟）
 */
void think()
{
    return;
}

/**
 * @brief 模拟哲学家进食的过程（此处简化为空操作，实际可添加延迟）
 */
void eat()
{
    return;
}

/**
 * @brief 哲学家线程的主函数：循环执行思考-拿叉-进食-放叉流程
 * @param arg 线程参数（arg_t类型，包含循环次数和线程ID）
 * @return 无返回值
 */
void *philosopher(void *arg)
{
    arg_t *args = (arg_t *)arg; // 解析线程参数

    // 打印线程启动信息
    space(args->thread_id);
    printf("%d: start\n", args->thread_id);
    space_end();

    // 循环指定次数：思考->拿叉->进食->放叉
    int i;
    for (i = 0; i < args->num_loops; i++)
    {
        space(args->thread_id);
        printf("%d: think\n", args->thread_id);
        space_end();
        think();                    // 思考
        get_forks(args->thread_id); // 获取叉子
        space(args->thread_id);
        printf("%d: eat\n", args->thread_id);
        space_end();
        eat();                      // 进食
        put_forks(args->thread_id); // 释放叉子
        space(args->thread_id);
        printf("%d: done\n", args->thread_id);
        space_end();
    }
    return NULL;
}

/**
 * @brief 主函数：初始化信号量、创建线程、等待线程结束
 * @param argc 命令行参数数量（需为2）
 * @param argv 命令行参数：argv[1]为循环次数
 * @return 0表示程序正常结束
 */
int main(int argc, char *argv[])
{
    // 检查命令行参数是否正确（需传入循环次数）
    if (argc != 2)
    {
        fprintf(stderr, "usage: dining_philosophers <num_loops>\n");
        exit(1);
    }
    printf("dining: started\n"); // 程序启动提示

    // 初始化5个叉子信号量（初始值为1，代表每个叉子初始状态为可用）
    int i;
    for (i = 0; i < 5; i++)
        Sem_init(&forks[i], 1); // 第二个参数1表示信号量在进程内共享
    // 初始化打印锁信号量（初始值为1，保证打印操作互斥）
    Sem_init(&print_lock, 1);

    // 创建5个哲学家线程
    pthread_t p[5]; // 线程ID数组
    arg_t a[5];     // 线程参数数组
    for (i = 0; i < 5; i++)
    {
        a[i].num_loops = atoi(argv[1]);                  // 设置循环次数
        a[i].thread_id = i;                              // 设置线程ID
        Pthread_create(&p[i], NULL, philosopher, &a[i]); // 创建线程
    }

    // 等待所有哲学家线程执行完毕
    for (i = 0; i < 5; i++)
        Pthread_join(p[i], NULL);

    printf("dining: finished\n"); // 程序结束提示
    return 0;
}