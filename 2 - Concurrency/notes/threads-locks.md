# threads-locks

## 核心结论
锁（Lock）的核心作用是**保证临界区（访问共享资源的代码段）互斥执行**，解决多线程并发导致的竞态条件问题。其实现依赖“硬件原子指令+操作系统支持”：硬件提供原子操作（如Test-And-Set、Compare-And-Swap）保证基础互斥，OS提供睡眠/唤醒机制（如park/unpark、futex）避免自旋浪费；演进逻辑是“先解决正确性，再优化公平性，最后降低性能开销”，最终形成兼顾效率、公平性和通用性的工业级实现（如Linux futex）。

## 一、锁的基本概念与语义
### 1. 核心定义
锁是一种同步原语，状态分为“可用（unlocked）”和“持有（locked）”：
- 可用状态：线程调用`lock()`可立即获取锁，进入临界区；
- 持有状态：其他线程调用`lock()`会阻塞/自旋，直到持有线程调用`unlock()`释放锁。

### 2. 核心语义
- 互斥性：同一时间仅一个线程能持有锁，进入临界区；
- 原子性：`lock()`到`unlock()`之间的临界区代码，执行效果等价于单条原子指令。

### 3. POSIX线程锁（Mutex）接口
```c
// 静态初始化
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
// 动态初始化
int pthread_mutex_init(pthread_mutex_t *lock, const pthread_mutexattr_t *attr);
// 加锁/解锁（封装后带错误检查）
Pthread_mutex_lock(&lock);   // 阻塞直到获取锁
Pthread_mutex_unlock(&lock); // 释放锁
```
- 粒度控制：粗粒度锁（一个锁保护所有共享资源）vs 细粒度锁（不同资源用不同锁），细粒度可提升并发度。

## 二、锁的评估标准
### 1. 正确性
- 核心要求：严格保证互斥，禁止多个线程同时进入临界区；
- 是锁的基础属性，所有实现的首要目标。

### 2. 公平性
- 定义：等待锁的线程是否按请求顺序获取锁，是否存在饥饿（线程永远无法获取锁）；
- 示例：简单自旋锁不公平，票锁（Ticket Lock）公平。

### 3. 性能
- 无竞争场景：单线程反复加解锁的开销（越低越好）；
- 单CPU竞争：多线程在同一CPU上争锁，避免自旋/上下文切换浪费；
- 多CPU竞争：线程分布在不同CPU，减少跨CPU缓存一致性开销。

## 三、锁的实现演进（从简单到复杂）
### 3.1 早期方案：禁用中断（Interrupt Disabling）
#### 原理
- 单CPU场景下，临界区前禁用中断，结束后启用，避免线程被中断切换，保证代码原子执行。
#### 代码逻辑
```c
void lock() { DisableInterrupts(); }
void unlock() { EnableInterrupts(); }
```
#### 优缺点
- 优点：实现极简，单CPU下正确；
- 缺点：
  1. 多CPU无效（其他CPU线程可同时进入临界区）；
  2. 滥用风险（恶意程序禁用中断后死循环，导致系统崩溃）；
  3. 丢失中断（长时间禁用可能错过磁盘I/O完成等关键事件）；
- 适用场景：仅OS内核内部访问私有数据结构（内核可信，无滥用风险）。

### 3.2 失败尝试：简单标志锁（Load/Store非原子）
#### 原理
- 用全局标志`flag`（0=可用，1=持有），线程先检查标志，无锁则设置为1。
#### 代码逻辑
```c
typedef struct __lock_t { int flag; } lock_t;
void lock(lock_t *m) { while (m->flag == 1); m->flag = 1; }
void unlock(lock_t *m) { m->flag = 0; }
```
#### 核心问题
- `flag`的“检查-设置”是非原子操作，多线程并发时会出现“同时检查到flag=0，同时设置为1”，导致互斥失效。

### 3.3 硬件原子指令：自旋锁（Spin Lock）
自旋锁的核心是用硬件提供的**原子指令**将“检查-设置”操作原子化，线程获取不到锁时会自旋（循环等待），适用于临界区短的场景。

#### 1. Test-And-Set（原子交换）
- 指令语义：原子返回内存地址的旧值，并设置为新值。
- 锁实现逻辑：
  ```c
  void lock(lock_t *m) {
    while (TestAndSet(&m->flag, 1) == 1); // 自旋直到旧值为0（锁可用）
  }
  void unlock(lock_t *m) { m->flag = 0; }
  ```
- 优缺点：
  - 优点：实现简单，多CPU有效；
  - 缺点：不公平（等待线程可能永远自旋），自旋浪费CPU（单CPU场景下尤为严重）。

#### 2. Compare-And-Swap（CAS，比较并交换）
- 指令语义：原子检查内存值是否等于预期值，是则更新为新值，返回原始值。
- 锁实现逻辑：
  ```c
  void lock(lock_t *m) {
    while (CompareAndSwap(&m->flag, 0, 1) == 1); // 预期0（可用）则设为1
  }
  ```
- 特点：功能比Test-And-Set更强，可用于实现锁-free数据结构；自旋锁场景下行为与Test-And-Set一致。

#### 3. Load-Linked/Store-Conditional（LL/SC，加载-链接/条件存储）
- 指令语义：
  - LL：加载内存值到寄存器，记录地址；
  - SC：仅当该地址未被其他线程修改时，原子更新值，返回成功（1）/失败（0）。
- 锁实现逻辑：
  ```c
  void lock(lock_t *m) {
    while (1) {
      while (LoadLinked(&m->flag) == 1); // 自旋直到flag=0
      if (StoreConditional(&m->flag, 1) == 1) return; // 成功则获取锁
    }
  }
  ```
- 优点：相比CAS，避免“ABA问题”（内存值先变回预期值再修改的误判）。

#### 4. Fetch-And-Add（原子自增）：票锁（Ticket Lock）
- 指令语义：原子自增内存值，返回旧值。
- 锁实现逻辑（公平锁）：
  ```c
  typedef struct __lock_t { int ticket; int turn; } lock_t;
  void lock(lock_t *m) {
    int myturn = FetchAndAdd(&m->ticket); // 原子取号
    while (m->turn != myturn); // 自旋直到轮到自己
  }
  void unlock(lock_t *m) { m->turn++; } // 下一个线程入场
  ```
- 核心优势：
  - 公平性：按取号顺序获取锁，无饥饿；
  - 缺点：仍存在自旋浪费（单CPU场景）。

### 3.4 解决自旋浪费：OS支持的锁优化
自旋锁的核心问题是“自旋浪费CPU”，需OS提供调度级支持，让等待线程放弃CPU而非自旋。

#### 1. 简单优化：Yield（放弃CPU）
- 原理：线程获取不到锁时，调用OS的`yield()`原语，主动从“运行态”转为“就绪态”，让其他线程运行。
- 代码逻辑：
  ```c
  void lock(lock_t *m) {
    while (TestAndSet(&m->flag, 1) == 1) yield(); // 自旋改为放弃CPU
  }
  ```
- 优点：单CPU场景下避免自旋浪费；
- 缺点：多线程竞争时，频繁yield导致上下文切换开销。

#### 2. 进阶方案：队列+Park/Unpark（睡眠/唤醒）
- 核心思想：用队列记录等待线程，获取不到锁的线程睡眠（park），释放锁时唤醒（unpark）队列首线程，完全避免自旋。
- 实现逻辑（Solaris为例）：
  ```c
  typedef struct __lock_t { int flag; int guard; queue_t *q; } lock_t;
  void lock(lock_t *m) {
    while (TestAndSet(&m->guard, 1) == 1); // 自旋获取守护锁（保护队列操作）
    if (m->flag == 0) {
      m->flag = 1; // 直接获取锁
    } else {
      queue_add(m->q, gettid()); // 加入等待队列
      park(); // 睡眠
    }
    m->guard = 0;
  }
  void unlock(lock_t *m) {
    while (TestAndSet(&m->guard, 1) == 1);
    if (!queue_empty(m->q)) {
      unpark(queue_remove(m->q)); // 唤醒下一个线程
    } else {
      m->flag = 0; // 无等待线程，释放锁
    }
    m->guard = 0;
  }
  ```
- 关键解决：
  - 守护锁（guard）：保护队列和flag的原子操作；
  - 避免唤醒丢失：用`setpark()`标记即将睡眠，若此时被唤醒，后续`park()`直接返回。

#### 3. 工业级实现：Linux Futex（快速用户态互斥锁）
- 核心设计：用户态+内核态结合，无竞争时纯用户态操作（低开销），有竞争时陷入内核睡眠。
- 关键逻辑：
  - 用一个整数同时标记“锁状态”（高bit=1表示持有）和“等待线程数”（低31bit）；
  - `futex_wait(address, expected)`：用户态检查锁状态，若符合预期则陷入内核睡眠；
  - `futex_wake(address)`：释放锁时，唤醒一个等待线程。
- 优点：兼顾无竞争时的低开销和有竞争时的低CPU浪费，是Linux pthread_mutex的底层实现。

### 3.5 混合优化：两阶段锁（Two-Phase Lock）
- 原理：第一阶段自旋一段时间（预期锁很快释放），若未获取则进入第二阶段（睡眠等待）。
- 适用场景：临界区时长不确定，平衡自旋的低延迟和睡眠的低浪费。

## 四、关键问题与解决方案
### 1. 优先级反转（Priority Inversion）
- 问题：高优先级线程等待低优先级线程持有的锁，而中优先级线程抢占低优先级线程，导致高优先级线程长期阻塞。
- 解决方案：
  - 避免自旋锁（单CPU场景）；
  - 优先级继承（临时提升低优先级线程优先级，使其尽快释放锁）；
  - 统一线程优先级。

### 2. 唤醒/等待竞争（Wakeup/Waiting Race）
- 问题：线程即将调用`park()`时，锁被释放并唤醒，但唤醒发生在`park()`前，导致线程永久睡眠。
- 解决方案：`setpark()`原语（标记即将睡眠，若已被唤醒则`park()`直接返回）。

## 五、锁的实现对比
| 锁类型               | 硬件依赖               | OS依赖 | 正确性 | 公平性 | 性能（无竞争） | 性能（有竞争） |
|----------------------|------------------------|--------|--------|--------|----------------|----------------|
| 禁用中断             | 中断控制指令           | 无     | 单CPU√ | -      | 极高           | 多CPU×         |
| 简单标志锁           | 无（load/store）       | 无     | ×      | -      | -              | -              |
| Test-And-Set自旋锁   | Test-And-Set           | 无     | √      | ×      | 高             | 单CPU差        |
| 票锁（Fetch-And-Add） | Fetch-And-Add          | 无     | √      | √      | 高             | 单CPU差        |
| Yield自旋锁          | Test-And-Set           | yield  | √      | ×      | 中             | 单CPU较好      |
| 队列+Park/Unpark     | Test-And-Set           | park/unpark | √ | √ | 中 | 好（无自旋） |
| Linux Futex          | 原子指令（CAS）        | futex  | √      | √      | 极高           | 好             |

## 六、核心总结
1. 锁的演进逻辑：从“单CPU有效”到“多CPU兼容”，从“自旋浪费”到“睡眠高效”，从“不公平”到“公平可控”；
2. 核心依赖：硬件原子指令（保证基础互斥）+ OS调度支持（优化性能和公平性）；
3. 关键权衡：
   - 无竞争开销 vs 有竞争效率；
   - 实现复杂度 vs 通用性；
4. 工业级选择：Linux Futex和两阶段锁成为主流，兼顾低延迟和低浪费。

## 七、作业方向（Simulation）
通过`x86.py`模拟器验证锁的行为：
1. 分析简单标志锁的竞态条件，观察中断频率对结果的影响；
2. 验证Test-And-Set、CAS、票锁的正确性，对比自旋开销；
3. 测试`yield.s`在自旋场景下的CPU节省效果；
4. 分析`test-and-test-and-set.s`的优化逻辑（先检查再原子操作，减少缓存一致性开销）。
