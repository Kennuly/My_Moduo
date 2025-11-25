# My_Moduo
Moduo网络库

#       muduo网络库给用户提供了两个主要的类
#       [TcpServer:用于编写服务器程序的]
#       [TcpClient:用于编写客户端程序的]

# epoll + 线程池
# 好处：能够把网络I/O的代码和业务代码区分开
#           用户的连接和断开        用户的可读写事件



# nocopyable.h        nocopyable被继承后，派生类可以正常构造和析构，但派生类无法进行拷贝构造和赋值操作
# Logger.h/.cpp       使用单例模式实现日志的相关操作
# Timestamp.h/.cpp    实现时间相关代码
# InetAddress.h/.cpp  实现网络连接的ip，端口，协议(IPV4)相关信息  (bind, accept,connect都会用到)
# Channel.h/.cpp      它把一个 fd、它关心的事件类型（读/写）、以及这些事件的处理函数（回调）绑定在一起封装管理，是事件与回调#                     之间的桥梁     
# Poller.h/.cpp       维护 Channel 列表（fd 到 Channel 的映射）,给所有IO复用保留统一的接口。Poller 是连接 EventLoop 和  #                     Channel 的桥梁，底层封装 epoll/poll 系统调用，为上层提供统一的事件检测接口
# DefaultPoller.cpp   为Poller.h文件中，实例化一个Poller*而创建的文件。
# EPollPoller.h/.cpp  实现epoll的具体功能，epoll_create,ctl,wait
#                     [ChannelList] 和 [Poller]实现的Reator模型中的 Demultiplexer（事件多路分发器）
# CurrentThread.h/.cpp 提供一个命名空间，记录当前线程tid的缓存
# EventLoop.h/.cpp    EventLoop 就是一个 事件堆（event loop），串联 Channel 和 Poller，任务分发和回调执行，跨线程唤醒
# 