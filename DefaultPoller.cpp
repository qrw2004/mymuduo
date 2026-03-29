#include "Poller.h"

#include <stdlib.h>

//因为基类Poller不能引用派生类PollPoller、EpollPoller，无法实现newDefaultPoller函数，所以单独在一个源文件中实现

Poller* Poller::newDefaultPoller(EventLoop* Loop)
{
    if(::getenv("MUDUO_USE_POLL"))
    {
        return nullptr;//生成poll的实例
    }
    else
    {
        return nullptr;//生成epoll的实例
    }
}