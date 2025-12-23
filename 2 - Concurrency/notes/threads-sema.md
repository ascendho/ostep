# threads-sema

## 核心结论
信号量是一种**通用同步原语**，通过整数状态和`sem_wait()`/`sem_post()`两个核心操作，可同时实现锁（互斥）和条件变量（等待-唤醒）的功能。其核心逻辑是：`sem_wait()`原子减1，值为负则阻塞；`sem_post()`原子加1，值非负则唤醒一个阻塞线程。初始化值决定用途（1=锁、0=排序、N=资源数），关键是避免死锁（控制操作顺序）和合理利用其灵活性，解决生产者/消费者、读写锁、哲学家等经典并发问题。

## 一、核心问题（CRUX）
如何用信号量替代锁和条件变量？信号量的定义与操作规则是什么？如何通过信号量解决各类并发问题（互斥、排序、资源限制），同时避免死锁和饥饿？

## 二、信号量的定义与核心接口
### 1. 本质
信号量是带整数状态的同步对象，状态值隐含资源可用数量或等待线程数，核心是**原子操作**保证同步安全性。

### 2. POSIX核心接口
```c
#include <semaphore.h>
// 初始化：sem=信号量指针，pshared=0（线程共享），value=初始值
int sem_init(sem_t *sem, int pshared, unsigned int value);
// 等待：原子减1，值<0则阻塞（P操作）
int sem_wait(sem_t *sem);
// 唤醒：原子加1，值≤0则唤醒一个阻塞线程（V操作）
int sem_post(sem_t *sem);
```

### 3. 关键特性
- 原子性：`sem_wait()`和`sem_post()`的加减操作不可中断；
- 状态含义：值≥0表示可用资源数，值<0表示阻塞线程数（绝对值）；
- 通用性：通过初始化值适配不同场景（锁、排序、资源限流）。

## 三、信号量的核心用途
### 3.1 二进制信号量：实现锁（互斥）
#### 设计逻辑
- 初始化值=1，代表“唯一资源”，通过`sem_wait()`加锁、`sem_post()`解锁，实现临界区互斥。
#### 代码示例
```c
sem_t mutex;
sem_init(&mutex, 0, 1); // 初始化值=1（锁）

// 临界区保护
sem_wait(&mutex);  // 加锁：值1→0，无阻塞
// 临界区代码
sem_post(&mutex);  // 解锁：值0→1，无唤醒
```
#### 执行流程（双线程）
1. 线程A调用`sem_wait()`，值1→0，进入临界区；
2. 线程B调用`sem_wait()`，值0→-1，阻塞；
3. 线程A`sem_post()`，值-1→0，唤醒线程B；
4. 线程B从`sem_wait()`返回，进入临界区，结束后`sem_post()`值0→1。

### 3.2 信号量用于排序（事件同步）
#### 设计逻辑
- 初始化值=0，代表“事件未发生”，一个线程`sem_wait()`等待，另一个线程`sem_post()`触发，实现线程间顺序控制（类似`join()`）。
#### 代码示例（父线程等待子线程）
```c
sem_t s;
sem_init(&s, 0, 0); // 初始化值=0（等待事件）

void *child(void *arg) {
    printf("child\n");
    sem_post(&s); // 触发事件：值0→1，唤醒父线程
    return NULL;
}

int main() {
    pthread_create(&c, NULL, child, NULL);
    sem_wait(&s); // 等待事件：值0→-1（阻塞），被唤醒后值-1→0
    printf("parent: end\n");
    return 0;
}
```
#### 关键：覆盖两种场景
- 子线程先执行：`sem_post()`值0→1，父线程`sem_wait()`值1→0，直接返回；
- 父线程先执行：`sem_wait()`值0→-1（阻塞），子线程`sem_post()`唤醒后返回。

### 3.3 生产者/消费者（有界缓冲区）问题
#### 问题定义
生产者向缓冲区放数据，消费者取数据，缓冲区满则生产者等待，缓冲区空则消费者等待。

#### 核心信号量设计
- `empty`：初始化值=MAX（缓冲区大小），表示“空槽位数量”，生产者等待；
- `full`：初始化值=0，表示“满槽位数量”，消费者等待；
- `mutex`：初始化值=1，保证缓冲区操作（`put()`/`get()`）的互斥。

#### 错误方案（死锁风险）
```c
// 生产者：先加锁再等empty（错误）
sem_wait(&mutex);  // 加锁
sem_wait(&empty);  // 等空槽位（可能阻塞，且持有锁）
put(i);
sem_post(&full);
sem_post(&mutex);

// 消费者：先加锁再等full（错误）
sem_wait(&mutex);  // 加锁
sem_wait(&full);   // 等满槽位（阻塞，持有锁）
get();
sem_post(&empty);
sem_post(&mutex);
```
- 死锁原因：消费者持有`mutex`并阻塞在`full`，生产者等待`mutex`，循环等待。

#### 正确方案（调整操作顺序）
```c
// 生产者：先等资源，再加锁
sem_wait(&empty);  // 等空槽位（无锁）
sem_wait(&mutex);  // 加锁（临界区）
put(i);
sem_post(&mutex);  // 解锁
sem_post(&full);   // 通知消费者

// 消费者：先等资源，再加锁
sem_wait(&full);   // 等满槽位（无锁）
sem_wait(&mutex);  // 加锁（临界区）
get();
sem_post(&mutex);  // 解锁
sem_post(&empty);  // 通知生产者
```
- 核心优化：锁仅包裹临界区（`put()`/`get()`），避免阻塞时持有锁。

### 3.4 读写锁（Reader-Writer Lock）
#### 设计逻辑
- 读操作可并发，写操作独占；
- 信号量`lock`（值=1）：保护读者计数；
- 信号量`writelock`（值=1）：实现写独占，读者并发时占用该锁。

#### 核心代码
```c
typedef struct _rwlock_t {
    sem_t lock;       // 保护readers计数
    sem_t writelock;  // 写独占锁
    int readers;      // 当前读者数
} rwlock_t;

// 读锁获取：第一个读者占用writelock，后续读者直接进入
void rwlock_acquire_readlock(rwlock_t *rw) {
    sem_wait(&rw->lock);
    rw->readers++;
    if (rw->readers == 1) sem_wait(&rw->writelock); // 第一个读者锁写
    sem_post(&rw->lock);
}

// 读锁释放：最后一个读者释放writelock
void rwlock_release_readlock(rwlock_t *rw) {
    sem_wait(&rw->lock);
    rw->readers--;
    if (rw->readers == 0) sem_post(&rw->writelock); // 最后一个读者解锁
    sem_post(&rw->lock);
}

// 写锁获取/释放：直接操作writelock
void rwlock_acquire_writelock(rwlock_t *rw) { sem_wait(&rw->writelock); }
void rwlock_release_writelock(rwlock_t *rw) { sem_post(&rw->writelock); }
```
#### 缺点：可能导致写者饥饿（读者持续并发，写者无法获取锁）。

### 3.5 哲学家就餐问题
#### 问题定义
5个哲学家围坐，需拿左右两把叉子才能就餐，避免死锁（每人拿一把叉子等待另一把）。

#### 死锁方案（错误）
```c
sem_t forks[5]; // 每个叉子一个信号量（值=1）
void get_forks(int p) {
    sem_wait(&forks[left(p)]);  // 先拿左叉
    sem_wait(&forks[right(p)]); // 再拿右叉
}
```
- 死锁原因：所有哲学家同时拿左叉，等待右叉，循环等待。

#### 正确方案（打破循环）
```c
void get_forks(int p) {
    if (p == 4) { // 最后一个哲学家反向拿叉
        sem_wait(&forks[right(p)]); // 先拿右叉
        sem_wait(&forks[left(p)]);  // 再拿左叉
    } else {
        sem_wait(&forks[left(p)]);
        sem_wait(&forks[right(p)]);
    }
}
```
- 核心：改变一个哲学家的拿叉顺序，打破死锁循环。

### 3.6 线程限流（Thread Throttling）
#### 设计逻辑
- 初始化信号量值=N（最大并发数），限制同时进入临界区的线程数，避免资源耗尽（如内存、CPU）。
#### 代码示例
```c
sem_t throttle;
sem_init(&throttle, 0, 5); // 最多5个线程并发

void memory_intensive_task() {
    sem_wait(&throttle);  // 限流：值5→4...0→-1（阻塞）
    // 内存密集型操作
    sem_post(&throttle);  // 释放：值-1→0...4→5
}
```

## 四、信号量的实现（基于锁和条件变量）
信号量可通过基础同步原语实现（称为“Zemaphores”），核心是用锁保证原子性，用条件变量实现阻塞-唤醒。

```c
typedef struct __Zem_t {
    int value;          // 信号量状态值
    pthread_cond_t cond; // 条件变量（阻塞-唤醒）
    pthread_mutex_t lock; // 互斥锁（保证原子性）
} Zem_t;

// 初始化
void Zem_init(Zem_t *s, int value) {
    s->value = value;
    pthread_cond_init(&s->cond, NULL);
    pthread_mutex_init(&s->lock, NULL);
}

// 模拟sem_wait()
void Zem_wait(Zem_t *s) {
    pthread_mutex_lock(&s->lock);
    while (s->value <= 0) { // 循环等待（防虚假唤醒）
        pthread_cond_wait(&s->cond, &s->lock);
    }
    s->value--; // 原子减1
    pthread_mutex_unlock(&s->lock);
}

// 模拟sem_post()
void Zem_post(Zem_t *s) {
    pthread_mutex_lock(&s->lock);
    s->value++; // 原子加1
    pthread_cond_signal(&s->cond); // 唤醒一个阻塞线程
    pthread_mutex_unlock(&s->lock);
}
```
- 特点：值不会小于0（与Dijkstra原始定义不同），实现简单，符合Linux原生信号量行为。

## 五、关键使用规则与避坑指南
### 1. 初始化值选择
- 锁（互斥）：值=1（二进制信号量）；
- 排序（事件等待）：值=0；
- 资源限流：值=N（N为最大并发数/资源数）。

### 2. 避免死锁
- 控制信号量操作顺序：先等待资源（`sem_wait(&empty)`/`sem_wait(&full)`），再加锁（`sem_wait(&mutex)`）；
- 打破循环依赖：如哲学家问题中改变拿叉顺序；
- 最小化锁持有范围：锁仅包裹临界区，不跨`sem_wait()`阻塞操作。

### 3. 饥饿问题
- 读写锁中写者可能饥饿：可添加“写者优先”逻辑（如用额外信号量标记写者等待）；
- 信号量本身无公平性：需通过额外设计（如队列）保证线程公平获取。

### 4. 核心提示（Hill’s Law）
简单方案往往更优：读写锁等复杂结构虽灵活，但开销可能高于普通锁，优先尝试简单信号量用法（如二进制信号量=锁），再按需优化。

## 六、核心总结
1. 信号量是通用同步原语，通过初始化值适配锁、排序、限流等场景，功能覆盖锁和条件变量；
2. 核心操作`sem_wait()`（P）和`sem_post()`（V）需保证原子性，状态值隐含资源/等待线程数；
3. 经典问题解决方案：
   - 生产者/消费者：`empty`+`full`+`mutex`三信号量；
   - 读写锁：`lock`（保护计数）+`writelock`（写独占）；
   - 哲学家问题：打破拿叉顺序，避免死锁；
4. 关键风险：死锁（操作顺序）和饥饿（公平性），需通过合理设计规避。

## 七、作业方向（Code）
1. 实现信号量版`fork/join`，验证线程顺序同步；
2. 解决“会面问题”（两线程必须同时进入临界区）；
3. 实现屏障同步（所有线程完成`P1`后再执行`P2`）；
4. 优化读写锁，避免写者饥饿；
5. 用信号量实现无饥饿互斥锁。
