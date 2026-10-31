/**
 * @file Logger.cpp
 * @brief 日志工具实现
 *
 * 依赖：utils/Logger.h、utils/PathUtil.h、utils/FileUtil.h
 * 文档：task.md T8.12
 */
#include "Logger.h"

#include <QDateTime>
#include <QDir>

#include "FileUtil.h"
#include "PathUtil.h"

Logger &Logger::instance()
{
    static Logger instance_;
    return instance_;
}

Logger::Logger()
    : initialized_(false)
{
}

Logger::~Logger()
{
    close();
}

bool Logger::initialize()
{
    if (initialized_) {
        return true;
    }

    // 确保 Data 目录存在
    QString dataDir = PathUtil::dataDirPath();
    FileUtil::ensureDir(dataDir);

    // 打开日志文件（追加模式）
    QString logPath = PathUtil::join(dataDir, "niuma.log");
    logFile_.setFileName(logPath);
    if (logFile_.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        logStream_.setDevice(&logFile_);
        logStream_.setCodec("UTF-8");
        initialized_ = true;
        write(LogLevel::Info, "===== 日志系统初始化 =====");
        return true;
    }
    return false;
}

void Logger::info(const QString &message)
{
    write(LogLevel::Info, message);
}

void Logger::warn(const QString &message)
{
    write(LogLevel::Warn, message);
}

void Logger::error(const QString &message)
{
    write(LogLevel::Error, message);
}

void Logger::debug(const QString &message)
{
    write(LogLevel::Debug, message);
}

void Logger::close()
{
    if (initialized_) {
        write(LogLevel::Info, "===== 日志系统关闭 =====");
        logStream_.flush();
        logFile_.close();
        initialized_ = false;
    }
}

void Logger::write(LogLevel level, const QString &message)
{
    // 如果未初始化，尝试自动初始化（保证 main 之前的日志也能写入）
    if (!initialized_) {
        initialize();
    }

    if (initialized_) {
        QString line = QString("[%1] [%2] %3")
                           .arg(currentTimestamp())
                           .arg(levelToString(level))
                           .arg(message);
        logStream_ << line << "\n";
        logStream_.flush();
    }
}

QString Logger::levelToString(LogLevel level)
{
    switch (level) {
        case LogLevel::Debug:
            return "DEBUG";
        case LogLevel::Info:
            return "INFO";
        case LogLevel::Warn:
            return "WARN";
        case LogLevel::Error:
            return "ERROR";
    }
    return "INFO";
}

QString Logger::currentTimestamp()
{
    return QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
}
