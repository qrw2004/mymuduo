#include "Poller.h"
#include "Channel.h"

Poller::Poller(EventLoop* Loop)
    : ownerLoop_(Loop)
{} 

bool Poller::hasChannel(Channel* channel) const
{
    auto it = channels_.find(channel->fd());//channels_ 是一个哈希表u nordered_map<int, Channel*>   key->sockfd   value->sockfd所属的channel通道类型
    return it != channels_.end() && it->second == channel;
}

