#pragma once

#include <string>

#include "noncopyable.h"

//LOG_INFO("%s %d", arg1, arg2)
//宏的代码后加 \ 用来换行
//用do while(0)防止造成意想不到的错误
#define LOG_INFO(logmsgFormat, ...) \
    do \
    { \
        Logger& logger =  Logger::instance(); \
        logger.setLogLevel(INFO); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    }while(0);

#define LOG_ERROR(logmsgFormat, ...) \
    do \
    { \
        Logger& logger =  Logger::instance(); \
        logger.setLogLevel(ERROR); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    }while(0);

#define LOG_FATAL(logmsgFormat, ...) \
    do \
    { \
        Logger& logger =  Logger::instance(); \
        logger.setLogLevel(FATAL); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
        exit(-1); \
    }while(0);

#ifdef MUDEBUG  //如果定义了 MUDEBUG 就输出正常的调试日志的信息
#define LOG_DEBUG(logmsgFormat, ...) \
    do \
    { \
        Logger& logger =  Logger::instance(); \
        logger.setLogLevel(DEBUG); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    }while(0);
#else //如果没定义 什么也不输出
    #define LOG_DEBUG(logmsgFormat, ...)
#endif

//定义日志的级别 INFO  ERROR  FATAL  DEBUG  
enum LogLevel
{
    INFO,  //普通信息
    ERROR, //错误信息
    FATAL, //core(崩溃)信息
    DEBUG, //调试信息
};

//输出一个日志表
class Logger : noncopyable
{
public:
    //获取日志唯一的实例对象
    static Logger& instance();
    //设置日志级别
    void setLogLevel(int level);
    //写日志
    void log(std::string msg/*message*/);

private:
    int logLevel_;
    Logger(){}  //私有构造函数防止外部实例化  new Logger()
};