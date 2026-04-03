#include "EventLoop.h"
#include "Logger.h"
#include "Poller.h"
#include "Channel.h"

#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <memory>

//防止一个线程创建多个EventLoop   每个线程都有自己独立的一份副本。用来记录当前线程正在运行哪个 EventLoop
__thread EventLoop* t_loopInThisThread = nullptr;//线程局部存储。确保每个线程只认自己的 Loop


//定义默认的Poller  IO复用接口的超时时间 10s
const int kPollTimeMs = 10000;

//创建wakeupfd，用来notify唤醒subReactor处理新来的channel
int createEventfd()
{
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if(evtfd < 0)
    {
        LOG_FATAL("eventfd error:%d \n", errno);
    }
    return evtfd;
}

EventLoop::EventLoop()
    :looping_(false),
    quit_(false),
    callingPendingFunctors_(false),
    threadId_(CurrentThread::tid()),//绑定当前线程
    poller_(Poller::newDefaultPoller(this)),
    wakeupFd_(createEventfd()),//调用 createEventfd() 得到 wakeupFd_
    wakeupChannel_(new Channel(this, wakeupFd_)) //std::unique_ptr<Channel> wakeupChannel_  创建 wakeupChannel_ 监听这个 fd
{
    LOG_DEBUG("EventLoop created %p in thread %d", this, threadId_);
    if(t_loopInThisThread)//如果该线程已经有了一个Loop,就不创建这个新的Loop了
    {
        LOG_FATAL("Another EventLoop %p exists in this thread %d \n", t_loopInThisThread, threadId_);
    }
    else
    {
        t_loopInThisThread = this;
    }

    //设置wakeupfd的事件类型以及发生事件后的回调操作
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    
    //每一个EventLoop都将监听wakeupchannel的EPOLLIN读事件了
    wakeupChannel_->enableReading();

}

EventLoop::~EventLoop()//回收资源
{
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

//开启事件循环
void EventLoop::loop()
{
    looping_ = true;
    quit_ = false;
                                                                                                                                                                                                                                                                                                                                                                                                                                      
    LOG_INFO("EventLoop %p start looping \n", this);

    while(!quit_)
    {
        activeChannels_.clear();
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);//调用 poller_->poll() 阻塞等待事件。

        for(Channel* channel : activeChannels_)//遍历活跃 Channel 列表，调用 channel->handleEvent() 处理业务
        {
            //poller监听哪些channel发生事件了，然后上报给EventLoop，通知channel处理相应的事件
            channel->handleEvent(pollReturnTime_);
        }
        //执行当前的EventLoop事件循环需要处理的回调操作
        doPendingFunctors();
    }

    LOG_INFO("EventLoop %p stop looping. \n", this);
    looping_ = false;
}

//退出事件循环   1.loop在自己的线程中调用quit  2.在非loop的线程中，调用loop的quit
void EventLoop::quit()//
{
    quit_ = true;

    if(!isInLoopThread())//如果调用者不在当前 IO 线程，调用 wakeup() 唤醒沉睡的 IO 线程，让它检测到 quit_ 并退出循环
    //如果是在其他线程中调用的quit  在一个subloop(worker)中，调用了mainloop(IO)的quit
    {
        wakeup();
    }
}

//在当前loop中执行cb
void EventLoop::runInLoop(Functor cb)
{
    if(isInLoopThread())//在当前的loop线程中执行callback(cb)
    {
        cb();
    }
    else  //在非当前loop线程中执行cb， 需要唤醒loop所在线程，执行cb
    {
        queueInLoop(cb);
    }
}


//把cb放入队列中，唤醒loop所在的线程，执行cb
void EventLoop::queueInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }//出了这个括号锁就释放掉了
    
    //唤醒相应的、需要执行上面回调操作的loop线程
    //  || callingPendingFunctors_  的意思是：当前loop正在执行回调，但是loop又有了新的回调
    if(!isInLoopThread() || callingPendingFunctors_)
    {
        wakeup();//唤醒loop所在线程
    }
}


//唤醒处理器 清空eventfd计数器，且这是 wakeupChannel_ 被触发的原因
void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_, &one, sizeof(one));
    if(n != sizeof(one))
    {
        LOG_ERROR("EventLoop::handleRead() reads %ld bytes instead of 8 \n", n);
    }
}

//唤醒loop所在的线程, 向wakeupfd_写一个数据, wakeupChannel就发生读事件，当前loop线程就会被唤醒
void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof(one));
    if(n != sizeof(one))
    {
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8 \n", n);
    }
}

    //EventLoop调用Poller的方法
void EventLoop::updateChannel(Channel* channel)
{
    poller_->updateChannel(channel);
}
void EventLoop::removeChannel(Channel* channel)
{
    poller_->removeChannel(channel);
}
bool EventLoop::hasChannel(Channel* channel)
{
    return poller_->hasChannel(channel);
}

//执行回调
void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;//标识当前loop有需要执行的回调操作

    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);//将共享队列 pendingFunctors_ 与局部变量 functors 交换
    }//出了这个括号锁就释放掉了
    for(const Functor &functor : functors)
    {
        functor();//执行当前loop需要执行的回调操作
    }
    callingPendingFunctors_ = false;
}