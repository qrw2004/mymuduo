#include "Buffer.h"

#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>

ssize_t Buffer::readFd(int fd, int* saveErrno)
{
    char extrabuf[65536] = {0};//栈上的内存空间 64K

    struct iovec vec[2];

    const size_t writeable = writeableBytes(); //这是Buffer底层缓冲区剩余的可写空间大小
    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writeable;

    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);

    const int iovcnt = (writeable < sizeof(extrabuf))? 2 : 1;
    const ssize_t n = ::readv(fd, vec, iovcnt);

    if(n < 0)
    {
        *saveErrno = errno;
    }
    else if (n <= writeable) // buffer的可写缓冲区已经能够存储读出来的数据了
    {
        writerIndex_ += n;
    }
    else //extrabuf里面也写入了数据
    {
        writerIndex_ = buffer_.size();
        append(extrabuf, n-writeable); //writeIndex_开始写 n - writebale 大小的数据
    }
    return n;
}

ssize_t Buffer::writeFd(int fd, int*saveErrno)
{
    ssize_t n = ::write(fd, peek(), readableBytes());

    if(n < 0)
    {
        *saveErrno = errno;
    }
    return n;
}