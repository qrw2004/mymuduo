#pragma once

#include "noncopyable.h"
#include "Timestamp.h"

#include <vector>
#include <unordered_map>

class Channel;
class EventLoop;

//muduo库中多路事件分发器的核心IO复用模块
class Poller : noncopyable
{
public:
    using ChannelList = std::vector<Channel*>;//using 等价于typedef 给数据类型起别名

    Poller(EventLoop* Loop);
    virtual ~Poller() = default;

    //给所有IO复用保留统一的接口  只保留纯虚函数，等待派生类来实现
    virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels) = 0;//作用：等待事件发生
    virtual void updateChannel(Channel* channel) = 0;//作用：更新内核监听状态
    virtual void removeChannel(Channel* channel) = 0;//作用：从内核移除监听
    
    //判断参数channel是否在当前Poller当中 
    bool hasChannel(Channel* channel) const;

    //EventLoop可以通过该接口获取默认的IO复用的具体实现
    static Poller* newDefaultPoller(EventLoop* Loop);

protected:
    //key->sockfd   value->sockfd所属的channel通道类型
    using ChannelMap = std::unordered_map<int, Channel*>;//记录当前 Poller 管理的所有 fd -> Channel* 的映射
    ChannelMap channels_;

    //子类 (EPollPoller) 需要访问这个 channels_ 来实现 hasChannel 或者辅助 update/remove 逻辑。如果是 private，子类就无法直接访问了。

private:
    EventLoop* ownerLoop_;//定义Poller所属的事件循环EventLoop
};