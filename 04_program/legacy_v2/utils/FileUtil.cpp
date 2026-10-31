/**
 * @file FileUtil.cpp
 * @brief 文件系统工具实现
 *
 * 依赖：utils/FileUtil.h
 * 文档：task.md T8.12、PRD 第六部分
 */
#include "FileUtil.h"

#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>

bool FileUtil::ensureDir(const QString &path)
{
    if (path.isEmpty()) {
        return false;
    }
    QDir dir(path);
    if (dir.exists()) {
        return true;
    }
    return dir.mkpath(path);
}

bool FileUtil::copyFile(const QString &src, const QString &dst)
{
    if (src.isEmpty() || dst.isEmpty()) {
        return false;
    }
    // 确保目标目录存在
    QFileInfo dstInfo(dst);
    QString dstDir = dstInfo.absolutePath();
    if (!ensureDir(dstDir)) {
        return false;
    }
    // 如果目标文件已存在则先删除
    if (QFile::exists(dst)) {
        QFile::remove(dst);
    }
    return QFile::copy(src, dst);
}

bool FileUtil::copyDir(const QString &src, const QString &dst)
{
    if (src.isEmpty() || dst.isEmpty()) {
        return false;
    }
    QDir srcDir(src);
    if (!srcDir.exists()) {
        return false;
    }
    if (!ensureDir(dst)) {
        return false;
    }

    // 复制所有文件
    QStringList files = srcDir.entryList(QDir::Files | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);
    for (const QString &fileName : files) {
        QString srcFilePath = srcDir.absoluteFilePath(fileName);
        QString dstFilePath = QDir(dst).absoluteFilePath(fileName);
        if (!copyFile(srcFilePath, dstFilePath)) {
            return false;
        }
    }

    // 递归复制子目录
    QStringList dirs = srcDir.entryList(QDir::Dirs | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);
    for (const QString &dirName : dirs) {
        QString srcSubDir = srcDir.absoluteFilePath(dirName);
        QString dstSubDir = QDir(dst).absoluteFilePath(dirName);
        if (!copyDir(srcSubDir, dstSubDir)) {
            return false;
        }
    }
    return true;
}

bool FileUtil::removeDir(const QString &path)
{
    if (path.isEmpty()) {
        return false;
    }
    QDir dir(path);
    if (!dir.exists()) {
        return true;
    }
    return dir.removeRecursively();
}

bool FileUtil::moveDir(const QString &src, const QString &dst)
{
    if (src.isEmpty() || dst.isEmpty()) {
        return false;
    }
    // 先尝试直接重命名（同盘符，最快）
    if (QDir().rename(src, dst)) {
        return true;
    }
    // 跨盘符则复制后删除
    if (!copyDir(src, dst)) {
        return false;
    }
    return removeDir(src);
}

QString FileUtil::sanitizePath(const QString &name)
{
    QString result = name;
    // 过滤 Windows 非法字符：\ / : * ? " < > |
    result.remove('\\');
    result.remove('/');
    result.remove(':');
    result.remove('*');
    result.remove('?');
    result.remove('"');
    result.remove('<');
    result.remove('>');
    result.remove('|');
    // 去除首尾空格和点（Windows 不允许目录名以 . 结尾）
    result = result.trimmed();
    while (result.endsWith('.')) {
        result.chop(1);
    }
    return result;
}

qint64 FileUtil::dirSize(const QString &path)
{
    if (path.isEmpty()) {
        return 0;
    }
    QDir dir(path);
    if (!dir.exists()) {
        return 0;
    }
    return dirSizeInternal(path);
}

qint64 FileUtil::dirSizeInternal(const QString &path)
{
    qint64 totalSize = 0;
    QDirIterator it(path, QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        totalSize += it.fileInfo().size();
    }
    return totalSize;
}

bool FileUtil::pathExists(const QString &path)
{
    if (path.isEmpty()) {
        return false;
    }
    QFileInfo info(path);
    return info.exists();
}

QString FileUtil::appDirPath()
{
    return QCoreApplication::applicationDirPath();
}
