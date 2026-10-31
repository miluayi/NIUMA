/**
 * @file PathUtil.h
 * @brief 路径工具类（静态方法集合）
 *
 * 提供路径拼接、规范化、长度检查、Data 目录路径获取等工具方法。
 *
 * 依赖：Qt5 Core
 * 文档：task.md T8.12、coding.md 第十三部分
 */
#pragma once

#include <QString>

/**
 * @brief 路径工具类
 *
 * 全部为静态方法，无需实例化。
 * 数据目录约定：{程序目录}/Data/
 *   - database.db     数据库文件
 *   - niuma.log       日志文件
 *   - backups/        默认备份目录
 *   - NIUMA_auto.niuma 自动备份文件
 */
class PathUtil {
public:
    /**
     * @brief 路径拼接（使用 / 分隔符）
     * @param a 前段路径
     * @param b 后段路径
     * @return 拼接后的路径
     */
    static QString join(const QString &a, const QString &b);

    /**
     * @brief 规范化路径（统一分隔符、去除冗余 . 和 ..）
     * @param path 原始路径
     * @return 规范化后的路径
     */
    static QString normalize(const QString &path);

    /**
     * @brief 路径长度是否超过 Windows 限制（>=260）
     * @param path 路径
     * @return true 表示过长
     */
    static bool isPathTooLong(const QString &path);

    /**
     * @brief 获取 Data 目录路径（appDir + "/Data"）
     * @return Data 目录绝对路径
     */
    static QString dataDirPath();

    /**
     * @brief 获取 database.db 路径（Data/database.db）
     * @return 数据库文件绝对路径
     */
    static QString databasePath();

    /**
     * @brief 获取默认备份目录路径（Data/backups）
     * @return 备份目录绝对路径
     */
    static QString defaultBackupDirPath();

    /**
     * @brief 获取 NIUMA_auto.niuma 路径（Data/NIUMA_auto.niuma）
     * @return 自动备份文件绝对路径
     */
    static QString autoNiumaPath();
};
