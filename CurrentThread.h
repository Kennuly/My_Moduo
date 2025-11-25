#pragma once
/*
CurrentThread 是 Muduo 为了性能、线程安全和统一接口而设计的线程 ID 缓存工具，是高性能网络库必备组件。
    Muduo 这种高性能网络框架每一个日志、每一个事件循环都要检查 tid，如果每次都系统调用，会非常慢。
    
    每一个线程都有自己独立的tid，第一次回去后将其缓存起来，后续直接读取缓存中的变量，不必在进行系统调用
*/


//__thread 是 GCC 和 Clang 提供的 线程局部存储（Thread Local Storage, TLS）关键字，用来声明一个 每个线程都有独立副本 的变量。
namespace CurrentThread
{
    extern __thread int t_cachedTid;
    void cachedTid();
    //内联函数就是让编译器在调用处“直接把函数内容展开”，而不是进行函数调用
    inline int tid()
    {
        //还没获取过当前线程的tid
        //__builtin_expect 是 GCC / Clang 提供的一个 分支预测优化 内置函数，用来告诉编译器：某个条件更可能成立，从而让 CPU 执行更快
        if(__builtin_expect(t_cachedTid == 0, 0))
        {
            cachedTid();
        }
        return t_cachedTid;
    } 
};