# threads-api

## 核心结论
POSIX线程（pthread）API的核心是**三大组件**：线程创建/等待、锁（互斥量）、条件变量，它们共同支撑多线程程序的并发控制。使用关键是：通过`pthread_create`创建线程、`pthread_join`等待线程完成，用锁保证临界区互斥，用条件变量实现线程间信号通信；必须重视初始化、错误检查、避免栈指针返回等细节，否则会导致程序不稳定或逻辑错误。

## 一、线程创建（Thread Creation）
### 1. 核心接口
```c
int pthread_create(pthread_t *thread, const pthread_attr_t *attr, 
                   void *(*start_routine)(void*), void *arg);
```
### 2. 参数解析
- `thread`：线程标识符指针，用于后续操作（如等待线程），由API初始化；
- `attr`：线程属性（如栈大小、调度优先级），默认传`NULL`；
- `start_routine`：线程入口函数（函数指针），格式要求：参数为`void*`，返回值为`void*`，支持传递任意类型数据；
- `arg`：传递给入口函数的参数，通过`void*`适配任意类型。

### 3. 灵活传参示例
- 传递多个参数：封装为结构体，将结构体指针作为`arg`传入；
- 传递单个简单参数（如整数）：直接将值强制转换为`void*`（适用于`long long int`等长度匹配指针的类型）。

## 二、线程完成与返回值（Thread Completion）
### 1. 等待线程接口
```c
int pthread_join(pthread_t thread, void **value_ptr);
```
### 2. 关键作用
- 阻塞调用线程，直到目标线程执行完毕；
- 接收目标线程的返回值：`value_ptr`是指向“返回值指针”的指针，线程通过`return (void*)result`返回结果。

### 3. 返回值传递注意事项
- 允许返回任意类型数据（通过`void*`），但**禁止返回线程栈上的局部变量指针**（线程结束后栈空间释放，指针失效）；
- 若无需返回值，`value_ptr`传`NULL`；
- 简单场景可直接返回数值（如`return (void*)100`），接收时再强制转换。

## 三、锁（互斥量，Mutex）：保证临界区互斥
### 1. 核心接口
```c
// 加锁：未获取到锁则阻塞
int pthread_mutex_lock(pthread_mutex_t *mutex);
// 解锁：仅持有锁的线程可调用
int pthread_mutex_unlock(pthread_mutex_t *mutex);
```
### 2. 初始化方式
- 静态初始化（全局/静态锁）：`pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;`
- 动态初始化（运行时）：`pthread_mutex_init(&lock, NULL);`（需搭配`pthread_mutex_destroy(&lock)`销毁）；
- 必须初始化：未初始化的锁会导致程序行为异常。

### 3. 扩展接口
- `pthread_mutex_trylock`：尝试加锁，失败立即返回（不阻塞）；
- `pthread_mutex_timedlock`：超时未获取锁则返回，避免无限阻塞；
- 适用场景：死锁检测等需避免长期阻塞的场景，一般场景优先用`lock/unlock`。

### 4. 使用规范
- 临界区前后必须配对调用`lock`和`unlock`；
- 加锁后临界区应尽量简短，减少阻塞其他线程的时间；
- 必须检查返回值（可通过封装函数断言成功），避免锁操作失败导致竞态条件。

## 四、条件变量（Condition Variables）：线程间信号通信
### 1. 核心接口
```c
// 等待信号：需配合锁使用
int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
// 发送信号：唤醒一个等待的线程
int pthread_cond_signal(pthread_cond_t *cond);
```
### 2. 核心作用
实现线程间“等待-唤醒”逻辑（如线程A等待线程B完成某操作后再执行），替代低效的自旋等待。

### 3. 关键使用规则
- 必须与锁配合：调用`wait`或`signal`时，调用线程必须持有对应的锁；
- `pthread_cond_wait`的行为：调用后释放锁，让其他线程可获取锁；被唤醒后会**重新获取锁**，再返回；
- 用`while`循环检查条件：避免虚假唤醒（部分实现可能无信号却唤醒线程），确保条件满足后再执行后续逻辑：
  ```c
  Pthread_mutex_lock(&lock);
  while (condition == false) { // 用while而非if
      Pthread_cond_wait(&cond, &lock);
  }
  Pthread_mutex_unlock(&lock);
  ```
- 初始化：支持静态（`PTHREAD_COND_INITIALIZER`）和动态（`pthread_cond_init`）两种方式，需对应销毁。

### 4. 优势 vs 自旋等待
- 自旋等待（`while(flag==0);`）浪费CPU，且易出错；
- 条件变量让线程休眠，不占用CPU，且通过锁保证数据一致性，是线程间通信的标准方式。

## 五、编译与运行
### 1. 编译要求
- 包含头文件：`#include <pthread.h>`；
- 链接线程库：编译时加`-pthread`选项（如`gcc -o main main.c -Wall -pthread`）。

## 六、API使用指南（避坑关键）
1. **保持简单**：线程交互逻辑尽量简洁，复杂交互易引发bug；
2. **最小化线程交互**：减少线程间共享资源和通信方式，每个交互都需谨慎设计；
3. **必须初始化**：锁和条件变量必须通过静态或动态方式初始化，否则行为未定义；
4. **检查返回值**：所有pthread接口都可能失败，需检查返回码（或用封装函数断言成功），避免静默错误；
5. **谨慎传递参数/返回值**：
   - 不传递栈上变量的指针（线程栈独立，其他线程访问可能失效）；
   - 不返回栈上变量的指针（线程结束后栈空间释放）；
6. **线程栈私有**：线程局部变量（栈上变量）仅当前线程可访问，共享数据需放在堆或全局区域；
7. **用条件变量通信**：避免用简单标志位同步，易出错且效率低；
8. **参考手册**：Linux下用`man -k pthread`查看完整API细节。

## 七、核心总结
1. API核心逻辑：线程创建/等待是基础，锁保证互斥，条件变量实现通信，三者缺一不可；
2. 细节决定稳定性：初始化、错误检查、返回值传递等细节，是避免程序崩溃或逻辑错误的关键；
3. 适用场景：多线程并行计算（用线程拆分任务）、I/O密集型任务（用线程重叠I/O与计算）、服务器并发处理（用线程响应多请求）。

## 八、作业方向（Code）
通过`helgrind`工具（Valgrind组件）检测多线程程序问题，核心任务：
1. 分析`main-race.c`的竞态条件，用helgrind验证，添加锁后观察检测结果；
2. 分析`main-deadlock.c`的死锁问题，用helgrind定位，对比`main-deadlock-global.c`的差异；
3. 分析`main-signal.c`的低效原因（自旋等待），对比`main-signal-cv.c`（条件变量）的优势，验证helgrind检测结果；
4. 编写简单多线程程序，练习线程创建、锁、条件变量的组合使用，用helgrind排查问题。
