#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "common.h"         // 包含通用工具函数（如错误处理封装等）
#include "common_threads.h" // 包含线程操作的封装函数（如Pthread_create、Pthread_join等，简化错误处理）

// 根据操作系统类型引入信号量库：Linux使用系统自带的semaphore.h，Apple使用自定义的zemaphore.h
#ifdef linux
#include <semaphore.h>
#elif __APPLE__
#include "zemaphore.h"
#endif

/**
 * @brief 线程参数结构体：存储每个哲学家线程的配置信息
 */
typedef struct
{
    int num_loops; // 每个哲学家的"思考-进食"循环次数
    int thread_id; // 哲学家的唯一标识（0-4，用于区分不同哲学家）
} arg_t;

// 全局信号量数组：模拟5个叉子，每个信号量初始值为1（代表叉子初始状态为"可用"）
sem_t forks[5];
// 打印锁信号量：保证多线程打印输出不混乱（互斥访问标准输出资源）
sem_t print_lock;

/**
 * @brief 打印指定数量的空格，用于对齐不同哲学家的输出信息
 * @param s 哲学家ID（0-4），每个ID对应固定数量的空格（s*10个）
 * 注：通过获取print_lock信号量，确保空格打印过程不被其他线程打断
 */
void space(int s)
{
    Sem_wait(&print_lock); // 获取打印锁，独占标准输出
    int i;
    for (i = 0; i < s * 10; i++)
        printf(" "); // 输出s*10个空格，实现按哲学家ID对齐
}

/**
 * @brief 释放打印锁，允许其他线程使用标准输出
 */
void space_end()
{
    Sem_post(&print_lock); // 释放打印锁，唤醒等待打印资源的线程
}

/**
 * @brief 计算哲学家左手边叉子的索引
 * @param p 哲学家ID（0-4）
 * @return 左手边叉子的索引（与哲学家ID相同，即叉子p）
 */
int left(int p)
{
    return p;
}

/**
 * @brief 计算哲学家右手边叉子的索引
 * @param p 哲学家ID（0-4）
 * @return 右手边叉子的索引（环形排列，即(p+1)%5）
 */
int right(int p)
{
    return (p + 1) % 5;
}

/**
 * @brief 哲学家获取叉子的操作（死锁根源）
 * @param p 哲学家ID（0-4）
 * 死锁原因：所有哲学家均先获取左手边叉子，再尝试获取右手边叉子。
 * 当所有哲学家同时拿起左手边叉子后，会互相等待对方的右手边叉子，形成循环等待，导致死锁。
 */
void get_forks(int p)
{
    space(p);
    printf("%d: try %d\n", p, left(p));
    space_end();               // 打印尝试获取左叉的信息
    Sem_wait(&forks[left(p)]); // 获取左叉（信号量-1，若叉子被占用则阻塞等待）
    space(p);
    printf("%d: try %d\n", p, right(p));
    space_end();                // 打印尝试获取右叉的信息
    Sem_wait(&forks[right(p)]); // 获取右叉（信号量-1，若叉子被占用则阻塞等待）
}

/**
 * @brief 哲学家释放叉子的操作
 * @param p 哲学家ID（0-4）
 * 释放顺序：先释放左叉，再释放右叉（释放后信号量+1，唤醒等待该叉子的线程）
 */
void put_forks(int p)
{
    Sem_post(&forks[left(p)]);  // 释放左叉
    Sem_post(&forks[right(p)]); // 释放右叉
}

/**
 * @brief 模拟哲学家思考的过程（简化实现，实际可添加随机延迟模拟思考时间）
 */
void think()
{
    return;
}

/**
 * @brief 模拟哲学家进食的过程（简化实现，实际可添加随机延迟模拟进食时间）
 */
void eat()
{
    return;
}

/**
 * @brief 哲学家线程的主函数：循环执行"思考-拿叉-进食-放叉"流程
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

    // 循环指定次数，执行"思考-拿叉-进食-放叉"
    int i;
    for (i = 0; i < args->num_loops; i++)
    {
        space(args->thread_id);
        printf("%d: think\n", args->thread_id);
        space_end();
        think();                    // 思考
        get_forks(args->thread_id); // 获取叉子（可能导致死锁）
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
 * @brief 主函数：初始化信号量、创建哲学家线程、等待线程结束
 * @param argc 命令行参数数量（需为2，即程序名+循环次数）
 * @param argv 命令行参数：argv[1]为每个哲学家的循环次数
 * @return 0表示程序正常结束
 */
int main(int argc, char *argv[])
{
    // 检查命令行参数是否正确（必须提供循环次数）
    if (argc != 2)
    {
        fprintf(stderr, "usage: dining_philosophers <num_loops>\n");
        exit(1);
    }
    printf("dining: started\n"); // 打印程序启动信息

    // 初始化5个叉子信号量（初始值为1，代表每个叉子初始可用）
    int i;
    for (i = 0; i < 5; i++)
        Sem_init(&forks[i], 1); // 第二个参数1表示信号量在进程内的线程间共享
    // 初始化打印锁信号量（初始值为1，保证打印操作的互斥性）
    Sem_init(&print_lock, 1);

    // 创建5个哲学家线程
    pthread_t p[5]; // 存储线程ID的数组
    arg_t a[5];     // 存储每个线程参数的数组
    for (i = 0; i < 5; i++)
    {
        a[i].num_loops = atoi(argv[1]); // 设置每个哲学家的循环次数
        a[i].thread_id = i;             // 设置哲学家ID（0-4）
        // 创建线程，执行philosopher函数，传入参数a[i]
        Pthread_create(&p[i], NULL, philosopher, &a[i]);
    }

    // 等待所有哲学家线程执行完毕
    for (i = 0; i < 5; i++)
        Pthread_join(p[i], NULL);

    printf("dining: finished\n"); // 打印程序结束信息
    return 0;
}