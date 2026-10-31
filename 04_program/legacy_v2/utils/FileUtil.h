/**
 * @file FileUtil.h
 * @brief 文件系统工具类（静态方法集合）
 *
 * 提供目录创建、文件复制、目录递归操作、路径过滤等工具方法。
 *
 * 依赖：Qt5 Core
 * 文档：task.md T8.12、PRD 第六部分
 */
#pragma once

#include <QString>

/**
 * @brief 文件系统工具类
 *
 * 全部为静态方法，无需实例化。
 */
class FileUtil {
public:
    /**
     * @brief 创建目录（含父目录）
     * @param path 目录路径
     * @return 是否创建成功（已存在也返回 true）
     */
    static bool ensureDir(const QString &path);

    /**
     * @brief 复制文件
     * @param src 源文件路径
     * @param dst 目标文件路径
     * @return 是否复制成功
     */
    static bool copyFile(const QString &src, const QString &dst);

    /**
     * @brief 递归复制目录
     * @param src 源目录路径
     * @param dst 目标目录路径
     * @return 是否复制成功
     */
    static bool copyDir(const QString &src, const QString &dst);

    /**
     * @brief 递归删除目录
     * @param path 目录路径
     * @return 是否删除成功
     */
    static bool removeDir(const QString &path);

    /**
     * @brief 移动目录（先复制后删除，跨盘符安全）
     * @param src 源目录路径
     * @param dst 目标目录路径
     * @return 是否移动成功
     */
    static bool moveDir(const QString &src, const QString &dst);

    /**
     * @brief 过滤非法字符（\\ / : * ? " < > |）
     * @param name 原始名称
     * @return 过滤后的安全名称
     */
    static QString sanitizePath(const QString &name);

    /**
     * @brief 计算目录大小（含子目录）
     * @param path 目录路径
     * @return 字节数
     */
    static qint64 dirSize(const QString &path);

    /**
     * @brief 路径是否存在（文件或目录）
     * @param path 路径
     * @return 是否存在
     */
    static bool pathExists(const QString &path);

    /**
     * @brief 获取程序所在目录
     * @return 程序目录绝对路径
     */
    static QString appDirPath();

private:
    /**
     * @brief 递归计算目录大小（内部辅助）
     */
    static qint64 dirSizeInternal(const QString &path);
};
