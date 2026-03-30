#pragma once

#include "Poller.h"
#include "Timestamp.h"
#include "Channel.h"

#include <vector>
#include <sys/epoll.h>

class Channel;


/*
epoll的使用
epoll_create
epoll_ctl    add/mod/del
epoll_wait
*/

class EPollPoller : public Poller
{
public:
    EPollPoller(EventLoop* Loop);
    ~EPollPoller() override;

    // 重写基类Poller的抽象方法
    Timestamp poll(int timeoutMs, ChannelList* activeChannels) override;
    void updateChannel(Channel* channel) override;
    void removeChannel(Channel* channel) override;

private:
    static const int kInitEventListSize = 16;

    // 填写活跃的连接
    void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;
    
    //更新channel通道
    void update(int op, Channel* channel);

    using EventList = std::vector<epoll_event>;//默认长度设置为16

    int epoll_fd;
    EventList events_;
};