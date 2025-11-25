#pragma once

/*
宏 / 级别	含义	        用途
LOG_TRACE	追踪	        打印最详细的信息，用于调试细节
LOG_DEBUG	调试	        一般调试级别，开发时使用
LOG_INFO	信息	        程序运行中的关键信息
LOG_WARN	警告	        出现非致命问题的提示
LOG_ERROR	错误	        程序出现错误但未崩溃
LOG_FATAL	严重错误	    输出后程序会终止（abort）
 */

//  定义日志的级别  INFO    代表“信息级”日志，常用于记录程序正常运行的关键节点或状态
//                 ERROR   程序在运行过程中出现 错误（Error），导致某些操作失败，但程序整体仍然可以继续运行
//                 FATAL   代表 严重错误/致命错误，通常表示程序无法继续运行下去，需要立刻终止
//                 DEBUG   表示 调试信息，用于开发阶段了解程序内部状态

#include "nocopyable.h"
#include <string>

// LOG_INFO("%s %d", arg1, arg2);
#define LOG_INFO(LogmsgFormat, ...)                       \
    do                                                    \
    {                                                     \
        Logger &logger = Logger::getlog();                \
        logger.setLogLevel(INFO);                         \
        char buf[1024] = {0};                             \
        snprintf(buf, 1024, LogmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                  \
    } while (0);

#define LOG_ERROR(LogmsgFormat, ...)                      \
    do                                                    \
    {                                                     \
        Logger &logger = Logger::getlog();                \
        logger.setLogLevel(ERROR);                        \
        char buf[1024] = {0};                             \
        snprintf(buf, 1024, LogmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                  \
    } while (0);

#define LOG_FATAL(LogmsgFormat, ...)                      \
    do                                                    \
    {                                                     \
        Logger &logger = Logger::getlog();                \
        logger.setLogLevel(FATAL);                        \
        char buf[1024] = {0};                             \
        snprintf(buf, 1024, LogmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                  \
        exit(-1);                                         \
    } while (0);

#ifdef MUDEBUG
#define LOG_DEBUG(LogmsgFormat, ...)                      \
    do                                                    \
    {                                                     \
        Logger &logger = Logger::getlog();                \
        logger.setLogLevel(DEBUG);                        \
        char buf[1024] = {0};                             \
        snprintf(buf, 1024, LogmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                  \
    } while (0);
#else
#define LOG_DEBUG(LogmsgFormat, ...)
#endif

enum LogLevel
{
    INFO,  // 普通信息
    ERROR, // 错误信息
    FATAL, // core信息（崩溃）
    DEBUG, // 调试信息
};

// 输出一个日志类
class Logger : nocopyable
{
public:
    // 获取日志的唯一实例对象
    static Logger &getlog();
    // 设置日志的级别
    void setLogLevel(int level);
    // 写日志
    void log(std::string msg);

private:
    Logger() {}
    int logLevel_;
};