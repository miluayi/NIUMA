/**
 * @file Logger.h
 * @brief 简单日志工具（单例模式）
 *
 * 写入 {程序目录}/Data/niuma.log
 * 日志格式：[YYYY-MM-DD HH:mm:ss] [LEVEL] message
 * 提供 info/warn/error/debug 方法，支持 qDebug() 风格的流式输出。
 *
 * 依赖：Qt5 Core
 * 文档：task.md T8.12
 */
#pragma once

#include <QFile>
#include <QObject>
#include <QTextStream>

/**
 * @brief 日志级别枚举
 */
enum class LogLevel {
    Debug,
    Info,
    Warn,
    Error
};

/**
 * @brief 日志工具单例
 *
 * 用法：
 *   Logger::instance().info("启动成功");
 *   Logger::instance().error("数据库初始化失败: " + errorMsg);
 *
 * 日志文件位置：{程序目录}/Data/niuma.log
 */
class Logger : public QObject {
    Q_OBJECT

public:
    /**
     * @brief 获取单例实例
     */
    static Logger &instance();

    /**
     * @brief 初始化日志文件（在 Data 目录下创建 niuma.log）
     * @return 是否成功打开日志文件
     */
    bool initialize();

    /**
     * @brief 记录 Info 级别日志
     */
    void info(const QString &message);

    /**
     * @brief 记录 Warn 级别日志
     */
    void warn(const QString &message);

    /**
     * @brief 记录 Error 级别日志
     */
    void error(const QString &message);

    /**
     * @brief 记录 Debug 级别日志
     */
    void debug(const QString &message);

    /**
     * @brief 关闭日志文件
     */
    void close();

private:
    Logger();
    ~Logger() override;
    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;

    /**
     * @brief 写入一条日志
     */
    void write(LogLevel level, const QString &message);

    /**
     * @brief 将日志级别转为字符串
     */
    static QString levelToString(LogLevel level);

    /**
     * @brief 获取当前时间戳字符串
     */
    static QString currentTimestamp();

    QFile logFile_;
    QTextStream logStream_;
    bool initialized_;
};
