#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"

#include <sys/epoll.h>

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;// 读事件：EPOLLIN (普通数据可读) | EPOLLPRI (紧急数据可读，如 TCP 带外数据)
const int Channel::kWriteEvent = EPOLLOUT;// 写事件：EPOLLOUT (可写入数据)

Channel::Channel(EventLoop* loop, int fd) 
    : loop_(loop),       // 绑定所属的事件循环 (线程)
      fd_(fd),           // 绑定监听的文件描述符 (socket)
      events_(0),        // 初始不关注任何事件
      revents_(0),       // 初始没有发生任何事件
      index_(-1),        // 标记为 kNew (未注册到 epoll)
      tied_(false)       // 初始未绑定生命周期对象
{}

Channel::~Channel(){}

//channel的tie方法什么时候调用过？ 一个TcpConnection新连接创建的时候 TcpConnection -> channel
void Channel::tie(const std::shared_ptr<void>& obj)//绑定当前 Channel 所属的对象
{
    tie_ = obj;
    tied_ = true;
}

//当改变Chann所表示的fd的events事件后， update负责在poller里面更改fd相应的事件epoll_ctl
void Channel::update()
{
    //通过Channel所属的EventLoop, 调用poller的相应方法， 注册fd的events事件
    loop_->updateChannel(this);
}

//在Channel所属的EventLoop中把所属的Channel删除掉
void Channel::remove()
{
    loop_->removeChannel(this);
}


/*
 1. 检查是否启用了生命周期绑定 (tied_)。
 2. 如果启用了，尝试通过 weak_ptr 锁定对象 (guard = tie_.lock())。
    如果 lock 成功 (guard 非空)：说明对象还活着，执行 handleEventWithGuard。
    如果 lock 失败 (guard 为空)：说明对象已被销毁，直接忽略，防止崩溃。
 3. 如果没启用绑定，直接执行 handleEventWithGuard。
*/
void Channel::handleEvent(Timestamp receiveTime)
{
    if(tied_)
    {
        std::shared_ptr<void> guard = tie_.lock();
        if(guard)
        {
            handleEventWithGuard(receiveTime);
        }
    }
    else
    {
        handleEventWithGuard(receiveTime);
    }
}

//根据poller通知的Channel发生的具体时间， 由Channel负责调用具体的回调操作
void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    LOG_INFO("Channel handleEvent revents:%d\n", revents_);

    if( (revents_ & EPOLLHUP) && !(revents_ & EPOLLIN) )//发生了 HUP (挂起) 且 没有数据可读 (EPOLLIN)
    {
        if(closeCallback_)
        {
            closeCallback_();
        }
    }
    
    if(revents_ & EPOLLERR)//如果发生错误
    {
        if(errorCallback_)
        {
            errorCallback_();
        }
    }

    if(revents_ & (EPOLLIN | EPOLLPRI) )//有数据可读 (EPOLLIN) 或 有紧急数据 (EPOLLPRI)
    {
        if(readCallback_)
        {
            readCallback_(receiveTime);
            // 读回调需要传入时间戳，用于计算网络延迟
        }
    }

    if(revents_ & EPOLLOUT)//可写入数据 (EPOLLOUT)
    {
        if(writeCallback_)
        {
            writeCallback_();
        }
    }

}