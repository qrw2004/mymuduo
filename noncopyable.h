#pragma once  //防止头文件被重复包含

/*
nocopyable被继承以后，派生类对象可以正常的构造和析构，但是派生类对象无法进行拷贝构造和赋值操作
*/
class noncopyable
{
public:
    noncopyable(const noncopyable&) = delete;//禁止拷贝构造
    noncopyable& operator=(const noncopyable&) = delete;//禁止赋值

protected:
    noncopyable() =  default;//默认构造函数 等同于noncopyable() {}
    ~noncopyable()  = default;
};