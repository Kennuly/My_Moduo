#include "InetAddress.h"
#include <string.h>

InetAddress::InetAddress(uint16_t port, std::string ip)
{
    bzero(&addr_, sizeof(addr_));
    addr_.sin_port = htons(port);
    addr_.sin_family = AF_INET;
    addr_.sin_addr.s_addr = inet_addr(ip.c_str());
}
std::string InetAddress::toIp() const
{
    //addr_in里读出ip
    //char *buf = inet_ntoa(addr_.sin_addr);  //inet_ntoa() 返回的是静态缓冲区指针（不可多线程安全）,每次调用都会覆盖上次结果。

    char buf[64] = {0};
    inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));  //这是线程安全版本，也是 C++ 推荐用法。
    return buf;
}
std::string InetAddress::toIpPort() const
{
    //ip : port
    char buf[64] = {0};
    inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
    size_t len = strlen(buf);
    uint16_t port = ntohs(addr_.sin_port);
    //sprintf(buf + len, ":%u", port);    //不安全，因为它不会检查缓冲区是否够大。若端口或偏移导致越界，会发生内存覆盖
    snprintf(buf + len, sizeof(buf) - len, ":%u", port);
    return buf;
}
uint16_t InetAddress::toPort() const
{
    return ntohs(addr_.sin_port);
}
const sockaddr_in *InetAddress::getSockAddr() const
{
    return &addr_;
}

