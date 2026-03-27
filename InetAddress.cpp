#include "InetAddress.h"

#include <strings.h>
#include <string.h>

InetAddress::InetAddress(uint16_t port, std::string ip)//参数的默认值在声明和定义中只能出现一次
{
    bzero(&addr_, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    addr_.sin_addr.s_addr = inet_addr(ip.c_str());//把字符串转成整数IP地址(点分十进制)  转成网络字节序
}

std::string InetAddress::toIp() const
{
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
    return buf;
}

std::string InetAddress::toIpPort() const
{
    //ip:port
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
    size_t end = strlen(buf);
    uint16_t port = ntohs(addr_.sin_port);
    sprintf(buf+end, ":%u", port);
    return buf;
}

uint16_t InetAddress::toPort() const
{
    return ntohs(addr_.sin_port);
}

#include <iostream>
int main()
{
    InetAddress addr(8080);
    std::cout << addr.toIpPort() << std::endl;

    return 0;
}