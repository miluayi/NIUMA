/**
 * @file FileService.cpp
 * @brief 文件系统业务逻辑层实现
 *
 * 实现：工作目录初始化/验证、任务目录命名/创建、状态变更目录移动、
 *       自愈、工作目录迁移、文件复制
 *
 * 依赖：service/FileService.h、database/SettingsDao、database/TaskDao、
 *       database/DatabaseManager、models/Task、models/ReferenceMaterial、
 *       utils/FileUtil、utils/PathUtil、utils/Logger
 * 文档：TD 3.9、PRD 第六部分、task.md T3.1-T3.7
 */

#include "FileService.h"

#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QHash>
#include <QMutex>
#include <QMutexLocker>
#include <QSqlError>
#include <QSqlQuery>
#include <QtConcurrent/QtConcurrentRun>

#include "DatabaseManager.h"
#include "SettingsDao.h"
#include "TaskDao.h"
#include "models/ReferenceMaterial.h"
#include "models/Task.h"
#include "utils/FileUtil.h"
#include "utils/Logger.h"
#include "utils/PathUtil.h"

// ============================================================================
// 6 个根目录名称常量定义
// ============================================================================

const QString FileService::kDirInProgress = QStringLiteral("01_工作区_进行中");
const QString FileService::kDirArchived = QStringLiteral("02_历史归档");
const QString FileService::kDirAnnualResult = QStringLiteral("03_年度成果");
const QString FileService::kDirReference = QStringLiteral("04_参考资料");
const QString FileService::kDirRecycleBin = QStringLiteral("05_回收站");
const QString FileService::kDirOtherHandler = QStringLiteral("06_他人办理");

namespace {
QMutex &copyMutex() { static QMutex mutex; return mutex; }
QHash<QString, int> &pendingCopies() { static QHash<QString, int> copies; return copies; }
bool copyPending(const QString &path)
{
    QMutexLocker lock(&copyMutex());
    return pendingCopies().value(QDir::cleanPath(path)) > 0;
}
}

// ============================================================================
// 工作目录管理
// ============================================================================

FileService::InitResult FileService::initializeWorkDir(const QString &rootPath)
{
    InitResult result;
    if (rootPath.isEmpty()) {
        result.errorMessage = QStringLiteral("工作目录路径为空");
        return result;
    }

    // 可直接选择已迁移的“工作目录”，也可选择其上级目录。
    QString journalDir = QFileInfo(rootPath).fileName() == QStringLiteral("工作目录")
        || validateStructure(rootPath) ? rootPath : PathUtil::join(rootPath, "工作目录");
    if (!FileUtil::ensureDir(journalDir)) {
        result.errorMessage = QStringLiteral("无法创建工作目录: %1").arg(journalDir);
        return result;
    }

    // 创建 6 个根目录
    QStringList rootDirs = {
        kDirInProgress, kDirArchived, kDirAnnualResult,
        kDirReference, kDirRecycleBin, kDirOtherHandler
    };
    for (const QString &dirName : rootDirs) {
        QString dirPath = PathUtil::join(journalDir, dirName);
        if (!FileUtil::ensureDir(dirPath)) {
            result.errorMessage = QStringLiteral("无法创建目录: %1").arg(dirPath);
            return result;
        }
    }

    // 保存工作目录到 settings
    if (!setWorkDir(journalDir)) {
        result.errorMessage = QStringLiteral("无法保存工作目录到配置: %1").arg(journalDir);
        return result;
    }

    Logger::instance().info(QStringLiteral("FileService: 工作目录初始化完成: %1").arg(journalDir));
    result.success = true;
    result.workDir = journalDir;
    return result;
}

bool FileService::validateWorkDir(const QString &rootPath) const
{
    if (rootPath.isEmpty()) {
        return false;
    }
    QString journalDir = PathUtil::join(rootPath, "工作目录");
    return validateStructure(journalDir);
}

bool FileService::validateStructure(const QString &workDir) const
{
    if (workDir.isEmpty() || !QDir(workDir).exists()) {
        return false;
    }
    QStringList rootDirs = {
        kDirInProgress, kDirArchived, kDirAnnualResult,
        kDirReference, kDirRecycleBin, kDirOtherHandler
    };
    for (const QString &dirName : rootDirs) {
        if (!QDir(PathUtil::join(workDir, dirName)).exists()) {
            return false;
        }
    }
    return true;
}

QString FileService::getWorkDir() const
{
    SettingsDao settingsDao(DatabaseManager::instance().database());
    return settingsDao.getValue(QStringLiteral("work_dir"));
}

bool FileService::setWorkDir(const QString &workDir)
{
    SettingsDao settingsDao(DatabaseManager::instance().database());
    return settingsDao.setValue(QStringLiteral("work_dir"), workDir);
}

// ============================================================================
// 目录命名
// ============================================================================

QString FileService::buildTaskFolderName(const Task &task) const
{
    // 格式：编号_[分类]_任务名称
    QString category = task.category;
    if (category.isEmpty()) {
        category = QStringLiteral("查");
    }
    QString safeTitle = FileUtil::sanitizePath(task.title);
    return QStringLiteral("%1_[%2]_%3").arg(task.taskNo, category, safeTitle);
}

QString FileService::buildTaskFolderPath(const Task &task, const QString &workDir) const
{
    QString folderName = buildTaskFolderName(task);
    QString rootDir = rootDirNameForStatus(task.status);
    return PathUtil::join(PathUtil::join(workDir, rootDir), folderName);
}

QString FileService::buildRefFolderName(const ReferenceMaterial &ref) const
{
    // 格式：编号_资料名称（无分类）
    QString safeName = FileUtil::sanitizePath(ref.name);
    return QStringLiteral("%1_%2").arg(ref.refNo, safeName);
}

QString FileService::buildRefFolderPath(const ReferenceMaterial &ref, const QString &workDir) const
{
    QString folderName = buildRefFolderName(ref);
    return PathUtil::join(PathUtil::join(workDir, kDirReference), folderName);
}

// ============================================================================
// 目录创建
// ============================================================================

bool FileService::createTaskFolder(const Task &task)
{
    QString workDir = getWorkDir();
    if (workDir.isEmpty()) {
        Logger::instance().error(QStringLiteral("FileService::createTaskFolder 工作目录未设置"));
        return false;
    }

    // 确定根目录（根据状态）
    QString rootDir = rootDirNameForStatus(task.status);
    QString folderName = buildTaskFolderName(task);
    QString folderPath = PathUtil::join(PathUtil::join(workDir, rootDir), folderName);

    // 路径长度检查
    if (PathUtil::isPathTooLong(folderPath)) {
        Logger::instance().error(QStringLiteral("FileService::createTaskFolder 路径过长: %1").arg(folderPath));
        return false;
    }

    // 创建目录
    if (!FileUtil::ensureDir(folderPath)) {
        Logger::instance().error(QStringLiteral("FileService::createTaskFolder 创建目录失败: %1").arg(folderPath));
        return false;
    }

    // 创建子目录（has_subdirs=1 时）
    if (task.hasSubdirs) {
        QStringList subDirs = {
            QStringLiteral("01_原始资料"),
            QStringLiteral("02_工作底稿"),
            QStringLiteral("03_反馈成果")
        };
        for (const QString &subDir : subDirs) {
            QString subPath = PathUtil::join(folderPath, subDir);
            if (!FileUtil::ensureDir(subPath)) {
                Logger::instance().warn(QStringLiteral("FileService::createTaskFolder 创建子目录失败: %1").arg(subPath));
            }
        }
    }

    Logger::instance().info(QStringLiteral("FileService::createTaskFolder 成功: %1").arg(folderPath));
    return true;
}

bool FileService::createRefFolder(const ReferenceMaterial &ref)
{
    QString workDir = getWorkDir();
    if (workDir.isEmpty()) {
        Logger::instance().error(QStringLiteral("FileService::createRefFolder 工作目录未设置"));
        return false;
    }

    QString folderPath = buildRefFolderPath(ref, workDir);
    if (PathUtil::isPathTooLong(folderPath)) {
        Logger::instance().error(QStringLiteral("FileService::createRefFolder 路径过长: %1").arg(folderPath));
        return false;
    }

    if (!FileUtil::ensureDir(folderPath)) {
        Logger::instance().error(QStringLiteral("FileService::createRefFolder 创建目录失败: %1").arg(folderPath));
        return false;
    }

    return true;
}

// ============================================================================
// 状态变更目录移动
// ============================================================================

bool FileService::archiveTaskFolder(const Task &task)
{
    QString workDir = getWorkDir();
    if (workDir.isEmpty()) {
        return false;
    }

    // 源路径：01_工作区_进行中/{folderName}
    QString srcPath = PathUtil::join(PathUtil::join(workDir, kDirInProgress), task.folderName);
    if (!QDir(srcPath).exists()) {
        Logger::instance().warn(QStringLiteral("FileService::archiveTaskFolder 源目录不存在: %1").arg(srcPath));
        return false;
    }

    // 目标路径：02_历史归档/年/月/{folderName}
    QDateTime archiveTime = task.finishTime.isValid() ? task.finishTime : QDateTime::currentDateTime();
    QString dstBase = buildArchivePath(workDir, archiveTime);
    QString dstPath = PathUtil::join(dstBase, task.folderName);

    if (!FileUtil::ensureDir(dstBase)) {
        return false;
    }

    if (!FileUtil::moveDir(srcPath, dstPath)) {
        Logger::instance().error(QStringLiteral("FileService::archiveTaskFolder 移动失败: %1 → %2").arg(srcPath, dstPath));
        return false;
    }

    Logger::instance().info(QStringLiteral("FileService::archiveTaskFolder 成功: %1 → %2").arg(srcPath, dstPath));
    return true;
}

bool FileService::moveTaskFolderForStatusChange(Task &task, const QString &oldStatus)
{
    if (task.folderName.isEmpty() && task.folderPath.isEmpty()) return true;
    QString workDir = getWorkDir();
    if (workDir.isEmpty()) {
        return task.folderPath.isEmpty();
    }

    // 确定源目录根名
    QString srcRootName = rootDirNameForStatus(oldStatus);

    // 确定目标路径
    QString dstPath;
    QString newStatus = task.status;

    if (newStatus == QStringLiteral("已完成")) {
        // 完成：01→02/年/月
        QDateTime archiveTime = task.finishTime.isValid() ? task.finishTime : QDateTime::currentDateTime();
        QString dstBase = buildArchivePath(workDir, archiveTime);
        if (!FileUtil::ensureDir(dstBase)) {
            return false;
        }
        dstPath = PathUtil::join(dstBase, task.folderName);
    } else if (newStatus == QStringLiteral("进行中") && oldStatus == QStringLiteral("已完成")) {
        // 取消完成：02/年/月→01
        dstPath = PathUtil::join(PathUtil::join(workDir, kDirInProgress), task.folderName);
    } else if (newStatus == QStringLiteral("已删除")) {
        // 删除：01/02/06→05
        dstPath = PathUtil::join(PathUtil::join(workDir, kDirRecycleBin), task.folderName);
    } else if (oldStatus == QStringLiteral("已删除")) {
        // 恢复：05→按 task.status 决定目标（restoreTask 已将 status 设为原状态）
        QString restoreStatus = task.status;
        QString dstRootName = rootDirNameForStatus(restoreStatus);
        if (restoreStatus == QStringLiteral("已完成")) {
            // 已完成的恢复仍放回归档区
            QDateTime archiveTime = task.finishTime.isValid() ? task.finishTime : QDateTime::currentDateTime();
            QString dstBase = buildArchivePath(workDir, archiveTime);
            FileUtil::ensureDir(dstBase);
            dstPath = PathUtil::join(dstBase, task.folderName);
        } else {
            dstPath = PathUtil::join(PathUtil::join(workDir, dstRootName), task.folderName);
        }
    } else {
        Logger::instance().warn(QStringLiteral("FileService::moveTaskFolderForStatusChange 无需移动: old=%1 new=%2").arg(oldStatus, newStatus));
        return true;
    }

    // 源路径：需要扫描定位（归档区可能在年/月子目录下）
    QString srcPath;
    const QString expectedRoot = QDir::cleanPath(PathUtil::join(workDir, srcRootName)) + '/';
    if (!task.folderPath.isEmpty()
        && QDir::cleanPath(task.folderPath).startsWith(expectedRoot, Qt::CaseInsensitive)
        && QFileInfo(task.folderPath).fileName() == task.folderName
        && QDir(task.folderPath).exists()) {
        srcPath = task.folderPath;
    } else if (oldStatus == QStringLiteral("已完成")) {
        // 归档区：扫描 02_历史归档 下所有年/月目录
        srcPath = scanForFolderName(PathUtil::join(workDir, kDirArchived), task.folderName);
    } else {
        srcPath = PathUtil::join(PathUtil::join(workDir, srcRootName), task.folderName);
    }

    if (srcPath.isEmpty() || !QDir(srcPath).exists()) {
        Logger::instance().warn(QStringLiteral("FileService::moveTaskFolderForStatusChange 源目录不存在: %1").arg(task.folderName));
        return false;
    }

    if (copyPending(srcPath) || QFileInfo::exists(dstPath)
        || !FileUtil::ensureDir(QFileInfo(dstPath).absolutePath())
        || !QDir().rename(srcPath, dstPath)) {
        Logger::instance().error(QStringLiteral("FileService::moveTaskFolderForStatusChange 移动失败: %1 → %2").arg(srcPath, dstPath));
        return false;
    }

    // 更新 folderPath
    task.folderPath = dstPath;

    Logger::instance().info(QStringLiteral("FileService::moveTaskFolderForStatusChange 成功: %1 → %2").arg(srcPath, dstPath));
    return true;
}

bool FileService::moveTaskFolderForHandlerChange(Task &task, const QString &targetDirName)
{
    QString workDir = getWorkDir();
    if (workDir.isEmpty()) {
        return false;
    }

    // 源路径：办理人变更时 01↔06 互移，根据 targetDirName 反推源目录
    QString srcRootName;
    if (targetDirName == kDirOtherHandler) {
        srcRootName = kDirInProgress;       // 01 → 06
    } else {
        srcRootName = kDirOtherHandler;     // 06 → 01
    }
    QString srcPath = PathUtil::join(PathUtil::join(workDir, srcRootName), task.folderName);

    // 目标路径
    QString dstPath = PathUtil::join(PathUtil::join(workDir, targetDirName), task.folderName);

    if (!QDir(srcPath).exists()) {
        Logger::instance().warn(QStringLiteral("FileService::moveTaskFolderForHandlerChange 源目录不存在: %1").arg(srcPath));
        return false;
    }

    if (copyPending(srcPath) || QFileInfo::exists(dstPath)
        || !FileUtil::ensureDir(QFileInfo(dstPath).absolutePath())
        || !QDir().rename(srcPath, dstPath)) {
        Logger::instance().error(QStringLiteral("FileService::moveTaskFolderForHandlerChange 移动失败: %1 → %2").arg(srcPath, dstPath));
        return false;
    }

    task.folderPath = dstPath;

    Logger::instance().info(QStringLiteral("FileService::moveTaskFolderForHandlerChange 成功: %1 → %2").arg(srcPath, dstPath));
    return true;
}

bool FileService::renameTaskFolder(Task &task)
{
    const QString newName = buildTaskFolderName(task);
    if (task.folderPath.isEmpty()) {
        task.folderName = newName;
        return true;
    }
    const QString oldPath = task.folderPath;
    const QString newPath = QDir(QFileInfo(oldPath).absolutePath()).filePath(newName);
    if (QDir::cleanPath(oldPath) != QDir::cleanPath(newPath)) {
        if (!QDir(oldPath).exists() || copyPending(oldPath) || QFileInfo::exists(newPath)
            || !QDir().rename(oldPath, newPath)) {
            return false;
        }
        task.folderPath = newPath;
    }
    task.folderName = newName;
    return true;
}

// ============================================================================
// 目录删除
// ============================================================================

bool FileService::deleteTaskFolder(const Task &task)
{
    if (task.folderPath.isEmpty()) {
        Logger::instance().warn(QStringLiteral("FileService::deleteTaskFolder folderPath 为空: id=%1").arg(task.id));
        return false;
    }

    QDir dir(task.folderPath);
    if (!dir.exists()) {
        Logger::instance().warn(QStringLiteral("FileService::deleteTaskFolder 目录不存在: %1").arg(task.folderPath));
        return false;
    }

    if (!FileUtil::removeDir(task.folderPath)) {
        Logger::instance().error(QStringLiteral("FileService::deleteTaskFolder 删除失败: %1").arg(task.folderPath));
        return false;
    }

    Logger::instance().info(QStringLiteral("FileService::deleteTaskFolder 成功: %1").arg(task.folderPath));
    return true;
}

// ============================================================================
// 自愈
// ============================================================================

bool FileService::tryAutoRepair(const Task &task)
{
    if (task.folderName.isEmpty()) {
        return false;
    }

    QString workDir = getWorkDir();
    if (workDir.isEmpty()) {
        return false;
    }

    // 扫描所有可能的目录
    QStringList scanDirs = {
        PathUtil::join(workDir, kDirInProgress),
        PathUtil::join(workDir, kDirArchived),
        PathUtil::join(workDir, kDirOtherHandler),
        PathUtil::join(workDir, kDirRecycleBin)
    };

    for (const QString &scanDir : scanDirs) {
        QString found = scanForFolderName(scanDir, task.folderName);
        if (!found.isEmpty()) {
            // 更新数据库路径
            TaskDao taskDao(DatabaseManager::instance().database());
            Task updated = task;
            updated.folderPath = found;
            taskDao.update(updated);
            Logger::instance().info(QStringLiteral("FileService::tryAutoRepair 自愈成功: %1 → %2").arg(task.folderName, found));
            return true;
        }
    }

    Logger::instance().warn(QStringLiteral("FileService::tryAutoRepair 未找到匹配目录: %1").arg(task.folderName));
    return false;
}

// ============================================================================
// 工作目录迁移
// ============================================================================

bool FileService::batchUpdatePaths(const QString &oldRoot, const QString &newRoot)
{
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("UPDATE task SET folder_path = REPLACE(folder_path, ?, ?) WHERE folder_path LIKE ?");
    query.addBindValue(oldRoot);
    query.addBindValue(newRoot);
    query.addBindValue(oldRoot + "%");
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("FileService::batchUpdatePaths 失败: %1").arg(query.lastError().text()));
        return false;
    }

    // 同步更新参考资料
    QSqlQuery refQuery(DatabaseManager::instance().database());
    refQuery.prepare("UPDATE reference_material SET folder_path = REPLACE(folder_path, ?, ?) WHERE folder_path LIKE ?");
    refQuery.addBindValue(oldRoot);
    refQuery.addBindValue(newRoot);
    refQuery.addBindValue(oldRoot + "%");
    refQuery.exec();

    return true;
}

FileService::MigrationReport FileService::migrateWorkDir(const QString &oldPath, const QString &newPath)
{
    MigrationReport report;

    // 物理移动文件
    if (!FileUtil::moveDir(oldPath, newPath)) {
        report.failCount = 1;
        report.failedItems.append(QStringLiteral("物理移动失败: %1 → %2").arg(oldPath, newPath));
        return report;
    }

    // 批量更新数据库路径
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("SELECT id, folder_path FROM task WHERE folder_path IS NOT NULL AND folder_path != ''");
    if (!query.exec()) {
        report.failCount = 1;
        report.failedItems.append(QStringLiteral("查询任务失败: %1").arg(query.lastError().text()));
        return report;
    }

    QList<QPair<int, QString>> tasks;
    while (query.next()) {
        int id = query.value(0).toInt();
        QString path = query.value(1).toString();
        if (path.startsWith(oldPath)) {
            tasks.append(qMakePair(id, path));
        }
    }

    report.totalRecords = tasks.size();
    for (const auto &item : tasks) {
        QString newPathForTask = newPath + item.second.mid(oldPath.length());
        QSqlQuery updateQuery(DatabaseManager::instance().database());
        updateQuery.prepare("UPDATE task SET folder_path = ? WHERE id = ?");
        updateQuery.addBindValue(newPathForTask);
        updateQuery.addBindValue(item.first);
        if (updateQuery.exec()) {
            report.successCount++;
        } else {
            report.failCount++;
            report.failedItems.append(QStringLiteral("任务 id=%1 路径更新失败").arg(item.first));
        }
    }

    // 更新 settings
    setWorkDir(newPath);

    Logger::instance().info(QStringLiteral("FileService::migrateWorkDir 完成: 总计=%1 成功=%2 失败=%3")
                                .arg(report.totalRecords).arg(report.successCount).arg(report.failCount));
    return report;
}

// ============================================================================
// 文件复制
// ============================================================================

bool FileService::copyFilesToTaskDir(const QStringList &sourceFiles, const QString &taskDir)
{
    if (!FileUtil::ensureDir(taskDir)) {
        return false;
    }

    bool allOk = true;
    for (const QString &srcFile : sourceFiles) {
        QFileInfo info(srcFile);
        QString dstFile = PathUtil::join(taskDir, info.fileName());
        if (!FileUtil::copyFile(srcFile, dstFile)) {
            Logger::instance().error(QStringLiteral("FileService::copyFilesToTaskDir 复制失败: %1 → %2").arg(srcFile, dstFile));
            allOk = false;
        }
    }
    return allOk;
}

QFuture<bool> FileService::copyFilesToTaskDirAsync(const QStringList &sourceFiles, const QString &taskDir)
{
    const QString key = QDir::cleanPath(taskDir);
    {
        QMutexLocker lock(&copyMutex());
        ++pendingCopies()[key];
    }
    return QtConcurrent::run([sourceFiles, taskDir]() -> bool {
        const bool result = copyFilesToTaskDir(sourceFiles, taskDir);
        QMutexLocker lock(&copyMutex());
        const QString key = QDir::cleanPath(taskDir);
        if (--pendingCopies()[key] == 0) pendingCopies().remove(key);
        return result;
    });
}

// ============================================================================
// 私有辅助方法
// ============================================================================

QString FileService::rootDirNameForStatus(const QString &status) const
{
    if (status == QStringLiteral("进行中")) {
        return kDirInProgress;
    } else if (status == QStringLiteral("已完成")) {
        return kDirArchived;
    } else if (status == QStringLiteral("已删除")) {
        return kDirRecycleBin;
    } else if (status == QStringLiteral("他人办理")) {
        return kDirOtherHandler;
    }
    return kDirInProgress;
}

QString FileService::buildArchivePath(const QString &workDir, const QDateTime &dateTime) const
{
    // 02_历史归档/年/月
    QString year = QString::number(dateTime.date().year());
    QString month = QString::number(dateTime.date().month()).rightJustified(2, '0');
    return PathUtil::join(PathUtil::join(PathUtil::join(workDir, kDirArchived), year), month);
}

QString FileService::scanForFolderName(const QString &rootDir, const QString &folderName) const
{
    if (rootDir.isEmpty() || folderName.isEmpty()) {
        return QString();
    }

    // 先在当前层查找
    QString directPath = PathUtil::join(rootDir, folderName);
    if (QDir(directPath).exists()) {
        return directPath;
    }

    // 递归查找（用于归档区的年/月结构）
    QDirIterator it(rootDir, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        if (it.fileName() == folderName) {
            return it.filePath();
        }
    }

    return QString();
}
