#ifndef __common_threads_h__
#define __common_threads_h__

#include <pthread.h>   // 包含POSIX线程库头文件
#include <assert.h>    // 包含断言库（用于检查函数返回值）
#include <sched.h>     // 包含调度相关函数（如线程切换）

// 仅在Linux系统下包含信号量头文件
#ifdef __linux__
#include <semaphore.h>
#endif

// 封装pthread_create：创建线程并检查是否成功（失败时assert触发）
// 参数与pthread_create一致：线程标识符、属性、执行函数、函数参数
#define Pthread_create(thread, attr, start_routine, arg) assert(pthread_create(thread, attr, start_routine, arg) == 0);

// 封装pthread_join：等待线程结束并检查是否成功
// 参数：线程标识符、接收线程返回值的指针
#define Pthread_join(thread, value_ptr)                  assert(pthread_join(thread, value_ptr) == 0);

// 封装pthread_mutex_lock：加锁并检查是否成功
#define Pthread_mutex_lock(m)                            assert(pthread_mutex_lock(m) == 0);

// 封装pthread_mutex_unlock：解锁并检查是否成功
#define Pthread_mutex_unlock(m)                          assert(pthread_mutex_unlock(m) == 0);

// 封装pthread_cond_signal：唤醒一个等待条件变量的线程并检查是否成功
#define Pthread_cond_signal(cond)                        assert(pthread_cond_signal(cond) == 0);

// 封装pthread_cond_wait：等待条件变量并检查是否成功（需传入互斥锁）
#define Pthread_cond_wait(cond, mutex)                   assert(pthread_cond_wait(cond, mutex) == 0);

// 封装pthread_mutex_init：初始化互斥锁并检查是否成功
#define Mutex_init(m)                                    assert(pthread_mutex_init(m, NULL) == 0);

// 与Pthread_mutex_lock功能相同（简化命名）
#define Mutex_lock(m)                                    assert(pthread_mutex_lock(m) == 0);

// 与Pthread_mutex_unlock功能相同（简化命名）
#define Mutex_unlock(m)                                  assert(pthread_mutex_unlock(m) == 0);

// 封装pthread_cond_init：初始化条件变量并检查是否成功
#define Cond_init(cond)                                  assert(pthread_cond_init(cond, NULL) == 0);

// 与Pthread_cond_signal功能相同（简化命名）
#define Cond_signal(cond)                                assert(pthread_cond_signal(cond) == 0);

// 与Pthread_cond_wait功能相同（简化命名）
#define Cond_wait(cond, mutex)                           assert(pthread_cond_wait(cond, mutex) == 0);

// Linux系统下封装信号量操作
#ifdef __linux__
// 初始化信号量并检查是否成功（value为初始值）
#define Sem_init(sem, value)                             assert(sem_init(sem, 0, value) == 0);
// 等待信号量（P操作）并检查是否成功
#define Sem_wait(sem)                                    assert(sem_wait(sem) == 0);
// 释放信号量（V操作）并检查是否成功
#define Sem_post(sem)                                    assert(sem_post(sem) == 0);
#endif // __linux__

#endif // __common_threads_h__