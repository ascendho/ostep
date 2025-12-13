// 对应章节：vm-api.pdf p3
// To compile: gcc -o sizeof_ptr_vs_array sizeof_ptr_vs_array.c
// To run:     ./sizeof_ptr_vs_array

#include <stdio.h>
#include <stdlib.h>

int main() {
    // 动态分配 10 个 int 类型元素的内存空间，返回指向该空间的指针 x
    // 此时 x 是一个 int 类型的指针变量
    int *x = malloc(10 * sizeof(int));
    // 打印指针变量 x 的大小（与系统位数相关，32 位系统通常为 4 字节，64 位系统通常为 8 字节）
    // 注意：此处 sizeof 计算的是指针本身的大小，而非所指向内存块的大小
    printf("%d\n", sizeof(x));

    // 定义一个包含 10 个 int 类型元素的数组 y
    // 数组定义时已确定大小（10 个int），编译器会记录其长度信息
    int y[10];
    // 打印数组 y 的总大小（数组元素个数 × 单个元素大小，int 通常为 4 字节，故 10 × 4 = 40 字节）
    // 注意：此处 sizeof 计算的是整个数组占用的内存大小
    printf("%d\n", sizeof(y));
    return 0;

    // 指针需通过额外手段（如手动记录）知晓指向内容的长度，
    // 而数组的长度可通过sizeof间接获取（sizeof(y)/sizeof(y[0])）
    
    // 注：数组名并非等价于指针，只是在特定场景下会 “隐式退化” 为指向数组首元素的指针
    // 1. 数组名作为函数参数传递时
    // 2. 数组名参与算术运算 / 取值时
    // 3. 赋值给指针变量时
}