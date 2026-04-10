#pragma once

#include <vector>
#include <string>
#include <algorithm>

/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=      size


//网络库底层的缓冲Ⅶ类型定义
class Buffer
{
public:
    //size_t 是一个无符号整型 (unsigned integer)
    static const size_t kCheapPrepend = 8;//数据包的长度
    static const size_t kInitialSize = 1024;//缓冲区初始大小

    explicit Buffer(size_t initalSize = kInitialSize):
        buffer_(kCheapPrepend + initalSize),
        readerIndex_(kCheapPrepend),
        writerIndex_(kCheapPrepend)//初始时没有数据，readerIndex和wrietrIndex指向同一个地方
        {}

    size_t readableBytes() const
    {
        return writerIndex_-readerIndex_;
    }

    size_t writeableBytes() const
    {
        return buffer_.size() - writerIndex_;
    }

    size_t prependableBytes() const
    {
        return readerIndex_;
    }

    //返回缓冲区中可读数据的起始地址
    const char* peek() const
    {
        return begin() + readerIndex_;
    }

    void retrieve(size_t len)
    {
        if(len < readableBytes())
        {
            readerIndex_ = readerIndex_ + len;// 应用只读取了刻度缓冲区数据的一部分，就是len， 还剩下从 readerIndex_ + len 到 writerIndex这部分数据没读
        }
        else
        {
            retrieveAll();
        }
    }

    void retrieveAll()
    {
        readerIndex_ = writerIndex_ = kCheapPrepend;
    }

    // 把onMessage函数上报的Buffer数据，转成string类型的数据返回
    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes());//应用可读取数据的长度
    }

    std::string retrieveAsString(size_t len)
    {
        std::string result(peek(), len);//把缓冲区中可读的数据读取出来
        retrieve(len);//对缓冲区进行复位操作

        return result;
    }

    void ensureWriteableBytes(size_t len)
    {
        if(writeableBytes() < len)
        {
            makeSpace(len);
        }
        else
        {

        }
    }

    // 把[data,data+len]内存上的数据， 添加到writeable缓冲区当中
    void append(const char* data, size_t len)
    {
        ensureWriteableBytes(len);
        std::copy(data, data + len, beginWrite());//把数据添加到可写的缓冲区当中
        writerIndex_ += len;
    }

    char* beginWrite()
    {
        return begin() + writerIndex_;
    }

    //从fd上读取数据
    ssize_t readFd(int fd, int* saveErrno);

    //通过fd发送数据
    ssize_t writeFd(int fd, int* saveErrno);

private:
    char* begin()
    {
        return &*buffer_.begin();//vector底层数组首元素的地址，也就是数组的起始地址
        // &*buffer_begin() = buffer_.data() = &buffer_[0] (如果buffer_不为空)
    }

    const char* begin() const
    {
        return &*buffer_.begin();
    }

    void makeSpace(size_t len)
    {
        if(writeableBytes() + prependableBytes() < len + kCheapPrepend)
        {
            buffer_.resize(writerIndex_ + len);
        }
        else
        {
            size_t readable = readableBytes();
            std::copy(begin() + readerIndex_, begin() + writerIndex_, begin() + kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readable;
        }
    } 

    std::vector<char> buffer_;
    size_t readerIndex_;//从这里开始读
    size_t writerIndex_;//从这里开始写
};