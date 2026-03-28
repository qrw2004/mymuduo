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
    char buf[64] = {0};// 准备缓冲区

    // 第一步：先把 IP 转成字符串放入 buf
    // 此时 buf 内容: "192.168.1.1\0"
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);

    // 第二步：计算当前 IP 字符串的长度
    // strlen 返回不包括 '\0' 的字符数
    size_t end = strlen(buf);//size_t 无符号整数类型

    // 第三步：把端口号转为主机字节序
    uint16_t port = ntohs(addr_.sin_port);

    // 第四步：关键拼接
    // buf + end 指向 IP 字符串末尾的 '\0' 位置。
    // sprintf 从这里开始写入，覆盖掉原来的 '\0'，写入 ":8080"，并在最后自动补一个新的 '\0'
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