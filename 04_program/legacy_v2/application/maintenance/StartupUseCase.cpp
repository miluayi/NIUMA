#include "application/maintenance/StartupUseCase.h"

#include "database/DatabaseManager.h"
#include "database/TaskDao.h"
#include "database/ReferenceMaterialDao.h"
#include "models/TaskFilter.h"
#include "service/BackupService.h"
#include "service/FileService.h"
#include "service/FocusService.h"
#include "service/TaskService.h"
#include "service/TaskInfoFile.h"
#include "service/ReferenceInfoFile.h"
#include "service/ScanMatchService.h"
#include <QFileInfo>
#include "utils/FileUtil.h"
#include "utils/Logger.h"
#include "utils/PathUtil.h"

StartupResult StartupUseCase::prepare(const std::function<QString()> &selectWorkDir) const
{
    StartupResult result;
    result.dataDir = PathUtil::dataDirPath();
    if (!FileUtil::pathExists(result.dataDir)) {
        FileUtil::ensureDir(result.dataDir);
    }

    result.databasePath = PathUtil::databasePath();
    result.databaseInitialized = DatabaseManager::instance().initialize(result.databasePath);
    if (!result.databaseInitialized) {
        Logger::instance().error(QStringLiteral("数据库初始化失败: ") + result.databasePath);
    }

    BackupService backups;
    backups.tryAutoRestoreFromInstallDir();
    backups.autoBackup();

    FileService files;
    QString workDir = files.getWorkDir();
    const QString previousWorkDir = workDir;
    if (workDir.isEmpty() || !files.validateStructure(workDir)) {
        QString rootPath = selectWorkDir ? selectWorkDir() : QString();
        if (rootPath.isEmpty() && previousWorkDir.isEmpty()) {
            rootPath = PathUtil::join(result.dataDir, "WorkDir");
        }
        if (!rootPath.isEmpty()) {
            const FileService::InitResult initialized = files.initializeWorkDir(rootPath);
            if (!initialized.success) {
                Logger::instance().warn(QStringLiteral("工作目录初始化失败: ") + initialized.errorMessage);
            }
            workDir = initialized.success ? initialized.workDir : QString();
            if (initialized.success && !previousWorkDir.isEmpty()
                && previousWorkDir != workDir)
                files.batchUpdatePaths(previousWorkDir, workDir);
        } else {
            workDir.clear();
        }
    }
    result.workDir = workDir;
    result.workDirInitialized = !workDir.isEmpty();

    // Give tasks from older releases a recoverable info file without replacing
    // an existing copy. A missing database can later be rebuilt by scan/import.
    if (result.databaseInitialized && result.workDirInitialized) {
        TaskFilter all;
        all.pageSize = 0;
        const QList<Task> tasks = TaskDao(DatabaseManager::instance().database()).findByFilter(all);
        for (const Task &task : tasks) {
            if (!task.folderPath.isEmpty() && QFileInfo::exists(task.folderPath)
                && !QFileInfo::exists(TaskInfoFile::path(task.folderPath))) {
                if (!TaskInfoFile::write(task)) {
                    Logger::instance().warn(QStringLiteral("旧任务信息文件补建失败: %1").arg(task.folderPath));
                }
            }
        }
        const QList<Task> deletedTasks =
            TaskDao(DatabaseManager::instance().database()).findDeleted();
        for (const Task &task : deletedTasks) {
            if (task.folderPath.isEmpty() || !QFileInfo::exists(task.folderPath)) continue;
            const auto saved = TaskInfoFile::read(task.folderPath);
            const bool needsBackfill = !QFileInfo::exists(TaskInfoFile::path(task.folderPath))
                || (saved && saved->taskNo == task.taskNo && saved->title == task.title
                    && saved->category == task.category && saved->originalStatus.isEmpty()
                    && !task.originalStatus.isEmpty());
            if (needsBackfill && !TaskInfoFile::write(task))
                Logger::instance().warn(QStringLiteral("回收站任务信息文件补建失败: %1")
                                            .arg(task.folderPath));
        }
        const QList<ReferenceMaterial> references =
            ReferenceMaterialDao(DatabaseManager::instance().database()).findAll();
        for (const ReferenceMaterial &reference : references) {
            if (!reference.folderPath.isEmpty() && QFileInfo::exists(reference.folderPath)
                && !QFileInfo::exists(ReferenceInfoFile::path(reference.folderPath))
                && !ReferenceInfoFile::write(reference)) {
                Logger::instance().warn(QStringLiteral("旧资料信息文件补建失败: %1")
                                            .arg(reference.folderPath));
            }
        }
        // 每次启动都扫描根层任务目录；安全项自动导入，冲突交给用户。
        ScanMatchService scanner;
        const QList<ScanImportItem> items = scanner.scanForImport(workDir);
        for (const ScanImportItem &item : items) {
            if (item.isConflict) {
                ++result.pendingConflictCount;
            } else if (item.status == QStringLiteral("待导入")) {
                if (scanner.importScanItem(item, workDir).ok)
                    ++result.importedTaskCount;
            }
        }
    }

    if (FocusService().clearIfNewDay()) {
        Logger::instance().info(QStringLiteral("启动时检测到跨天，已清零今日专注"));
    }

    result.cleanedTaskCount = TaskService().cleanExpiredDeleted(30);
    if (result.cleanedTaskCount > 0) {
        Logger::instance().info(QStringLiteral("回收站自动清理 %1 条过期任务")
                                    .arg(result.cleanedTaskCount));
    }
    return result;
}

bool StartupUseCase::exportOnExit() const
{
    return BackupService().autoExportToInstallDir();
}

void StartupUseCase::closeDatabase() const
{
    DatabaseManager::instance().close();
}
