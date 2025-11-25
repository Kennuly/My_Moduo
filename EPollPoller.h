#pragma once
#include "Poller.h"
#include "Timestamp.h"
#include <vector>
#include <sys/epoll.h>

/*
    epoll的使用:监听集合为红黑树，就绪集合为队列
        epoll_create(); 
        epoll_ctl();    add/mod/del
        epoll_wait();
*/

class EPollPoller : public Poller
{
public:
    EPollPoller(EventLoop *loop);
    ~EPollPoller() override; 

    //重写基类的抽象方法
    Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;
    void updateChannel(Channel *channel) override;
    void removeChannel(Channel *channel) override;
private:
    static const int kInitEventList = 16;

    //填写活跃的连接
    void fillActiveChannels(int numEvent, ChannelList *activeChannels) const;
    
    //更新channel通道
    void update(int operation, Channel *channel);   //operation代表ADD, MOD, DEL

    using EventList = std::vector<epoll_event>;

    int epollfd_;       
    EventList events_;
};