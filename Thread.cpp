#include "Thread.h"

static std::atomic_int32_t Thread::numCreated_ = 0;//静态成员变量要在类外进行初始化

Thread::Thread(ThreadFunc func, const std:: string& name):
    started_(false),
    joined_(false),
    tid_(0),
    func_(std::move(func)),
    name_(name)
{
    setDefaultName();
}

~Thread();

void start();
void join();

bool started() const (return started_;)
pid_t tid() const (return tid_;)
const std::string& name() const {return name_;}

static int numCreated() {return numCreated_;}


void setDefaultName();