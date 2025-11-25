#pragma once

#include <vector>
#include <iostream>
#include <string>
#include <algorithm>

// 网络库底层缓冲区类型定义
class Buffer
{
public:
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;

    explicit Buffer(size_t initialSize = kInitialSize) : buffer_(kCheapPrepend + initialSize) 
                                                        , readerIndex_(kCheapPrepend)
                                                        , writerIndex_(kCheapPrepend) 
    {}

    size_t readableBytes() const 
    {
        return writerIndex_ - readerIndex_;
    }
    size_t writableBytes() const
    {
        return buffer_.size() - writerIndex_;
    }
    size_t prependableBytes() const
    {
        return readerIndex_;
    }
    //返回缓冲区中可读数据的起始地址
    const char* peek()
    {
        return begin() + readerIndex_;
    }

    //onMessage string <- Buffer
    void retrieve(size_t len)
    {
        if(len < readableBytes())
        {
            readerIndex_ += len;    //应用只读取了可读缓冲区数据的一部分，就是len长度，还剩下readerIndex_ += len -> writerIndex_没读
        }
        else    //len == readableBytes()
        {
            retrieveAll();
        }
    }
    
    void retrieveAll()
    {
        readerIndex_ = writerIndex_ = kCheapPrepend;
    }
    //把onMessage函数上报的Buffer数据，转成string类型的数据返回
    std::string retrieveAllString()
    {
        return restriveAsString(readableBytes());   //应用可读取数据的长度
    }
    std::string restriveAsString(size_t len)
    {
        //从内存地址 s 开始，拷贝 n 个字节，构造一个字符串。
        std::string result(peek(), len);
        retrieve(len);  //上面把缓冲区中可读的数据，已经读取出来，这里肯定要对缓冲区进行复位操作
        return result;
    }

    //buffer_.size() - writeIndex_      len
    void ensureWriteableBytes(size_t len)
    {
        if(writableBytes() < len)
        {
            makespace(len);     //扩容函数
        }
    }
    // 把[data, data + len]内存上的数据，添加到writable缓冲区当中
    void append(const char *data, size_t len)
    {
        ensureWriteableBytes(len);
        std::copy(data, data + len, beginWrite());
        writerIndex_ += len;
    }
    char *beginWrite()
    {
        return begin() + writerIndex_;
    }
    const char* beginWrite() const 
    {
        return begin() + writerIndex_;
    }

    //从fd上读取数据
    ssize_t readFd(int fd, int *saveErrno);
    //通过fd发送数据
    ssize_t writeFd(int fd, int *saveErrno);
private:
    char *begin()
    {
        return &*buffer_.begin();
    }
    const char *begin() const
    {
        return &*buffer_.begin();
    }
    void makespace(size_t len)
    {
        // 我能写的空间 + 我已经读了的空间 < 我要写的数据 + 预留段 --> 我的string空间不够，就要扩容
        if(writableBytes() + prependableBytes() < len + kCheapPrepend)
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
    size_t readerIndex_;
    size_t writerIndex_;
};
