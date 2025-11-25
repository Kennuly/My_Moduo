#pragma once

#include "nocopyable.h"
#include <functional>
#include "Timestamp.h"
#include <memory>
/*
    Reactor（也就是 EventLoop）只知道有文件描述符（fd）要监听；
    Channel 则封装了这个 fd 以及它关心的事件（如读、写、关闭、错误）；
    当事件触发时，Channel 会回调用户注册的函数。

    理清楚 EventLoop、Channel、Poller之间的关系  <==   Reactor模型上对应  多路事件分发器
    Channel 理解为通道，封装了sockfd和其感兴趣的event，如EPOLLIN,EPOLLOUT事件
    还绑定了Poller返回的具体事件
*/
class EventLoop;

class Channel : nocopyable
{
public:
    //typedef std::function<void()> EventCallback;
    using EventCallback = std::function<void()>;                //事件的回调
    using ReadEventCallback = std::function<void(Timestamp)>;   //只读事件的回调
    
    Channel(EventLoop *loop, int fd);
    ~Channel();

    // fd得到Poller通知以后，处理事件。 调用相应的回调方法
    void handleEvent(Timestamp receiveTime);

    //设置回调函数对象
    void setReadCallback(ReadEventCallback cb){readCallback_ = std::move(cb);}
    void setWriteCallback(EventCallback cb) {writeCallback_ = std::move(cb);}
    void setCloseCallback(EventCallback cb) {closeCallback_ = std::move(cb);}
    void setErrorCallback(EventCallback cb) {errorCallback_ = std::move(cb);}

    //当Channel被手动remove掉，Channel还在执行回调操作
    void tie(const std::shared_ptr<void> &obj);

    int fd() const {return fd_;}
    int events() const {return events_;}
    void set_revents(int revt) {revents_ = revt;}

    //设置fd相应的事件状态
    void enableReading() {events_ |= kReadEvent; update();}
    void disableReading() {events_ &= ~kNodeEvent; update();}
    void enableWriting() {events_ |= kWriteEvent; update();}
    void disableWriting() {events_ &= ~kWriteEvent; update();}
    void disableAll() {events_ = kNodeEvent; update();}

    //返回当前fd的事件状态
    bool isNodeEvent() const {return events_ == kNodeEvent;}
    bool isReadEvent() const {return events_ & kReadEvent;}
    bool isWriteEvent() const {return events_ & kWriteEvent;}

    int index() {return index_;}
    void set_index(int index) {index_ = index;}

    EventLoop *ownerLoop() {return loop_;}
    void remove();

private:
    void update();
    void handleEventWithGuard(Timestamp receiveTime);
private:
    static const int kNodeEvent;        //没任何事件
    static const int kReadEvent;        //读事件
    static const int kWriteEvent;       //写事件

    EventLoop *loop_;   //事件循环      (这个Channel属于哪个EventLoop)
    const int fd_;      //fd,Poller监听的对象
    int events_;        //注册fd感兴趣的事件
    int revents_;       //poller返回的具体发生的事件
    int index_;         //保存当前状态,     从未添加到 epoll    已添加到 epoll    添加过，但后来被删除
    
    std::weak_ptr<void> tie_;
    bool tied_;         //Channel 是否绑定过 TcpConnection

    //因为Channel通道里面能够获取fd最终发生的具体事件revents,所以他负责调用具体事件的回调操作
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};