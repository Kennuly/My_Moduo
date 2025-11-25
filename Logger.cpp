#include "Logger.h"
#include <iostream>
#include "Timestamp.h"

// 获取日志的唯一实例对象
Logger &Logger::getlog()
{
    static Logger log;
    return log;
}
// 设置日志的级别
void Logger::setLogLevel(int level)
{
    logLevel_ = level;
}
// 写日志   [级别信息] time : msg
void Logger::log(std::string msg)
{
    switch (logLevel_)
    {
    case INFO:
        std::cout << "[INFO]";
        break;
    case ERROR:
        std::cout << "[ERROR]";
        break;
    case FATAL:
        std::cout << "[FATAL]";
        break;
    case DEBUG:
        std::cout << "[DEBUG]";
        break;
    default:
        break;
    }
    // 打印时间和msg
    std::cout << Timestamp::now().toString() << " -> " << msg << std::endl;
}