#include "EventLoop.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/eventfd.h>
#include "Logger.h"
#include <errno.h>
#include "Poller.h"
#include "Channel.h"

// 防止一个线程创建多个EventLoop   （thread_local）
__thread EventLoop *t_loopInThisThread = nullptr;

// 定义默认的Poller IO复用接口的超时时间(10s)
const int kPollTimeMs = 10000;

// 创建wakeupfd，用来唤醒subReactor处理新来的channel
int createEventfd()
{
    /*
        如果只需要跨线程唤醒 EventLoop → 用 eventfd 更合适。
        如果需要双向通信或传 fd → 用 socketpair。
    */
    // eventfd() 是 Linux 专门为 事件通知 设计的系统调用，它比 socketpair 更轻量、开销更低，非常适合在 多线程 Reactor/EventLoop 中进行线程唤醒。
    // eventfd() 创建一个 事件计数器 (event counter)，并返回一个 文件描述符 (fd)。
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
    {
        LOG_FATAL("eventfd error:%d\n", errno);
    }
    return evtfd;
}

EventLoop::EventLoop() : looping_(false),
                         quit_(false),
                         threadId_(CurrentThread::tid()),
                         poller_(Poller::newDefaultPoller(this)),
                         wakeupFd_(createEventfd()),
                         wakeupChannel_(new Channel(this, wakeupFd_)),
                         // currentActiveChannel_(nullptr),
                         callingPendingFunctors_(false)
{
    LOG_DEBUG("EventLoop created %p in thread %d\n", this, threadId_);
    if (t_loopInThisThread)
    {
        LOG_FATAL("Another EventLoop %p exists in thread %d\n", t_loopInThisThread, threadId_);
    }
    else
    {
        t_loopInThisThread = this;
    }
    // 设置wakeupFd的事件类型以及发生事件后的回调操作
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    // 每一个EventLoop都将监听wakeupchannel的EPOLLIN读事件了
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

// 开启事件循环
void EventLoop::loop()
{
    looping_ = true;
    quit_ = false;

    LOG_INFO("EventLoop %p start looping\n", this);
    while (!quit_)
    {
        activeChannels_.clear();

        // 监听两类fd    一种是client的fd，一种是wakeupfd
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        for (Channel *channel : activeChannels_)
        {
            // Poller监听哪些channel发生事件了，然后上报给EventLoop，通知channel处理相应的事件
            channel->handleEvent(pollReturnTime_);
        }
        // 执行当前EventLoop事件循环需要处理的回调操作
        /*
            IO线程  mainLoop accept fd  <==  channel subloop
            mainLoop 事先注册一个回调cb（需要subloop来执行）    wakeup subloop后，执行下面的方法，执行之前mainloop注册的回调
        */
        doPendingFunctors();
    }
    LOG_INFO("EVentLoop %p stop looping.\n", this);
    looping_ = false;
}
// 退出事件循环 1.loop在自己的线程中调用quit    2.在一个非loop的线程中，调用loop的quit
void EventLoop::quit()
{
    quit_ = true;
    // 如果在其他线程中，调用的quit  在一个subloop中，调用了mainloop的quit，
    if (!isInLoopThread())
    {
        wakeup();
    }
}

// 在当前loop中执行
void EventLoop::runInloop(Functor cb)
{
    if (isInLoopThread()) // 在当前的线程中，执行cb
    {
        cb();
    }
    else // 在非当前loop线程中执行cb，就需要唤醒loop所在线程，执行cb
    {
        queueInloop(cb);
    }
}
// 把cb放入队列中，唤醒loop所在的线程，执行cb
void EventLoop::queueInloop(Functor cb)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }
    // 唤醒相应的，需要执行上面回调操作的loop的线程
    // 或上callingPendingFunctors_的意思是：当前loop正在执行回调，但是loop  又有了新的回调
    if (!isInLoopThread() || callingPendingFunctors_) 
    {
        wakeup(); // 唤醒loop所在线程
    }
}

// 唤醒loop所在的线程的     向wakeupfd_写一个数据，wakeupChannel就发生读事件，当前loop就会被唤醒
void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof one);
    if(n != sizeof one)
    {
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8\n", n);
    }
}

// Eventloop的方法 ==> Poller的方法
void EventLoop::updateChannel(Channel *channel)
{
    poller_->updateChannel(channel);
}
void EventLoop::removeChannel(Channel *channel)
{
    poller_->removeChannel(channel);
}
bool EventLoop::hasChannel(Channel *channel)
{
    return poller_->hasChannel(channel);
}

// 执行回调
void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functions;
    callingPendingFunctors_ = true;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        functions.swap(pendingFunctors_);
    }
    for(const Functor &functor : functions)
    {
        functor();  //执行当前loop需要执行的回调操作
    }
    callingPendingFunctors_ = false;
}

void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_, &one, sizeof one);
    if (n != sizeof(one))
    {
        LOG_ERROR("EVentLoop::handleRead() reads %ld bytes instead of 8", n);
    }
}