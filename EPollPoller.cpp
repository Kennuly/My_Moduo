#include "EPollPoller.h"
#include "Logger.h"
#include <errno.h>
#include <unistd.h>
#include "Channel.h"
#include <string.h>

// channel未添加到Poller中
const int kNew = -1;        //Channel的成员index_ = -1
// channel已添加到Poller中
const int kAdded = 1;
// channel从Poller中删除
const int kDeleted = 2;

EPollPoller::EPollPoller(EventLoop *loop) : Poller(loop),
                                            // EPOLL_CLOEXEC：在 exec 系列函数执行新程序时自动关闭 epoll fd，防止文件描述符泄漏。
                                            epollfd_(::epoll_create1(EPOLL_CLOEXEC)), 
                                            // 会创建一个 vector，大小为 kInitEventList，每个元素使用 默认构造函数 初始化
                                            events_(kInitEventList)                   
{
    if (epollfd_ < 0)
    {
        LOG_FATAL("epoll_create error:%d\n", errno);
    }
}
EPollPoller::~EPollPoller()
{
    ::close(epollfd_);
}
Timestamp EPollPoller::poll(int timeoutMs, ChannelList *activeChannels)
{
    //实际上应该LOG_DEBUG输出日志更为合理
    LOG_INFO("func=%s => fd total count:%d\n", __FUNCTION__, channels_.size());
    //LOG_DEBUG("func=%s => fd=%d events=%d index=%d \n", __FUNCTION__, channel->fd(), channel->events(), idx);

    int numEvents = ::epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeoutMs);
    int saveErrno = errno;
    Timestamp now(Timestamp::now());
    if(numEvents > 0)
    {
        LOG_INFO("%d events happened\n", numEvents);
        fillActiveChannels(numEvents, activeChannels);
        if(numEvents == events_.size())
        {
            events_.resize(2 * events_.size());
        }
    }
    else if(numEvents == 0)
    {
        LOG_DEBUG("%s timeout!\n", __FUNCTION__);
    }
    else    //外部中断
    {
        if(saveErrno != EINTR)
        {
            errno = saveErrno;
            LOG_ERROR("EPOLLPoller::poll() error");
        }
    }
    return now;
}

//填写活跃的连接
void EPollPoller::fillActiveChannels(int numEvent, ChannelList *activeChannels) const
{
    for(int i = 0; i < numEvent; ++i)
    {
        Channel *channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel); // EventLoop就拿到了他的poller给他返回的所有事件的channel列表了
    }
}

// channel update remove => EventLoop updateChannel removeChannel => Poller
/*
                EventLoop => poller.poll
     ChannelList         Poller
                        ChannelMap <fd, channel*>


*/
void EPollPoller::updateChannel(Channel *channel) 
{
    const int idx = channel->index();
    LOG_INFO("func=%s => fd=%d events=%d index=%d \n", __FUNCTION__, channel->fd(), channel->events(), idx);
    if(idx == kNew || idx == kDeleted)
    {
        if(idx == kNew)
        {
            int fd = channel->fd();
            channels_[fd] = channel;
        }
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    else    //channel已经在poller上注册过了
    {
        int fd = channel->fd();
        if(channel->isNodeEvent())
        {
            update(EPOLL_CTL_DEL, channel);
        }
        else
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }
    
}

//从Poller中删除channel
void EPollPoller::removeChannel(Channel *channel) 
{
    int fd = channel->fd();
    int idx = channel->index();
    channels_.erase(fd);
    LOG_INFO("func=%s => fd=%d\n", __FUNCTION__, channel->fd());
    if(idx == kAdded)
    {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}



//更新channel通道 epoll_ctl add/mod/del具体操作
void EPollPoller::update(int operation, Channel *channel)
{
    epoll_event event;
    bzero(&event, sizeof(event));
    event.events = channel->events();
    event.data.fd = channel->fd();
    event.data.ptr = channel;

    int fd = channel->fd();

    if(::epoll_ctl(epollfd_, operation, fd, &event) < 0)
    {
        if(operation == EPOLL_CTL_DEL)
        {
            LOG_ERROR("epoll_ctl del error:%d\n", errno);
        }
        else
        {
            LOG_FATAL("epoll_ctl add/mod error:%d\n", errno);
        }
    }
    
}
