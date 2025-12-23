# threads-bugs

## 核心结论
并发编程的bug主要分为**非死锁bug**和**死锁bug**两类，其中非死锁bug（占比~70%）更常见，核心是原子性违反或顺序违反，修复关键是保证临界区原子性或强制线程执行顺序；死锁bug需满足四大必要条件，最实用的解决策略是**预防**（尤其是锁排序），避免、检测恢复等策略仅适用于特定场景。理解两类bug的模式和成因，是编写健壮并发代码的核心。

## 一、核心问题（CRUX）
并发程序的bug具有隐蔽性、偶发性（依赖线程调度），难以复现和调试。关键是识别常见bug模式（原子性违反、顺序违反、死锁），掌握对应的预防和修复方法，从根源减少bug。

## 二、非死锁bug（Non-Deadlock Bugs）
非死锁bug占实际并发bug的70%以上（Lu等人对MySQL、Apache、Mozilla、OpenOffice的研究显示，105个并发bug中74个为非死锁bug），核心分为两类，占非死锁bug的97%。

### 2.1 原子性违反（Atomicity-Violation Bugs）
#### 定义
一段预期为原子执行的代码（多步内存访问），因未加同步机制，被线程调度打断，导致执行结果不符合预期。

#### 典型示例（MySQL中的真实bug）
```c
// 线程1：检查proc_info非空后打印
if (thd->proc_info) {
    fputs(thd->proc_info, ...); // 可能 dereference NULL
}

// 线程2：将proc_info设为NULL
thd->proc_info = NULL;
```
- 问题：线程1执行“检查→打印”的间隙，线程2可能修改`proc_info`为NULL，导致线程1崩溃。
- 本质：“检查+使用”的复合操作未保证原子性。

#### 修复方法
给共享资源访问添加锁，确保原子性：
```c
pthread_mutex_t proc_info_lock = PTHREAD_MUTEX_INITIALIZER;

// 线程1
pthread_mutex_lock(&proc_info_lock);
if (thd->proc_info) {
    fputs(thd->proc_info, ...);
}
pthread_mutex_unlock(&proc_info_lock);

// 线程2
pthread_mutex_lock(&proc_info_lock);
thd->proc_info = NULL;
pthread_mutex_unlock(&proc_info_lock);
```

### 2.2 顺序违反（Order-Violation Bugs）
#### 定义
两个（组）内存访问的预期执行顺序被颠倒（A应在B前执行，但未强制），导致依赖A的B执行失败。

#### 典型示例（线程初始化顺序问题）
```c
// 线程1：初始化mThread
void init() {
    mThread = PR_CreateThread(mMain, ...); // A：创建线程
}

// 线程2：直接访问mThread
void mMain(...) {
    mState = mThread->State; // B：依赖A完成，可能访问NULL
}
```
- 问题：线程2的`mMain`可能在`mThread`初始化前执行，导致空指针解引用。
- 本质：未强制“初始化（A）→访问（B）”的顺序。

#### 修复方法
用条件变量+锁强制顺序：
```c
pthread_mutex_t mtLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t mtCond = PTHREAD_COND_INITIALIZER;
int mtInit = 0; // 状态标记：0=未初始化，1=已初始化

// 线程1（初始化）
void init() {
    mThread = PR_CreateThread(mMain, ...);
    pthread_mutex_lock(&mtLock);
    mtInit = 1; // 标记初始化完成
    pthread_cond_signal(&mtCond); // 唤醒等待线程
    pthread_mutex_unlock(&mtLock);
}

// 线程2（访问）
void mMain(...) {
    pthread_mutex_lock(&mtLock);
    while (mtInit == 0) { // 循环等待（防虚假唤醒）
        pthread_cond_wait(&mtCond, &mtLock);
    }
    pthread_mutex_unlock(&mtLock);
    mState = mThread->State; // 安全访问
}
```

### 2.3 非死锁bug总结
- 核心模式：要么“复合操作未原子化”，要么“依赖顺序未强制”；
- 修复原则：原子性问题用锁/原子指令，顺序问题用条件变量/信号量；
- 特点：bug触发依赖线程调度，复现难度大，需提前通过代码审查规避。

## 三、死锁bug（Deadlock Bugs）
#### 定义
多个线程互相持有对方需要的资源（如锁），且均不释放已持有资源，导致所有线程永久阻塞，程序停滞。

### 3.1 死锁的四大必要条件（缺一不可）
1. **互斥（Mutual Exclusion）**：资源（锁）只能被一个线程独占；
2. **持有并等待（Hold-and-Wait）**：线程持有部分资源，同时等待其他线程的资源；
3. **不可抢占（No Preemption）**：资源不能被强制从持有线程夺走；
4. **循环等待（Circular Wait）**：存在线程链，每个线程等待下一个线程的资源。

### 3.2 典型示例（双锁循环等待）
```c
// 线程1：先拿L1，再拿L2
pthread_mutex_lock(L1);
pthread_mutex_lock(L2); // 若L2被线程2持有，阻塞

// 线程2：先拿L2，再拿L1
pthread_mutex_lock(L2);
pthread_mutex_lock(L1); // 若L1被线程1持有，阻塞
```
- 触发场景：线程1拿到L1后被调度切换，线程2拿到L2，双方互相等待，形成死锁。

### 3.3 死锁的成因
1. **复杂依赖**：大型系统中组件间依赖嵌套（如虚拟内存系统需访问文件系统，文件系统需访问内存）；
2. **封装隐藏**：模块化设计隐藏了锁的获取顺序（如Java Vector的`AddAll()`方法，内部获取两个Vector的锁，调用方无法感知）。

### 3.4 死锁的解决方案
#### 策略1：预防（Prevention）—— 最实用
通过破坏四大必要条件之一，从根源避免死锁，优先选择破坏“循环等待”。

##### 1. 破坏循环等待：强制锁排序
- 核心逻辑：所有线程按固定顺序获取锁（全局/部分排序），避免循环依赖；
- 实操技巧：
  - 简单场景：约定“L1先于L2”“小ID锁先于大ID锁”；
  - 通用场景：按锁的内存地址排序（高地址→低地址或反之），适用于函数参数传入不确定顺序的锁：
    ```c
    void do_something(pthread_mutex_t *m1, pthread_mutex_t *m2) {
        if (m1 > m2) { // 按地址高低排序
            pthread_mutex_lock(m1);
            pthread_mutex_lock(m2);
        } else {
            pthread_mutex_lock(m2);
            pthread_mutex_lock(m1);
        }
        // 临界区
        pthread_mutex_unlock(m1);
        pthread_mutex_unlock(m2);
    }
    ```
- 实例：Linux内核内存映射代码中，明确规定10组锁的获取顺序（如“i_mutex先于i_mmap_rwsem”）。

##### 2. 破坏持有并等待：原子获取所有锁
- 核心逻辑：线程在执行前，一次性获取所有需要的锁，避免“持有部分+等待其他”；
- 实现示例：
  ```c
  pthread_mutex_t global_lock; // 全局锁保护锁获取过程
  pthread_mutex_lock(&global_lock);
  pthread_mutex_lock(L1); // 原子获取所有需要的锁
  pthread_mutex_lock(L2);
  pthread_mutex_unlock(&global_lock);
  ```
- 缺点：破坏封装（需提前知道所有锁）、降低并发（锁持有时间过长）。

##### 3. 破坏不可抢占：使用`trylock`退避
- 核心逻辑：用`pthread_mutex_trylock()`尝试获取锁，失败则释放已持有锁，稍后重试；
- 实现示例：
  ```c
  top:
  pthread_mutex_lock(L1);
  if (pthread_mutex_trylock(L2) != 0) { // 尝试获取L2，失败则退避
      pthread_mutex_unlock(L1);
      usleep(rand() % 100); // 随机延迟防活锁
      goto top;
  }
  // 临界区
  pthread_mutex_unlock(L2);
  pthread_mutex_unlock(L1);
  ```
- 注意：可能引发“活锁”（线程反复退避重试，无进展），需添加随机延迟。

##### 4. 破坏互斥：无锁数据结构
- 核心逻辑：用硬件原子指令（如CAS、LL/SC）构建无锁数据结构，避免显式锁；
- 示例（原子自增）：
  ```c
  void AtomicIncrement(int *value, int amount) {
      do {
          int old = *value;
      } while (CompareAndSwap(value, old, old + amount) == 0);
  }
  ```
- 缺点：实现复杂（需处理重试、ABA问题），仅适用于简单数据结构。

#### 策略2：避免（Avoidance）—— 适用场景有限
- 核心逻辑：基于全局资源需求（线程需获取的锁），通过调度避免死锁（如Dijkstra的银行家算法）；
- 示例：已知线程T1、T2均需L1和L2，调度时让两者在同一CPU串行执行，避免并发竞争；
- 缺点：需提前知晓所有线程的资源需求，限制并发，不适用于通用系统。

#### 策略3：检测与恢复（Detection and Recovery）—— 兜底方案
- 核心逻辑：允许死锁偶尔发生，定期检测（如构建资源依赖图，检查循环），发现后恢复；
- 恢复方式：
  - 简单场景：重启程序（适用于死锁罕见的系统）；
  - 复杂场景：数据库系统中，终止一个线程释放资源，修复数据一致性；
- 适用场景：死锁概率低、恢复成本低的系统（如数据库）。

### 3.5 死锁总结
- 实用优先级：**锁排序预防 > trylock退避 > 检测恢复 > 无锁结构**；
- 关键提示：死锁预防无需复杂逻辑，仅需在代码设计阶段约定锁顺序，是工业界最常用的方案。

## 四、关键设计原则与提示
### 1. Tom West定律
“不是所有值得做的事都值得做好”——若死锁发生概率极低（如一年一次），且重启成本低，无需投入大量精力做完美预防，优先保证代码简洁。

### 2. 非死锁bug优先规避
- 原子性：复合操作（检查+修改、读取+使用）必须加锁；
- 顺序：线程间有依赖关系时，用条件变量/信号量强制顺序，避免“先访问后初始化”。

### 3. 锁的使用技巧
- 最小持有时间：锁仅包裹临界区，不跨I/O、睡眠等耗时操作；
- 显式锁顺序：大型项目中，文档化锁的获取顺序，避免团队成员误用。

## 五、核心总结
1. 并发bug的核心是“同步缺失”：非死锁bug缺失原子性/顺序控制，死锁bug缺失锁顺序约定；
2. 非死锁bug修复：原子性用锁/原子指令，顺序用条件变量；
3. 死锁bug修复：优先用“锁排序”预防，简单高效，适配大多数场景；
4. 进阶方向：无锁数据结构可避免死锁，但实现复杂，仅适用于性能敏感场景；
5. 调试建议：并发bug难以复现，需通过代码审查（检查原子性、锁顺序）提前规避，而非依赖测试。

## 六、作业方向（Code）
1. 分析`vector-deadlock.c`的死锁触发条件，观察线程数、循环次数对死锁概率的影响；
2. 验证`vector-global-order.c`的锁排序方案，理解为何能避免死锁；
3. 对比`vector-try-wait.c`（trylock退避）与`vector-global-order.c`的性能差异；
4. 分析`vector-nolock.c`的无锁实现，验证其语义是否与有锁版本一致，评估性能优势。
