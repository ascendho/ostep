# threads-cv

## 核心结论
条件变量（CV）是多线程同步的核心原语，与锁配合使用，解决“线程等待特定条件成立”的问题——它让等待线程休眠而非自旋，避免CPU浪费，同时通过信号机制确保条件满足时线程被唤醒。核心使用规则：**必须与锁绑定、用while循环检查条件、按场景选择signal/broadcast**，经典应用是解决生产者/消费者问题，核心价值是实现“高效等待-唤醒”机制，避免忙等。

## 一、核心问题（CRUX）
多线程中，线程常需等待某个条件成立（如子线程完成、缓冲区非空）才能继续执行。简单自旋等待（`while(condition==false);`）会浪费大量CPU资源，甚至因竞态条件导致逻辑错误。如何设计一种机制，让线程高效等待条件，且条件成立时能及时被唤醒？

## 二、条件变量的定义与核心接口
### 1. 本质
条件变量是一个显式队列，线程可在条件不满足时将自己放入队列休眠（`wait`），其他线程修改条件后可唤醒队列中的线程（`signal`/`broadcast`）。

### 2. POSIX核心接口
```c
// 声明条件变量
pthread_cond_t cond;
// 初始化（静态）
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
// 等待：必须持有锁，原子释放锁并休眠，唤醒后重新获取锁
pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
// 唤醒：唤醒一个等待该条件的线程
pthread_cond_signal(pthread_cond_t *cond);
// 广播：唤醒所有等待该条件的线程
pthread_cond_broadcast(pthread_cond_t *cond);
```

### 3. 关键特性
- 必须与锁配合：`wait`调用时需持有锁，否则行为未定义；
- 原子操作：`wait`会原子完成“释放锁+休眠”，避免竞态条件；
- 唤醒后重获锁：线程从`wait`返回前，会重新获取锁，保证条件检查的原子性；
- 依赖状态变量：条件变量本身不存储状态，需配合全局/共享状态变量（如`done`、`count`）判断条件是否成立。

## 三、基础应用：线程join（父线程等待子线程）
### 1. 错误方案1：自旋等待
```c
volatile int done = 0;
void *child(void *arg) { done = 1; return NULL; }
int main() {
    pthread_create(&c, NULL, child, NULL);
    while (done == 0); // 自旋浪费CPU
    return 0;
}
```
- 问题：单CPU场景下，子线程可能无法获得CPU，父线程无限自旋；多CPU场景下浪费资源。

### 2. 错误方案2：无状态变量的条件变量
```c
pthread_cond_t c;
pthread_mutex_t m;
void thr_exit() { Pthread_cond_signal(&c); } // 无状态标记
void thr_join() { Pthread_cond_wait(&c, &m); }
```
- 问题：若子线程先执行并`signal`，父线程后续`wait`会永久休眠（信号丢失）。

### 3. 正确方案：条件变量+状态变量+锁
```c
int done = 0;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t c = PTHREAD_COND_INITIALIZER;

void thr_exit() {
    Pthread_mutex_lock(&m);
    done = 1; // 标记状态
    Pthread_cond_signal(&c); // 唤醒等待线程
    Pthread_mutex_unlock(&m);
}

void thr_join() {
    Pthread_mutex_lock(&m);
    while (done == 0) { // 循环检查条件
        Pthread_cond_wait(&c, &m); // 释放锁并休眠
    }
    Pthread_mutex_unlock(&m);
}
```
- 核心逻辑：
  1. 状态变量`done`记录子线程是否完成；
  2. 父线程`thr_join`中，持有锁循环检查`done`，不满足则`wait`；
  3. 子线程`thr_exit`中，持有锁修改`done`并`signal`，确保状态修改与信号发送原子性。

## 四、经典应用：生产者/消费者（有界缓冲区）问题
### 1. 问题定义
- 生产者线程：生成数据并放入缓冲区；
- 消费者线程：从缓冲区取出数据并消费；
- 约束：缓冲区满时生产者不能放入，缓冲区空时消费者不能取出。

### 2. 错误演进与正确方案
#### 错误方案1：单条件变量+if判断
```c
pthread_cond_t cond;
pthread_mutex_t mutex;
int count = 0; // 缓冲区数据量（0=空，1=满）

void *producer(void *arg) {
    Pthread_mutex_lock(&mutex);
    if (count == 1) Pthread_cond_wait(&cond, &mutex); // if判断
    put(i); count = 1;
    Pthread_cond_signal(&cond);
    Pthread_mutex_unlock(&mutex);
}

void *consumer(void *arg) {
    Pthread_mutex_lock(&mutex);
    if (count == 0) Pthread_cond_wait(&cond, &mutex); // if判断
    get(); count = 0;
    Pthread_cond_signal(&cond);
    Pthread_mutex_unlock(&mutex);
}
```
- 问题：多消费者场景下，生产者唤醒一个消费者后，另一个消费者可能“插队”消费，导致被唤醒的消费者无数据可取（断言失败）。原因是`if`仅检查一次条件，未处理“唤醒后条件已变化”的情况。

#### 错误方案2：单条件变量+while判断
```c
// 仅将if改为while，其余同方案1
while (count == 1) Pthread_cond_wait(&cond, &mutex); // 生产者
while (count == 0) Pthread_cond_wait(&cond, &mutex); // 消费者
```
- 问题：多生产者/多消费者场景下，信号可能唤醒错误类型的线程。例如，消费者消费后`signal`，可能唤醒另一个消费者（而非生产者），导致所有线程休眠（死锁）。

#### 正确方案：双条件变量+while判断
```c
// 两个条件变量：分别对应“缓冲区空”和“缓冲区满”
pthread_cond_t empty; // 生产者等待：缓冲区空
pthread_cond_t fill;  // 消费者等待：缓冲区满
pthread_mutex_t mutex;
int count = 0;
#define MAX 10 // 缓冲区最大容量

// 生产者：等待empty，唤醒fill
void *producer(void *arg) {
    for (int i = 0; i < loops; i++) {
        Pthread_mutex_lock(&mutex);
        while (count == MAX) { // 缓冲区满，等待空
            Pthread_cond_wait(&empty, &mutex);
        }
        put(i); count++; // 放入数据
        Pthread_cond_signal(&fill); // 唤醒消费者
        Pthread_mutex_unlock(&mutex);
    }
}

// 消费者：等待fill，唤醒empty
void *consumer(void *arg) {
    for (int i = 0; i < loops; i++) {
        Pthread_mutex_lock(&mutex);
        while (count == 0) { // 缓冲区空，等待满
            Pthread_cond_wait(&fill, &mutex);
        }
        get(); count--; // 取出数据
        Pthread_cond_signal(&empty); // 唤醒生产者
        Pthread_mutex_unlock(&mutex);
    }
}
```
- 核心优化：
  1. 双条件变量分离“生产者等待”和“消费者等待”，避免唤醒错误类型线程；
  2. `while`循环确保线程唤醒后重新检查条件，处理“条件已变化”或“虚假唤醒”。

### 3. 缓冲区扩展：多槽位实现
将单槽位缓冲区扩展为循环队列，支持多个数据存储，核心修改`put`/`get`：
```c
int buffer[MAX];
int fill_ptr = 0; // 写入指针
int use_ptr = 0;  // 读取指针
int count = 0;    // 数据个数

void put(int value) {
    buffer[fill_ptr] = value;
    fill_ptr = (fill_ptr + 1) % MAX; // 循环写入
    count++;
}

int get() {
    int tmp = buffer[use_ptr];
    use_ptr = (use_ptr + 1) % MAX; // 循环读取
    count--;
    return tmp;
}
```
- 优势：减少生产者/消费者的唤醒频率，降低上下文切换开销，提升并发效率。

## 五、特殊场景：覆盖条件（Covering Conditions）
### 1. 问题定义
当等待线程的条件存在“优先级”差异时，`signal`可能唤醒不满足条件的线程，导致需要的线程无法被唤醒。例如：
- 线程A请求100字节内存，线程B请求10字节内存，均因内存不足等待；
- 其他线程释放50字节，`signal`可能唤醒线程A（仍不满足），线程B（满足）仍休眠。

### 2. 解决方案：`broadcast`唤醒所有等待线程
```c
void free(void *ptr, int size) {
    Pthread_mutex_lock(&m);
    bytesLeft += size;
    Pthread_cond_broadcast(&c); // 唤醒所有等待线程
    Pthread_mutex_unlock(&m);
}
```
- 逻辑：所有等待线程被唤醒后，重新检查条件（`while (bytesLeft < size)`），仅满足条件的线程继续执行，其余重新休眠；
- 权衡：可能唤醒不必要的线程，带来轻微性能开销，但确保正确性，适用于等待条件存在差异的场景（如内存分配、资源申请）。

## 六、关键使用规则与避坑指南
### 1. 必须与锁绑定
- `wait`调用前必须持有锁，否则会导致竞态条件（如线程检查条件后、`wait`前被中断，条件被修改但未收到信号）；
- `signal`/`broadcast`时建议持有锁，确保状态修改与信号发送原子性。

### 2. 用while而非if检查条件
- 原因1：Mesa语义（主流系统采用）中，线程被唤醒后条件可能已变化（如其他线程抢先修改）；
- 原因2：存在虚假唤醒（线程库实现导致无信号却唤醒），需重新验证条件。

### 3. 合理选择signal与broadcast
- `signal`：唤醒一个等待线程，效率高，适用于“条件满足后仅一个线程能继续”的场景（如生产者/消费者）；
- `broadcast`：唤醒所有等待线程，适用于“条件满足后多个线程可能继续”或“等待条件有差异”的场景（如内存分配）。

### 4. 状态变量与条件变量配套
- 条件变量本身不存储状态，必须通过共享变量（如`done`、`count`）记录条件，避免信号丢失或误唤醒。

## 七、核心总结
1. 条件变量的核心价值：替代自旋等待，让线程休眠以节省CPU，同时通过信号机制确保及时唤醒；
2. 三大核心要素：锁（保证原子性）、状态变量（记录条件）、条件变量（实现等待-唤醒）；
3. 经典应用：生产者/消费者问题（双条件变量+while循环是标准解法）；
4. 关键权衡：`signal`高效但需精准匹配唤醒对象，`broadcast`通用但有轻微性能开销；
5. 避坑关键：永远用while检查条件、与锁绑定使用、不依赖条件变量存储状态。

## 八、作业方向（Code）
1. 运行`main-two-cvs-while.c`（正确方案），测试不同缓冲区大小、生产者/消费者数量对性能的影响；
2. 验证`main-one-cv-while.c`（单条件变量）在多消费者场景下的死锁问题；
3. 测试`main-two-cvs-if.c`（if判断）在多线程场景下的条件失效问题；
4. 分析`main-two-cvs-while-extra-unlock.c`中“提前释放锁”导致的竞态条件；
5. 通过调整线程休眠位置，观察缓冲区大小对程序总执行时间的影响。
