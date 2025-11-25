#include "Socket.h"
#include <unistd.h>
#include "Logger.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <netinet/tcp.h>
#include "InetAddress.h"
#include <memory.h>

Socket::~Socket()
{
    ::close(sockfd_);
}

void Socket::bindAddress(const InetAddress &localaddr)
{
    if (0 != bind(sockfd_, reinterpret_cast<const sockaddr *>(localaddr.getSockAddr()), sizeof(localaddr)))
    {
        LOG_FATAL("bind sockfd:%d fail\n", sockfd_);
    }
}
void Socket::listen()
{
    if (0 != ::listen(sockfd_, 1024))
    {
        LOG_FATAL("listen sockfd:%d fail\n", sockfd_);
    }
}

int Socket::accept(InetAddress *peeraddr)
{
    sockaddr_in addr;
    socklen_t len = sizeof(addr);
    bzero(&addr, sizeof(addr));
    int connfd = ::accept4(sockfd_, reinterpret_cast<sockaddr *>(&addr), &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (connfd >= 0)
    {
        peeraddr->setSockAddr(addr);
    }
    return connfd;
}
void Socket::shutdownWrite()
{
    if (::shutdown(sockfd_, SHUT_WR) < 0)
    {
        LOG_ERROR("Socket::shutdownWrite() error");
    }
}
/*
int setsockopt(int sockfd, int level, int optname,
               const void *optval, socklen_t optlen);

| 参数              | 含义                           |
| --------------- | ---------------------------- |
| `sockfd_`       | 要设置的 socket 文件描述符            |
| `IPPROTO_TCP`   | 设置 TCP 协议级别的选项               |
| `TCP_NODELAY`   | 关闭 Nagle 算法                  |
| `&optval`       | 选项的值（通常是 1 表示开启 TCP_NODELAY） |
| `sizeof optval` | 选项值的大小                       |


*/
void Socket::setTcpNoDely(bool on)
{
    int optval = on ? 1 : 0;
    // 给这个 TCP socket 设置 TCP_NODELAY 选项（关闭 Nagle 算法）,TCP 默认启用 Nagle 算法：小包会被延迟发送，等积累到一定大小才发
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof optval);

}

void Socket::setKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    //SO_KEEPALIVE，让 TCP 长连接在长时间没有读写时自动探测“对端是否还活着”。
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof optval);
}

/*
SO_REUSEADDR：允许服务器在 TIME_WAIT 状态下立即重用地址（端口）
常用于服务器重启时避免 “Address already in use” 错误

| 参数              | 说明                            |
| --------------- | ----------------------------- |
| `sockfd`        | 你的 socket 文件描述符               |
| `SOL_SOCKET`    | 选项作用于 socket 层（非 TCP/UDP 协议层） |
| `SO_REUSEADDR`  | 地址可重用                         |
| `optval`        | int，通常设为 1                    |
| `sizeof optval` | optval 大小                     |


*/
void Socket::setReuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
}
void Socket::setReusePort(bool on)
{
    int optval = on ? 1 : 0;
    //SO_REUSEPORT：端口完全可共享（高性能负载均衡）,让多个进程 / 线程 同时 bind 同一个端口。
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof optval);
}