#ifndef __common_h__
#define __common_h__
// 防止头文件被重复包含的宏定义：如果未定义__common_h__，则定义它并包含后续内容
// 避免同一头文件在编译单元中多次引入导致的重定义错误

// 引入系统头文件：
// sys/time.h：提供时间相关的结构体（如struct timeval）和函数（如gettimeofday）
// sys/stat.h：提供文件状态相关的定义（可能用于依赖此头文件的其他代码）
// assert.h：提供断言宏assert，用于调试时检查条件是否成立
#include <sys/time.h>
#include <sys/stat.h>
#include <assert.h>

// 获取当前时间，返回以秒为单位的浮点数（包含微秒精度）
// 返回值：当前时间（秒），整数部分为秒，小数部分为微秒（1微秒=1e-6秒）
double GetTime()
{
    struct timeval t; // 用于存储时间的结构体，包含秒(tv_sec)和微秒(tv_usec)
    // 调用gettimeofday获取当前时间，第二个参数为NULL表示不获取时区信息
    int rc = gettimeofday(&t, NULL);
    assert(rc == 0); // 断言系统调用成功（rc为0），失败则终止程序并提示
    // 转换为总秒数：秒数 + 微秒数/1e6（将微秒转换为秒）
    return (double)t.tv_sec + (double)t.tv_usec / 1e6;
}

// 自旋等待指定的时间（忙等待，不释放CPU）
// 参数howlong：需要等待的时间（秒）
void Spin(int howlong)
{
    double t = GetTime(); // 记录开始等待的时间
    // 循环检查当前时间与开始时间的差值，直到达到指定的等待时长
    // 循环体为空（;），仅通过时间判断控制等待时长
    while ((GetTime() - t) < (double)howlong)
        ; // 空循环，持续占用CPU直到等待结束
}

#endif // __common_h__
       // 结束头文件保护宏