/**
 * @file ScanMatchService.cpp
 * @brief 路径校对与扫描导入服务实现
 *
 * 依赖：database/TaskDao、database/ReferenceMaterialDao、database/DatabaseManager、
 *       models/Task、models/ReferenceMaterial、service/TaskService（TaskResult）、
 *       service/FileService（根目录常量）、utils/PathUtil、utils/Logger
 * 文档：task.md T8.6、TD 4.4、PRD 11.3.3/13.3
 */

#include "ScanMatchService.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QMap>
#include <QDate>
#include <QFile>
#include <QSqlError>
#include <QSqlQuery>

#include "DatabaseManager.h"
#include "FileService.h"
#include "ReferenceMaterialDao.h"
#include "TaskDao.h"
#include "TaskInfoFile.h"
#include "ReferenceInfoFile.h"
#include "models/ReferenceMaterial.h"
#include "models/Task.h"
#include "models/TaskFilter.h"
#include "utils/DateUtil.h"
#include "utils/Logger.h"
#include "utils/PathUtil.h"

namespace {
void normalizeUnknownOtherHandler(Task &task)
{
    if ((task.status == QStringLiteral("他人办理")
         || task.originalStatus == QStringLiteral("他人办理"))
        && (task.handler.trimmed().isEmpty() || task.handler == QStringLiteral("本人"))) {
        task.handler = QStringLiteral("非本人");
    }
}

void applyRecoveredLocation(Task &task, const QString &sourceDir)
{
    if (sourceDir == FileService::kDirArchived) {
        task.status = QStringLiteral("已完成");
        if (!task.finishTime.isValid()) task.finishTime = QDateTime::currentDateTime();
    } else if (sourceDir == FileService::kDirOtherHandler) {
        task.status = QStringLiteral("他人办理");
    } else if (sourceDir == FileService::kDirRecycleBin) {
        task.status = QStringLiteral("已删除");
        if (task.originalStatus != QStringLiteral("进行中")
            && task.originalStatus != QStringLiteral("已完成")
            && task.originalStatus != QStringLiteral("他人办理")) {
            task.originalStatus = task.handler.trimmed().isEmpty()
                || task.handler == QStringLiteral("本人")
                ? QStringLiteral("进行中") : QStringLiteral("他人办理");
        }
        // Give a directory recovered from backup a full retention period.
        task.deleteTime = QDateTime::currentDateTime();
    } else {
        task.status = QStringLiteral("进行中");
        task.handler = QStringLiteral("本人");
    }
    if (task.handler.trimmed().isEmpty()) task.handler = QStringLiteral("本人");
    normalizeUnknownOtherHandler(task);
}
}

// ============================================================================
// 编号解析辅助
// ============================================================================

QString ScanMatchService::extractTaskNo(const QString &folderName) const
{
    if (folderName.isEmpty()) {
        return QString();
    }
    int idx = folderName.indexOf('_');
    if (idx <= 0) {
        return QString();
    }
    QString taskNo = folderName.left(idx);
    if (!isValidTaskNo(taskNo)) {
        return QString();
    }
    return taskNo;
}

bool ScanMatchService::isValidTaskNo(const QString &taskNo) const
{
    // 格式：YYYYMMDD-NNN（8 位数字 + 连字符 + 3 位数字 = 12 字符）
    if (taskNo.length() != 12) {
        return false;
    }
    if (taskNo.at(8) != '-') {
        return false;
    }
    for (int i = 0; i < 12; ++i) {
        if (i == 8) {
            continue;
        }
        if (!taskNo.at(i).isDigit()) {
            return false;
        }
    }
    return QDate::fromString(taskNo.left(8), QStringLiteral("yyyyMMdd")).isValid()
        && taskNo.right(3).toInt() > 0;
}

bool ScanMatchService::parseTaskFolderName(const QString &folderName, QString &taskNo,
                                           QString &category, QString &name) const
{
    if (folderName.isEmpty()) {
        return false;
    }

    QStringList parts = folderName.split('_');
    if (parts.size() < 2) {
        return false;
    }
    if (parts.size() == 2 && parts.at(1).startsWith('[')
        && parts.at(1).endsWith(']')) return false;

    // 第一段为编号
    taskNo = parts.at(0);
    if (!isValidTaskNo(taskNo)) {
        return false;
    }

    // 第二段若以 [ 开头、] 结尾则为分类
    if (parts.size() >= 3 && parts.at(1).startsWith('[') && parts.at(1).endsWith(']')) {
        category = parts.at(1).mid(1, parts.at(1).length() - 2);
        name = parts.mid(2).join('_');
    } else {
        // 无分类段，名称从第二段开始
        category.clear();
        name = parts.mid(1).join('_');
    }

    return !name.isEmpty();
}

bool ScanMatchService::parseRefFolderName(const QString &folderName, QString &refNo,
                                          QString &name) const
{
    if (folderName.isEmpty()) {
        return false;
    }

    QStringList parts = folderName.split('_');
    if (parts.size() < 2) {
        return false;
    }

    refNo = parts.at(0);
    if (!isValidTaskNo(refNo)) {
        return false;
    }

    name = parts.mid(1).join('_');
    return !name.isEmpty();
}

QStringList ScanMatchService::scanFolders(const QString &rootDir) const
{
    QStringList result;
    if (rootDir.isEmpty() || !QDir(rootDir).exists()) {
        return result;
    }

    // 任务目录只会位于工作区根层；附件子目录绝不能当成另一项任务。
    const QFileInfoList folders = QDir(rootDir).entryInfoList(
        QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks, QDir::Name);
    for (const QFileInfo &folder : folders) {
        if (isValidTaskNo(folder.fileName().section('_', 0, 0))) {
            result.append(folder.absoluteFilePath());
        }
    }
    return result;
}

QStringList ScanMatchService::scanArchivedFolders(const QString &rootDir) const
{
    QStringList result = scanFolders(rootDir); // Support older flat archives.
    const QFileInfoList years = QDir(rootDir).entryInfoList(
        QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks, QDir::Name);
    for (const QFileInfo &year : years) {
        bool validYear = false;
        const int yearNumber = year.fileName().toInt(&validYear);
        if (!validYear || year.fileName().size() != 4
            || !QDate(yearNumber, 1, 1).isValid()) continue;
        const QFileInfoList months = QDir(year.absoluteFilePath()).entryInfoList(
            QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks, QDir::Name);
        for (const QFileInfo &month : months) {
            bool validMonth = false;
            const int monthNumber = month.fileName().toInt(&validMonth);
            if (!validMonth || month.fileName().size() != 2
                || monthNumber < 1 || monthNumber > 12) continue;
            result.append(scanFolders(month.absoluteFilePath()));
        }
    }
    return result;
}

// ============================================================================
// 路径校对
// ============================================================================

QList<ScanResult> ScanMatchService::scanAndRepairAll(const QString &workDir)
{
    QList<ScanResult> results;
    if (workDir.isEmpty()) {
        return results;
    }

    // 1. 读取数据库全部任务（含回收站）和参考资料
    TaskDao taskDao(DatabaseManager::instance().database());
    TaskFilter filter;
    filter.pageSize = 0;  // 不分页，获取全部
    QList<Task> tasks = taskDao.findByFilter(filter);
    tasks.append(taskDao.findDeleted());

    ReferenceMaterialDao refDao(DatabaseManager::instance().database());
    QList<ReferenceMaterial> refs = refDao.findAll();

    // 构建 编号 → 记录 的映射
    QMap<QString, Task> taskMap;
    for (const Task &task : tasks) {
        if (!task.taskNo.isEmpty()) {
            taskMap.insert(task.taskNo, task);
        }
    }
    QMap<QString, ReferenceMaterial> refMap;
    for (const ReferenceMaterial &ref : refs) {
        if (!ref.refNo.isEmpty()) {
            refMap.insert(ref.refNo, ref);
        }
    }

    // 2. 遍历任务/资料根目录；归档任务在年/月子目录。
    QStringList scanDirs = {
        PathUtil::join(workDir, FileService::kDirInProgress),
        PathUtil::join(workDir, FileService::kDirArchived),
        PathUtil::join(workDir, FileService::kDirReference),
        PathUtil::join(workDir, FileService::kDirRecycleBin),
        PathUtil::join(workDir, FileService::kDirOtherHandler)
    };

    for (const QString &scanDir : scanDirs) {
        const QStringList folders = scanDir.endsWith(FileService::kDirArchived)
            ? scanArchivedFolders(scanDir) : scanFolders(scanDir);
        for (const QString &folderPath : folders) {
            QString folderName = QFileInfo(folderPath).fileName();
            QString taskNo = extractTaskNo(folderName);

            ScanResult result;
            result.folderName = folderName;
            result.taskNo = taskNo;
            result.actualPath = folderPath;

            if (taskNo.isEmpty()) {
                result.matched = false;
                result.errorMessage = QStringLiteral("无法解析编号");
                results.append(result);
                continue;
            }

            // 判断是任务还是参考资料（04_参考资料目录下为参考资料）
            bool isReference = scanDir.endsWith(FileService::kDirReference);

            if (isReference) {
                auto it = refMap.find(taskNo);
                if (it == refMap.end()) {
                    result.matched = false;
                    result.errorMessage = QStringLiteral("参考资料编号未入库: %1").arg(taskNo);
                    results.append(result);
                    continue;
                }
                result.taskId = it->id;
                result.expectedPath = it->folderPath;

                // 比对路径
                QString dbPath = PathUtil::normalize(it->folderPath);
                QString actualPath = PathUtil::normalize(folderPath);
                if (dbPath == actualPath) {
                    result.matched = true;
                } else {
                    // 路径不一致，更新数据库
                    ReferenceMaterial ref = it.value();
                    ref.folderPath = folderPath;
                    if (refDao.update(ref)) {
                        result.matched = true;
                        Logger::instance().info(
                            QStringLiteral("ScanMatchService: 修复参考资料路径: %1 → %2")
                                .arg(dbPath, actualPath));
                    } else {
                        result.matched = false;
                        result.errorMessage = QStringLiteral("更新参考资料路径失败");
                    }
                }
            } else {
                auto it = taskMap.find(taskNo);
                if (it == taskMap.end()) {
                    result.matched = false;
                    result.errorMessage = QStringLiteral("任务编号未入库: %1").arg(taskNo);
                    results.append(result);
                    continue;
                }
                result.taskId = it->id;
                result.expectedPath = it->folderPath;

                // 比对路径
                QString dbPath = PathUtil::normalize(it->folderPath);
                QString actualPath = PathUtil::normalize(folderPath);
                if (dbPath == actualPath) {
                    result.matched = true;
                } else {
                    // 路径不一致，更新数据库
                    Task task = it.value();
                    task.folderPath = folderPath;
                    if (taskDao.update(task)) {
                        result.matched = true;
                        Logger::instance().info(
                            QStringLiteral("ScanMatchService: 修复任务路径: %1 → %2")
                                .arg(dbPath, actualPath));
                    } else {
                        result.matched = false;
                        result.errorMessage = QStringLiteral("更新任务路径失败");
                    }
                }
            }

            results.append(result);
        }
    }

    Logger::instance().info(
        QStringLiteral("ScanMatchService::scanAndRepairAll 完成: 共扫描 %1 项").arg(results.size()));

    return results;
}

bool ScanMatchService::manualRepairPath(int taskId, const QString &newPath)
{
    if (newPath.isEmpty()) {
        return false;
    }

    TaskDao taskDao(DatabaseManager::instance().database());
    auto opt = taskDao.findById(taskId);
    if (!opt) {
        Logger::instance().warn(
            QStringLiteral("ScanMatchService::manualRepairPath 任务不存在: id=%1").arg(taskId));
        return false;
    }

    Task task = *opt;
    task.folderPath = newPath;
    return taskDao.update(task);
}

bool ScanMatchService::manualRepairRefPath(int refId, const QString &newPath)
{
    if (newPath.isEmpty()) {
        return false;
    }

    ReferenceMaterialDao refDao(DatabaseManager::instance().database());
    auto opt = refDao.findById(refId);
    if (!opt) {
        Logger::instance().warn(
            QStringLiteral("ScanMatchService::manualRepairRefPath 参考资料不存在: id=%1").arg(refId));
        return false;
    }

    ReferenceMaterial ref = *opt;
    ref.folderPath = newPath;
    return refDao.update(ref);
}

// ============================================================================
// 扫描导入
// ============================================================================

QList<ScanImportItem> ScanMatchService::scanForImport(const QString &workDir)
{
    QList<ScanImportItem> items;
    if (workDir.isEmpty()) {
        return items;
    }

    // 1. 读取数据库全部任务和参考资料编号（用于重复检测）
    TaskDao taskDao(DatabaseManager::instance().database());
    TaskFilter filter;
    filter.pageSize = 0;
    QList<Task> tasks = taskDao.findByFilter(filter);
    tasks.append(taskDao.findDeleted());

    ReferenceMaterialDao refDao(DatabaseManager::instance().database());
    QList<ReferenceMaterial> refs = refDao.findAll();

    QMap<QString, Task> taskNoMap;  // taskNo → record
    for (const Task &task : tasks) {
        if (!task.taskNo.isEmpty()) {
            taskNoMap.insert(task.taskNo, task);
        }
    }
    QMap<QString, ReferenceMaterial> refNoMap;
    for (const ReferenceMaterial &ref : refs) {
        if (!ref.refNo.isEmpty()) {
            refNoMap.insert(ref.refNo, ref);
        }
    }

    // 2. 扫描任务/资料根目录；归档任务在年/月子目录。
    struct ScanDirInfo {
        QString dirName;
        QString importType;
    };
    QList<ScanDirInfo> scanDirs = {
        {FileService::kDirInProgress, QStringLiteral("任务")},
        {FileService::kDirArchived,   QStringLiteral("任务")},
        {FileService::kDirReference,  QStringLiteral("参考资料")},
        {FileService::kDirRecycleBin, QStringLiteral("任务")},
        {FileService::kDirOtherHandler, QStringLiteral("任务")}
    };

    for (const ScanDirInfo &info : scanDirs) {
        QString scanDir = PathUtil::join(workDir, info.dirName);
        const QStringList folders = info.dirName == FileService::kDirArchived
            ? scanArchivedFolders(scanDir) : scanFolders(scanDir);

        for (const QString &folderPath : folders) {
            QString folderName = QFileInfo(folderPath).fileName();

            ScanImportItem item;
            item.folderName = folderName;
            item.folderPath = folderPath;
            item.importType = info.importType;
            item.sourceDir = info.dirName;

            // 解析目录名
            if (info.importType == QStringLiteral("参考资料")) {
                QString refNo;
                QString name;
                if (!parseRefFolderName(folderName, refNo, name)) {
                    item.status = QStringLiteral("解析失败");
                    items.append(item);
                    continue;
                }
                item.taskNo = refNo;
                item.name = name;

                // 重复检测
                auto it = refNoMap.find(refNo);
                if (it != refNoMap.end()) {
                    item.isDuplicate = true;
                    item.existingId = it.value().id;
                    if (QDir::cleanPath(it.value().folderPath) == QDir::cleanPath(folderPath)) {
                        item.status = QStringLiteral("跳过-已存在");
                    } else {
                        item.isConflict = true;
                        item.conflictReason = QStringLiteral("同编号资料已位于另一目录");
                        item.status = QStringLiteral("待解决冲突");
                    }
                } else {
                    item.status = QStringLiteral("待导入");
                }
                const auto saved = ReferenceInfoFile::read(folderPath);
                if (QFileInfo::exists(ReferenceInfoFile::path(folderPath)) && !saved) {
                    item.isConflict = true;
                    item.conflictReason = QStringLiteral("资料信息文件无效");
                } else if (saved && (saved->refNo != refNo || saved->name != name)) {
                    const bool matchesKnownRecord = it != refNoMap.end()
                        && QDir::cleanPath(it.value().folderPath) == QDir::cleanPath(folderPath)
                        && saved->refNo == it.value().refNo && saved->name == it.value().name;
                    if (!matchesKnownRecord) {
                        item.isConflict = true;
                        item.conflictReason = QStringLiteral("目录名与资料信息文件不一致");
                    }
                }
                if (item.isConflict) item.status = QStringLiteral("待解决冲突");
            } else {
                QString taskNo;
                QString category;
                QString name;
                if (!parseTaskFolderName(folderName, taskNo, category, name)) {
                    item.status = QStringLiteral("解析失败");
                    items.append(item);
                    continue;
                }
                item.taskNo = taskNo;
                item.name = name;
                item.category = category;

                // 重复检测
                auto it = taskNoMap.find(taskNo);
                if (it != taskNoMap.end()) {
                    item.isDuplicate = true;
                    item.existingId = it.value().id;
                    if (QDir::cleanPath(it.value().folderPath)
                        == QDir::cleanPath(folderPath)) {
                        item.status = QStringLiteral("跳过-已存在");
                    } else {
                        item.isConflict = true;
                        item.conflictReason = QStringLiteral("同编号任务已位于另一目录");
                        item.status = QStringLiteral("待解决冲突");
                    }
                } else {
                    item.status = QStringLiteral("待导入");
                }
                const auto saved = TaskInfoFile::read(folderPath);
                if (QFileInfo::exists(TaskInfoFile::path(folderPath)) && !saved) {
                    item.isConflict = true;
                    item.conflictReason = QStringLiteral("任务信息文件无效");
                } else if (saved && (saved->taskNo != taskNo
                                      || saved->title != name
                                      || saved->category != category)) {
                    const bool matchesKnownRecord = it != taskNoMap.end()
                        && QDir::cleanPath(it.value().folderPath) == QDir::cleanPath(folderPath)
                        && saved->taskNo == it.value().taskNo
                        && saved->title == it.value().title
                        && saved->category == it.value().category;
                    if (!matchesKnownRecord) {
                        item.isConflict = true;
                        item.conflictReason = QStringLiteral("目录名与任务信息文件不一致");
                    }
                }
                if (item.isConflict) item.status = QStringLiteral("待解决冲突");
            }

            if (item.status != QStringLiteral("跳过-已存在")) items.append(item);
        }
    }

    // 两台电脑合并后，数据库里可能尚无此编号，但磁盘上已有两份。
    QMap<QString, int> seenTaskNos;
    QMap<QString, int> seenRefNos;
    for (const ScanImportItem &item : items) {
        if (item.importType == QStringLiteral("任务") && !item.taskNo.isEmpty())
            ++seenTaskNos[item.taskNo];
        if (item.importType == QStringLiteral("参考资料") && !item.taskNo.isEmpty())
            ++seenRefNos[item.taskNo];
    }
    for (ScanImportItem &item : items) {
        if (item.importType == QStringLiteral("任务") && !item.isDuplicate
            && seenTaskNos.value(item.taskNo) > 1) {
            item.isConflict = true;
            item.conflictReason = QStringLiteral("多个工作区中存在同编号任务");
            item.status = QStringLiteral("待解决冲突");
        }
        if (item.importType == QStringLiteral("参考资料") && !item.isDuplicate
            && seenRefNos.value(item.taskNo) > 1) {
            item.isConflict = true;
            item.conflictReason = QStringLiteral("多个目录中存在同编号资料");
            item.status = QStringLiteral("待解决冲突");
        }
    }

    Logger::instance().info(
        QStringLiteral("ScanMatchService::scanForImport 完成: 共扫描 %1 项").arg(items.size()));

    return items;
}

TaskResult ScanMatchService::importScanItem(const ScanImportItem &item, const QString &workDir)
{
    Q_UNUSED(workDir);

    TaskResult result;
    result.ok = false;

    // 跳过已存在或解析失败的项
    if (item.isConflict) {
        result.error = QStringLiteral("目录存在冲突，请先逐项解决");
        return result;
    }
    if (item.isDuplicate) {
        result.error = QStringLiteral("该项已存在，跳过导入");
        return result;
    }
    if (item.status == QStringLiteral("解析失败")) {
        result.error = QStringLiteral("目录名解析失败，无法导入");
        return result;
    }
    if (item.taskNo.isEmpty() || item.name.isEmpty()) {
        result.error = QStringLiteral("编号或名称为空，无法导入");
        return result;
    }

    if (item.importType == QStringLiteral("参考资料")) {
        // 导入参考资料
        ReferenceMaterial ref;
        ref.refNo = item.taskNo;
        ref.name = item.name;
        ref.folderName = item.folderName;
        ref.folderPath = item.folderPath;
        const auto saved = ReferenceInfoFile::read(item.folderPath);
        if (saved && saved->refNo == item.taskNo) {
            ref.name = saved->name;
            ref.source = saved->source;
            ref.tag = saved->tag;
            ref.description = saved->description;
            ref.createTime = saved->createTime;
        }

        ReferenceMaterialDao refDao(DatabaseManager::instance().database());
        int newId = refDao.insert(ref);
        if (newId <= 0) {
            result.error = QStringLiteral("参考资料入库失败");
            Logger::instance().error(
                QStringLiteral("ScanMatchService::importScanItem 参考资料入库失败: %1").arg(item.folderName));
            return result;
        }

        result.ok = true;
        result.taskId = newId;
        ref.id = newId;
        if (!ReferenceInfoFile::write(ref))
            Logger::instance().warn(QStringLiteral("扫描导入后写入资料信息文件失败: %1")
                                        .arg(item.folderPath));
        Logger::instance().info(
            QStringLiteral("ScanMatchService::importScanItem 参考资料导入成功: %1 (id=%2)")
                .arg(item.folderName).arg(newId));
    } else {
        // 导入任务
        Task task;
        task.taskNo = item.taskNo;
        task.title = item.name;
        task.category = item.category;
        task.folderName = item.folderName;
        task.folderPath = item.folderPath;

        // Folder location is authoritative for status; the info file supplies
        // fields that cannot be reconstructed from the folder name.
        const auto saved = TaskInfoFile::read(item.folderPath);
        if (saved && saved->taskNo == item.taskNo) {
            task.title = saved->title;
            task.category = saved->category;
            task.source = saved->source;
            task.tag = saved->tag;
            task.description = saved->description;
            task.deadline = saved->deadline;
            task.isKeyTask = saved->isKeyTask;
            task.hasSubdirs = saved->hasSubdirs;
            task.createTime = saved->createTime;
            task.finishTime = saved->finishTime;
            task.originalStatus = saved->originalStatus;
            task.deleteTime = saved->deleteTime;
            task.handler = saved->handler;
        }
        // 子目录结构由磁盘决定，附件清单不写入数据库。
        task.hasSubdirs = !QDir(item.folderPath).entryList(
            QDir::Dirs | QDir::NoDotAndDotDot).isEmpty();

        // 所属工作区决定状态；无具体办理人时只为他人办理任务兜底。
        applyRecoveredLocation(task, item.sourceDir);

        TaskDao taskDao(DatabaseManager::instance().database());
        int newId = taskDao.insert(task);
        if (newId <= 0) {
            result.error = QStringLiteral("任务入库失败");
            Logger::instance().error(
                QStringLiteral("ScanMatchService::importScanItem 任务入库失败: %1").arg(item.folderName));
            return result;
        }

        result.ok = true;
        result.taskId = newId;
        task.id = newId;
        if (!TaskInfoFile::write(task))
            Logger::instance().warn(QStringLiteral("扫描导入后写入任务信息文件失败: %1")
                                        .arg(item.folderPath));
        Logger::instance().info(
            QStringLiteral("ScanMatchService::importScanItem 任务导入成功: %1 (id=%2)")
                .arg(item.folderName).arg(newId));
    }

    return result;
}

namespace {
QString chosenValue(const QString &key, const QMap<QString, QString> &sources,
                    const QString &existing, const QString &folder, const QString &info)
{
    const QString source = sources.value(key);
    if (source == QStringLiteral("info")) return info;
    if (source == QStringLiteral("folder")) return folder;
    return existing;
}

bool copyMergeFiles(const QString &source, const QString &target)
{
    QDirIterator it(source, QDir::Files | QDir::Dirs | QDir::Hidden | QDir::System
                    | QDir::NoDotAndDotDot | QDir::NoSymLinks,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString path = it.next();
        const QFileInfo info = it.fileInfo();
        const QString relative = QDir(source).relativeFilePath(path);
        if (relative == QStringLiteral(".niuma-task.json")
            || relative == QStringLiteral(".niuma-reference.json")) continue;
        QString output = QDir(target).filePath(relative);
        if (info.isDir()) {
            if (!QDir().mkpath(output)) return false;
            continue;
        }
        if (!QDir().mkpath(QFileInfo(output).absolutePath())) return false;
        if (QFileInfo::exists(output)) {
            const QFileInfo existing(output);
            int suffix = 1;
            do {
                output = QDir(existing.absolutePath()).filePath(
                    existing.completeBaseName()
                    + QStringLiteral(" (合并来源%1)").arg(suffix++)
                    + (existing.suffix().isEmpty() ? QString()
                       : QStringLiteral(".") + existing.suffix()));
            } while (QFileInfo::exists(output));
        }
        if (!QFile::copy(path, output)) return false;
    }
    return true;
}
}

TaskResult ScanMatchService::resolveConflict(const ScanImportItem &item, bool merge,
    const QMap<QString, QString> &fieldSources)
{
    TaskResult result;
    result.ok = false;
    if (!item.isConflict || item.importType != QStringLiteral("任务")
        || !QDir(item.folderPath).exists()) {
        result.error = QStringLiteral("冲突目录不存在或不属于任务");
        return result;
    }
    TaskDao dao(DatabaseManager::instance().database());
    auto existing = item.existingId > 0 ? dao.findById(item.existingId) : std::optional<Task>();
    const auto info = TaskInfoFile::read(item.folderPath);
    if (item.existingId > 0 && !existing) {
        result.error = QStringLiteral("原任务已发生变化，请重新扫描");
        return result;
    }
    if (merge && !existing) {
        const QString workDir = FileService().getWorkDir();
        for (ScanImportItem peer : scanForImport(workDir)) {
            if (peer.importType != QStringLiteral("任务") || peer.taskNo != item.taskNo
                || peer.folderPath == item.folderPath || peer.isDuplicate) continue;
            const auto peerInfo = TaskInfoFile::read(peer.folderPath);
            if (QFileInfo::exists(TaskInfoFile::path(peer.folderPath)) && !peerInfo) continue;
            if (peerInfo && (peerInfo->taskNo != peer.taskNo
                             || peerInfo->title != peer.name
                             || peerInfo->category != peer.category)) continue;
            peer.isConflict = false;
            peer.status = QStringLiteral("待导入");
            const TaskResult imported = importScanItem(peer, workDir);
            if (imported.ok) existing = dao.findById(imported.taskId);
            break;
        }
        // 没有另一目录时是目录名与 info 的元数据合并，继续逐字段建立记录。
    }
    QMap<QString, QString> resolvedSources = fieldSources;
    const QString defaultSource = existing ? QStringLiteral("existing")
        : info ? QStringLiteral("info") : QStringLiteral("folder");
    for (const QString &key : {QStringLiteral("task_no"), QStringLiteral("title"),
                               QStringLiteral("category"), QStringLiteral("source"),
                               QStringLiteral("tag"), QStringLiteral("description"),
                               QStringLiteral("handler"), QStringLiteral("deadline"),
                               QStringLiteral("is_key_task")}) {
        if (!resolvedSources.contains(key)) resolvedSources.insert(key, defaultSource);
    }
    const bool sameFolder = existing && QDir::cleanPath(existing->folderPath)
        == QDir::cleanPath(item.folderPath);
    Task task = existing && (merge || sameFolder) ? *existing : Task();
    const QString priorTitle = existing ? existing->title : item.name;
    const QString priorCategory = existing ? existing->category : item.category;
    const QString priorSource = existing ? existing->source : QString();
    const QString priorTag = existing ? existing->tag : QString();
    const QString priorDescription = existing ? existing->description : QString();
    const QString fallbackHandler = item.sourceDir == FileService::kDirOtherHandler
        ? QStringLiteral("非本人") : QStringLiteral("本人");
    const QString priorHandler = existing ? existing->handler : fallbackHandler;
    const QString priorDeadline = existing && existing->deadline.isValid()
        ? existing->deadline.toString(Qt::ISODate) : QString();
    task.title = chosenValue(QStringLiteral("title"), resolvedSources,
        priorTitle, item.name, info ? info->title : item.name);
    task.category = chosenValue(QStringLiteral("category"), resolvedSources,
        priorCategory, item.category, info ? info->category : item.category);
    task.source = chosenValue(QStringLiteral("source"), resolvedSources,
        priorSource, QString(), info ? info->source : QString());
    task.tag = chosenValue(QStringLiteral("tag"), resolvedSources,
        priorTag, QString(), info ? info->tag : QString());
    task.description = chosenValue(QStringLiteral("description"), resolvedSources,
        priorDescription, QString(), info ? info->description : QString());
    task.handler = chosenValue(QStringLiteral("handler"), resolvedSources,
        priorHandler, fallbackHandler, info ? info->handler : fallbackHandler);
    normalizeUnknownOtherHandler(task);
    task.deadline = QDate::fromString(chosenValue(QStringLiteral("deadline"), resolvedSources,
        priorDeadline, QString(), info && info->deadline.isValid()
            ? info->deadline.toString(Qt::ISODate) : QString()), Qt::ISODate);
    const QString selectedKey = chosenValue(QStringLiteral("is_key_task"), resolvedSources,
        existing && existing->isKeyTask ? QStringLiteral("1") : QStringLiteral("0"),
        QStringLiteral("0"), info && info->isKeyTask ? QStringLiteral("1") : QStringLiteral("0"));
    task.isKeyTask = selectedKey == QStringLiteral("1");
    if (task.title.trimmed().isEmpty()) {
        result.error = QStringLiteral("任务名称不能为空");
        return result;
    }

    if (sameFolder) {
        const QString selectedNumber = chosenValue(QStringLiteral("task_no"), resolvedSources,
            existing->taskNo, item.taskNo, info ? info->taskNo : item.taskNo);
        if (!isValidTaskNo(selectedNumber)
            || (selectedNumber != existing->taskNo && dao.findByTaskNo(selectedNumber))) {
            result.error = QStringLiteral("所选编号无效或已被其他任务使用");
            return result;
        }
        task.taskNo = selectedNumber;
        task.folderName = FileService().buildTaskFolderName(task);
        task.folderPath = QDir(QFileInfo(item.folderPath).absolutePath()).filePath(task.folderName);
        if (QFileInfo::exists(task.folderPath) && QDir::cleanPath(task.folderPath)
            != QDir::cleanPath(item.folderPath)) {
            result.error = QStringLiteral("目标目录名已存在");
            return result;
        }
        const bool renamed = QDir::cleanPath(task.folderPath) != QDir::cleanPath(item.folderPath);
        if (renamed && !QDir().rename(item.folderPath, task.folderPath)) {
            result.error = QStringLiteral("目录重命名失败，请关闭占用文件后重试");
            return result;
        }
        task.hasSubdirs = !QDir(task.folderPath).entryList(
            QDir::Dirs | QDir::NoDotAndDotDot).isEmpty();
        if (!dao.update(task) || !TaskInfoFile::write(task)) {
            dao.update(*existing);
            if (renamed) QDir().rename(task.folderPath, item.folderPath);
            result.error = QStringLiteral("任务信息修复失败，已尝试回退");
            return result;
        }
        result.ok = true;
        result.taskId = task.id;
        return result;
    }

    if (merge && existing) {
        task.folderName = FileService().buildTaskFolderName(task);
        const bool oldFolderExists = QDir(existing->folderPath).exists();
        const bool useScannedFolder = !oldFolderExists
            || (task.folderName == item.folderName && task.folderName != existing->folderName);
        if (useScannedFolder) {
            const QString target = QDir(QFileInfo(item.folderPath).absolutePath())
                .filePath(task.folderName);
            if (QDir::cleanPath(target) != QDir::cleanPath(item.folderPath)
                && QFileInfo::exists(target)) {
                result.error = QStringLiteral("目标任务目录名已存在");
                return result;
            }
            if (oldFolderExists && !copyMergeFiles(existing->folderPath, item.folderPath)) {
                result.error = QStringLiteral("附件合并失败，原目录仍保留");
                return result;
            }
            QString retained;
            if (oldFolderExists) {
                const QString base = QFileInfo(existing->folderPath).absolutePath()
                    + QStringLiteral("/_已合并_") + existing->folderName;
                retained = base;
                int suffix = 1;
                while (QDir(retained).exists()) retained = base + QString::number(suffix++);
                if (!QDir().rename(existing->folderPath, retained)) {
                    result.error = QStringLiteral("原任务目录留存失败，请关闭占用文件后重试");
                    return result;
                }
            }
            const bool renamed = QDir::cleanPath(target) != QDir::cleanPath(item.folderPath);
            if (renamed && !QDir().rename(item.folderPath, target)) {
                if (oldFolderExists) QDir().rename(retained, existing->folderPath);
                result.error = QStringLiteral("新任务目录重命名失败");
                return result;
            }
            task.folderPath = target;
            applyRecoveredLocation(task, item.sourceDir);
            task.hasSubdirs = !QDir(target).entryList(
                QDir::Dirs | QDir::NoDotAndDotDot).isEmpty();
            if (!dao.update(task) || !TaskInfoFile::write(task)) {
                dao.update(*existing);
                if (renamed) QDir().rename(target, item.folderPath);
                if (oldFolderExists) QDir().rename(retained, existing->folderPath);
                result.error = QStringLiteral("新任务信息写入失败，已尝试回退");
                return result;
            }
            if (oldFolderExists && !QFile::remove(TaskInfoFile::path(retained)))
                Logger::instance().warn(QStringLiteral("旧任务信息文件清理失败: %1").arg(retained));
            result.ok = true;
            result.taskId = task.id;
            return result;
        }
        if (!QDir(existing->folderPath).exists()
            || !copyMergeFiles(item.folderPath, existing->folderPath)) {
            result.error = QStringLiteral("附件合并失败，来源目录未删除，请关闭占用文件后重试");
            return result;
        }
        task.folderPath = existing->folderPath;
        if (!FileService().renameTaskFolder(task)) {
            result.error = QStringLiteral("合并后目录重命名失败，来源目录仍保留");
            return result;
        }
        task.hasSubdirs = !QDir(task.folderPath).entryList(
            QDir::Dirs | QDir::NoDotAndDotDot).isEmpty();
        if (!dao.update(task) || !TaskInfoFile::write(task)) {
            if (task.folderPath != existing->folderPath)
                QDir().rename(task.folderPath, existing->folderPath);
            dao.update(*existing);
            result.error = QStringLiteral("合并信息写入失败，来源目录仍保留");
            return result;
        }
        const QString baseRetained = QFileInfo(item.folderPath).absolutePath()
            + QStringLiteral("/_已合并_") + item.folderName;
        QString retained = baseRetained;
        int suffix = 1;
        while (QDir(retained).exists()) retained = baseRetained + QString::number(suffix++);
        if (!QDir().rename(item.folderPath, retained)) {
            result.error = QStringLiteral("任务已合并，但来源目录重命名失败，请手动处理重复目录");
            return result;
        }
        result.ok = true;
        result.taskId = task.id;
        return result;
    }

    // 保留副本时分配全新编号；单纯的目录/info 冲突优先使用所选编号。
    QString number = chosenValue(QStringLiteral("task_no"), resolvedSources,
        item.taskNo, item.taskNo, info ? info->taskNo : item.taskNo);
    if (existing || !isValidTaskNo(number) || dao.findByTaskNo(number)) {
        const QString date = QDate::currentDate().toString(QStringLiteral("yyyyMMdd"));
        for (int seq = dao.getMaxSequenceForDate(date) + 1; seq <= 999; ++seq) {
            const QString candidate = QStringLiteral("%1-%2").arg(date)
                .arg(seq, 3, 10, QLatin1Char('0'));
            if (!dao.findByTaskNo(candidate)) { number = candidate; break; }
        }
        if (dao.findByTaskNo(number) || !isValidTaskNo(number)) {
            result.error = QStringLiteral("无法分配新的任务编号");
            return result;
        }
    }
    task.taskNo = number;
    if (info) {
        task.originalStatus = info->originalStatus;
        task.deleteTime = info->deleteTime;
    }
    applyRecoveredLocation(task, item.sourceDir);
    task.folderName = FileService().buildTaskFolderName(task);
    task.folderPath = QDir(QFileInfo(item.folderPath).absolutePath()).filePath(task.folderName);
    if (QFileInfo::exists(task.folderPath) && QDir::cleanPath(task.folderPath)
        != QDir::cleanPath(item.folderPath)) {
        result.error = QStringLiteral("目标目录名已存在，请修改字段选择");
        return result;
    }
    const bool renamed = QDir::cleanPath(task.folderPath) != QDir::cleanPath(item.folderPath);
    if (renamed && !QDir().rename(item.folderPath, task.folderPath)) {
        result.error = QStringLiteral("任务目录重命名失败，请关闭占用文件后重试");
        return result;
    }
    task.hasSubdirs = !QDir(task.folderPath).entryList(QDir::Dirs | QDir::NoDotAndDotDot).isEmpty();
    if (info) task.createTime = info->createTime;
    const int id = dao.insert(task);
    if (id <= 0) {
        if (renamed) QDir().rename(task.folderPath, item.folderPath);
        result.error = QStringLiteral("任务写入数据库失败");
        return result;
    }
    task.id = id;
    if (!TaskInfoFile::write(task)) {
        dao.deleteById(id);
        if (renamed) QDir().rename(task.folderPath, item.folderPath);
        result.error = QStringLiteral("任务信息文件写入失败");
        return result;
    }
    result.ok = true;
    result.taskId = id;
    return result;
}

TaskResult ScanMatchService::resolveReferenceConflict(const ScanImportItem &item, bool merge,
    const QMap<QString, QString> &fieldSources)
{
    TaskResult result;
    result.ok = false;
    if (!item.isConflict || item.importType != QStringLiteral("参考资料")
        || !QDir(item.folderPath).exists()) {
        result.error = QStringLiteral("冲突资料目录不存在");
        return result;
    }
    ReferenceMaterialDao dao(DatabaseManager::instance().database());
    auto existing = item.existingId > 0 ? dao.findById(item.existingId)
        : std::optional<ReferenceMaterial>();
    if (item.existingId > 0 && !existing) {
        result.error = QStringLiteral("原资料已发生变化，请重新扫描");
        return result;
    }
    const auto info = ReferenceInfoFile::read(item.folderPath);
    if (merge && !existing) {
        const QString workDir = FileService().getWorkDir();
        for (ScanImportItem peer : scanForImport(workDir)) {
            if (peer.importType != QStringLiteral("参考资料") || peer.taskNo != item.taskNo
                || peer.folderPath == item.folderPath || peer.isDuplicate) continue;
            const auto peerInfo = ReferenceInfoFile::read(peer.folderPath);
            if (QFileInfo::exists(ReferenceInfoFile::path(peer.folderPath)) && !peerInfo) continue;
            if (peerInfo && (peerInfo->refNo != peer.taskNo || peerInfo->name != peer.name)) continue;
            peer.isConflict = false;
            peer.status = QStringLiteral("待导入");
            const TaskResult imported = importScanItem(peer, workDir);
            if (imported.ok) existing = dao.findById(imported.taskId);
            break;
        }
    }
    const bool sameFolder = existing && QDir::cleanPath(existing->folderPath)
        == QDir::cleanPath(item.folderPath);
    ReferenceMaterial reference = existing && (merge || sameFolder) ? *existing
        : ReferenceMaterial();
    auto choose = [&](const QString &key, const QString &oldValue,
                      const QString &folderValue, const QString &infoValue) {
        return chosenValue(key, fieldSources, oldValue, folderValue, infoValue);
    };
    reference.name = choose(QStringLiteral("name"), existing ? existing->name : item.name,
        item.name, info ? info->name : item.name);
    reference.source = choose(QStringLiteral("source"), existing ? existing->source : QString(),
        QString(), info ? info->source : QString());
    reference.tag = choose(QStringLiteral("tag"), existing ? existing->tag : QString(),
        QString(), info ? info->tag : QString());
    reference.description = choose(QStringLiteral("description"),
        existing ? existing->description : QString(), QString(),
        info ? info->description : QString());
    if (reference.name.trimmed().isEmpty()) {
        result.error = QStringLiteral("资料名称不能为空");
        return result;
    }
    if (merge && existing && !sameFolder) {
        if (!QDir(existing->folderPath).exists()
            || !copyMergeFiles(item.folderPath, existing->folderPath)) {
            result.error = QStringLiteral("资料附件合并失败，来源目录未删除");
            return result;
        }
        if (!dao.update(reference) || !ReferenceInfoFile::write(reference)) {
            result.error = QStringLiteral("合并资料信息写入失败，来源目录仍保留");
            return result;
        }
        const QString retainedBase = QFileInfo(item.folderPath).absolutePath()
            + QStringLiteral("/_已合并_") + item.folderName;
        QString retained = retainedBase;
        int suffix = 1;
        while (QDir(retained).exists()) retained = retainedBase + QString::number(suffix++);
        if (!QDir().rename(item.folderPath, retained)) {
            result.error = QStringLiteral("资料已合并，但来源目录重命名失败");
            return result;
        }
        result.ok = true;
        result.taskId = reference.id;
        return result;
    }
    QString number = choose(QStringLiteral("ref_no"), existing ? existing->refNo : item.taskNo,
        item.taskNo, info ? info->refNo : item.taskNo);
    if (!sameFolder && (existing || !isValidTaskNo(number) || dao.findByRefNo(number))) {
        const QString date = QDate::currentDate().toString(QStringLiteral("yyyyMMdd"));
        for (int seq = dao.getMaxSequenceForDate(date) + 1; seq <= 999; ++seq) {
            const QString candidate = QStringLiteral("%1-%2").arg(date)
                .arg(seq, 3, 10, QLatin1Char('0'));
            if (!dao.findByRefNo(candidate)) { number = candidate; break; }
        }
    }
    if (!isValidTaskNo(number)
        || (dao.findByRefNo(number) && (!sameFolder || number != existing->refNo))) {
        result.error = QStringLiteral("所选资料编号无效或已被使用");
        return result;
    }
    reference.refNo = number;
    reference.folderName = FileService().buildRefFolderName(reference);
    reference.folderPath = QDir(QFileInfo(item.folderPath).absolutePath())
        .filePath(reference.folderName);
    if (QFileInfo::exists(reference.folderPath)
        && QDir::cleanPath(reference.folderPath) != QDir::cleanPath(item.folderPath)) {
        result.error = QStringLiteral("目标资料目录名已存在");
        return result;
    }
    const bool renamed = QDir::cleanPath(reference.folderPath)
        != QDir::cleanPath(item.folderPath);
    if (renamed && !QDir().rename(item.folderPath, reference.folderPath)) {
        result.error = QStringLiteral("资料目录重命名失败，请关闭占用文件后重试");
        return result;
    }
    if (!sameFolder && info) reference.createTime = info->createTime;
    const int id = sameFolder ? (dao.update(reference) ? reference.id : 0)
                              : dao.insert(reference);
    if (id <= 0) {
        if (renamed) QDir().rename(reference.folderPath, item.folderPath);
        result.error = QStringLiteral("资料写入数据库失败");
        return result;
    }
    reference.id = id;
    if (!ReferenceInfoFile::write(reference)) {
        if (sameFolder) dao.update(*existing);
        else dao.deleteById(id);
        if (renamed) QDir().rename(reference.folderPath, item.folderPath);
        result.error = QStringLiteral("资料信息文件写入失败");
        return result;
    }
    result.ok = true;
    result.taskId = id;
    return result;
}
