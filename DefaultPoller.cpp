#include "Poller.h"
#include "EPollPoller.h"

Poller* Poller::newDefaultPoller(EventLoop *loop)
{
    //从当前进程的环境变量中获取指定名字的变量值。
    if(::getenv("MUDUO_USE_POLL"))
    {
        // 如果环境变量存在 → 使用 poll 实现
        //return new PollPoller(loop);
        return nullptr;
    }
    else
    {
        // 默认使用 epoll 实现（更高效）
        return new EPollPoller(loop);
    }
}