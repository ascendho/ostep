#ifndef __zemaphore_h__
#define __zemaphore_h__

#include <pthread.h>

// 自定义信号量结构体
// 模拟POSIX sem_t，解决跨平台（Linux/Apple）兼容性
typedef struct __Zem_t
{
    int value;            // 信号量值：>0表示可用资源数；=0表示无资源；<0表示等待线程数
    pthread_cond_t cond;  // 条件变量：用于线程等待/唤醒
    pthread_mutex_t lock; // 互斥锁：保护信号量值的原子操作
} Zem_t;

// 初始化信号量
// 参数：z-信号量指针；value-初始值（资源数量）
void Zem_init(Zem_t *z, int value)
{
    z->value = value;
    Cond_init(&z->cond);  // 初始化条件变量（封装pthread_cond_init）
    Mutex_init(&z->lock); // 初始化互斥锁（封装pthread_mutex_init）
}

// 等待信号量（P操作）
// 功能：获取资源，若资源不足则阻塞等待
void Zem_wait(Zem_t *z)
{
    Mutex_lock(&z->lock); // 加锁保护value
    // 循环检查（避免虚假唤醒）
    while (z->value <= 0)
        Cond_wait(&z->cond, &z->lock); // 释放锁并等待唤醒
    z->value--;                        // 资源数减1
    Mutex_unlock(&z->lock);            // 解锁
}

// 释放信号量（V操作）
// 功能：释放资源，若有等待线程则唤醒一个
void Zem_post(Zem_t *z)
{
    Mutex_lock(&z->lock);   // 加锁保护value
    z->value++;             // 资源数加1
    Cond_signal(&z->cond);  // 唤醒一个等待线程
    Mutex_unlock(&z->lock); // 解锁
}

// 苹果系统兼容：将Zem_t映射为sem_t，统一接口
#ifdef __APPLE__
typedef Zem_t sem_t;
#define Sem_wait(s) Zem_wait(s)
#define Sem_post(s) Zem_post(s)
#define Sem_init(s, v) Zem_init(s, v)
#endif

#endif // __zemaphore_h__