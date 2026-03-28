#pragma once

#include "noncopyable.h"
#include "Timestamp.h"

#include <memory>
#include <functional>


class EventLoop;
class Timestamp;

/*
理清楚 EventLoop  Channel  Poller之间的关系   Reactor模型上对应Demultiplex(多路事件分发器)

Channel 理解为通道， 封装了sockfd及其感兴趣的event， 如EPOLLIN  EPOLLOUT事件
还绑定了poller返回的具体事件
*/
class Channel : noncopyable
{
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;

    Channel(EventLoop *Loop, int fd);
    ~Channel();

    //fd得到poller通知以后，处理事件
    /*
    事件分发入口。
    当 EventLoop 拿到活跃的 Channel 后，调用此函数。
    1. 如果绑定了 tie_，先尝试 lock()，如果失败则直接返回（对象已销毁）。
    2. 如果成功，调用 handleEventWithGuard。
    */
    void handleEvent(Timestamp receiveTime);

    //设置回调函数对象
    void setReadCallback(ReadEventCallback cb) {readCallback_ = std::move(cb);}//move :把一个左值（有名字的变量）强制转换为右值引用（即将被销毁的临时值）
    //直接把cb的内部资源（指针、函数对象）给 readCallback_ ,省去了第二次拷贝，性能大幅提升。
    void setWriteCallback(EventCallback cb) {writeCallback_ = std::move(cb);}
    void setCloseCallback(EventCallback cb) {closeCallback_ = std::move(cb);}
    void setErrorCallback(EventCallback cb) {errorCallback_ = std::move(cb);}

    //防止当Channel被手动remove掉，Channel还在执行回调操作
    void tie(const std::shared_ptr<void>&);

    int fd() const {return fd_;}
    int events() const {return events_;}
    void set_revents(int revt) {revents_ = revt;}

    //设置fd相应的事件状态
    void enableReading() {events_ |= kReadEvent; update();}
    void disableReading() {events_ &= ~kReadEvent; update();}
    void enableWriting() {events_ |= kWriteEvent; update();}
    void disableWriting() {events_ &= ~kWriteEvent; update();}
    void disableAll() {events_ = kNoneEvent; update();}

    //返回fd当前的事件状态
    bool isNoneEvent() const {return events_ == kNoneEvent;}
    bool isWriting() const {return events_ & kWriteEvent;}
    bool isReading() const {return events_ & kReadEvent;}

    int index() {return index_;}
    void set_index(int idx) {index_ = idx;}

    EventLoop* ownerLoop() {return loop_;}//一个线程可以监听多个fd
    void remove();//用于删除Channel

private:


    //调用 loop_->updateChannel(this)。会根据 index_ 的状态，决定调用 epoll_ctl(ADD), epoll_ctl(MOD) 还是 epoll_ctl(DEL)。
    void update();

    /*
    真正执行回调的地方。
    根据 revents_ 的值进行判断：
    如果有 EPOLLIN -> 调用 readCallback_。
    如果有 EPOLLOUT -> 调用 writeCallback_。
    如果有 EPOLLHUP 且没有数据 -> 调用 closeCallback_。
    如果有 EPOLLERR -> 调用 errorCallback_。
    */
    void handleEventWithGuard(Timestamp receiveTime);

    static const int kNoneEvent;//没有事件
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop *loop_;//事件循环  所属的事件循环指针。每个 Channel 必须属于且仅属于一个 EventLoop（即一个线程）。
    const int fd_;   //poller监听的对象  文件描述符 (File Descriptor) 这是操作系统内核识别 Socket 的 ID
    int events_;     //注册fd感兴趣的事件
    int revents_;    //poller返回的具体发生的事件、实际发生的事件 (Returned Events)   handleEvent 函数就是根据这个值来决定调用哪个回调
    int index_;
    /*在 Poller 中的状态索引。用来标记这个 Channel 当前在 epoll 里的状态：
    1 (kNew)：新创建的，还没加到 epoll 里。
    0 (kAdded)：已经在 epoll 里了。
    2 (kDeleted)：已经从 epoll 里删除了。
    */

    std::weak_ptr<void> tie_;//指向拥有这个 Channel 的对象
    //用于防止“悬空指针”：如果 TcpConnection 被销毁了，但 EventLoop 还没来得及移除 Channel，此时通过 tie_.lock() 会发现对象已死，从而安全地跳过回调，避免崩溃
    bool tied_;//标记是否调用了 tie() 函数。如果为 false，则不检查 tie_，直接执行回调

    //因为Channel通道里面能够获知fd最终发生的具体的事件revents, 所以它负责调用具体事件的回调操作
    ReadEventCallback readCallback_;//读事件回调函数。当 revents_ 包含读事件时，执行这个函数。
    EventCallback writeCallback_;//写事件回调函数。当 revents_ 包含写事件时，执行这个函数
    EventCallback closeCallback_;//关闭事件回调函数。当对方断开连接（收到 EPOLLHUP 或 EPOLLRDHUP）时执行。通常用于清理连接资源。
    EventCallback errorCallback_;//错误事件回调函数。当发生错误（EPOLLERR）时执行。通常用于记录日志并关闭连接。
};