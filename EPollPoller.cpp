#include "EPollPoller.h"
#include "Logger.h"
#include "Channel.h"

#include <errno.h>
#include <unistd.h>
#include <strings.h>

const int kNew = -1;//channel未添加到poller中  channel的成员index_ = -1
const int kAdded = 1;
const int kDeleted = 2;

EPollPoller::EPollPoller(EventLoop* Loop)
    :Poller(Loop),
    epoll_fd(::epoll_create1(EPOLL_CLOEXEC)),
    events_(kInitEventListSize)  // vector<epoll_event>
{
    if(epoll_fd < 0)
    {
        LOG_FATAL("epoll_create error:%d \n", errno);
    }
}

EPollPoller::~EPollPoller()
{
    ::close(epoll_fd);
}

Timestamp EPollPoller::poll(int timeoutMs, ChannelList* activeChannels)
{
    //实际上用LOG_DEBUG输出日志更为合理
    LOG_INFO("func=%s => fd total count:%lu\n",__FUNCTION__, channels_.size());

    int numEvents = ::epoll_wait(epoll_fd, &*events_.begin(), static_cast<int>(events_.size()), timeoutMs);
    int saveErrno = errno;
    Timestamp now(Timestamp::now());

    if(numEvents > 0)
    {
        LOG_INFO("%d events happened \n", numEvents);
        fillActiveChannels(numEvents, activeChannels);
        if(numEvents == events_.size())
        {
            events_.resize(events_.size() * 2 );//对vector扩容
        }
    }
    else if(numEvents == 0)
    {
        LOG_DEBUG("%s timeout! \n", __FUNCTION__);
    }
    else
    {
        if(saveErrno != EINTR)
        {
            errno = saveErrno;
            LOG_ERROR("EPollPoller::poll() err!");
        }
    }
    return now;
}

//channel update remove 调用 EventLoop update removeChannel 调用 Poller updateChannel removeChannel
//                     EventLoop 
//    ChannelList                         Poller
//                                 ChannelMap <fd,channel*>   epollfd
void EPollPoller::updateChannel(Channel* channel)
{   
    const int index = channel->index();// 获取 Channel 当前在 poller 中的状态
    LOG_INFO("func=%s  fd=%d \n",__FUNCTION__, channel->fd());
    
    if(index == kNew || index == kDeleted)
    {
        if(index == kNew)
        {
            int fd = channel->fd();
            channels_[fd] = channel;//哈希表unordered_map<int, Channel*> channels_
        }
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    else   //channel已经在poller上注册过了
    {
        int fd = channel->fd();
        if(channel->isNoneEvent())
        {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        }
        else
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}
void EPollPoller::removeChannel(Channel* channel)
{
    int fd = channel->fd();
    channels_.erase(fd);
    
    int index = channel->index();
    if(index == kAdded)
    {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);

}

// 填写活跃的连接
void EPollPoller::fillActiveChannels(int numEvents, ChannelList* activeChannels) const
//作用: epoll_wait 返回的是一个 struct epoll_event 数组。这个函数负责遍历数组，把里面的 data.ptr (即 Channel*) 提取出来，填入 activeChannels 列表中，并设置 revents。
{
    for(int i = 0 ; i < numEvents ; i++)
    {
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);//告诉 Channel 发生了什么事件 (如 EPOLLIN)
        activeChannels->push_back(channel);  //EventLoop就拿到了它的poller给它返回的所有事件的channel列表了
    }
}
    
//更新channel通道  epoll_ctl add/mod/del
void EPollPoller:: update(int op, Channel* channel)
{
/*
typedef union epoll_data 
{
    void        *ptr;   // 指针：通常用来存 Channel* 对象指针 (Muduo 的优化技巧)
    int          fd;    // 文件描述符：最原始的用法
    uint32_t     u32;   // 32 位整数
    uint64_t     u64;   // 64 位整数
} epoll_data_t;

struct epoll_event 
{
    uint32_t     events;      // 【输入/输出】事件掩码 (如 EPOLLIN, EPOLLOUT)
    epoll_data_t data;        // 【输入/输出】用户数据 ( ptr 或 fd )
};
*/

    epoll_event event;
    bzero(&event, sizeof(event));
    event.events = channel->events();
    event.data.fd = channel->fd();
    event.data.ptr = channel;
    int fd = channel->fd();

    if(::epoll_ctl(epoll_fd, op, fd, &event) < 0)
    {
        if(op== EPOLL_CTL_DEL)
        {
            LOG_ERROR("epoll_ctl delete error:%d\n", errno);
        }
        else
        {
            LOG_FATAL("epoll_ctl add/mod error:%d\n", errno);
        }
    }
}