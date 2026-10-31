/**
 * @file PathUtil.cpp
 * @brief 路径工具实现
 *
 * 依赖：utils/PathUtil.h、utils/FileUtil.h
 * 文档：task.md T8.12、coding.md 第十三部分
 */
#include "PathUtil.h"

#include <QCoreApplication>
#include <QDir>

#include "FileUtil.h"

QString PathUtil::join(const QString &a, const QString &b)
{
    if (a.isEmpty()) {
        return b;
    }
    if (b.isEmpty()) {
        return a;
    }
    return a + "/" + b;
}

QString PathUtil::normalize(const QString &path)
{
    if (path.isEmpty()) {
        return path;
    }
    // QDir::cleanPath 会统一分隔符、去除冗余 . 和 ..
    return QDir::cleanPath(path);
}

bool PathUtil::isPathTooLong(const QString &path)
{
    // Windows MAX_PATH = 260，>= 260 视为过长
    return path.length() >= 260;
}

QString PathUtil::dataDirPath()
{
    const QString stagingDataDir = qEnvironmentVariable("NIUMA_DATA_DIR");
    if (!stagingDataDir.isEmpty()) {
        return QDir::cleanPath(stagingDataDir);
    }
    return join(FileUtil::appDirPath(), "Data");
}

QString PathUtil::databasePath()
{
    return join(dataDirPath(), "database.db");
}

QString PathUtil::defaultBackupDirPath()
{
    return join(dataDirPath(), "backups");
}

QString PathUtil::autoNiumaPath()
{
    return join(dataDirPath(), "NIUMA_auto.niuma");
}
