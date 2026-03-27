#pragma once

#include <iostream>
#include <string>
#include <cstdint>

//时间类
class Timestamp
{
public:
    Timestamp();
    explicit Timestamp(int64_t microSecondsSinceEpoch);//explicit防止隐式类型转换和拷贝初始化
    static Timestamp now();
    std::string toString() const;//只读方法
private:
    int64_t microSecondsSinceEpoch_;
};