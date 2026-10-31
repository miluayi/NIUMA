#include "application/maintenance/MainWindowMaintenanceUseCase.h"

#include "database/DatabaseManager.h"
#include "database/TaskDao.h"
#include "service/BackupService.h"
#include "service/FileService.h"
#include "service/FocusService.h"
#include "utils/PathUtil.h"

MainWindowStatus MainWindowMaintenanceUseCase::status() const
{
    TaskDao tasks(DatabaseManager::instance().database());
    const int taskCount = tasks.countByStatus(QStringLiteral("进行中"))
        + tasks.countByStatus(QStringLiteral("已完成"))
        + tasks.countByStatus(QStringLiteral("他人办理"));
    const QString workDir = FileService().getWorkDir();
    return {taskCount, workDir, BackupService::workDirSize(workDir)};
}

bool MainWindowMaintenanceUseCase::clearFocusIfNewDay() const
{
    return FocusService().clearIfNewDay();
}

WorkDirChangeResult MainWindowMaintenanceUseCase::changeWorkDir(const QString &rootPath) const
{
    FileService files;
    const QString previousWorkDir = files.getWorkDir();
    const FileService::InitResult initialized = files.initializeWorkDir(rootPath);
    if (!initialized.success) {
        return {false, initialized.errorMessage, {}, previousWorkDir};
    }
    if (!previousWorkDir.isEmpty() && previousWorkDir != initialized.workDir) {
        files.batchUpdatePaths(previousWorkDir, initialized.workDir);
    }
    return {true, {}, initialized.workDir, previousWorkDir};
}

bool MainWindowMaintenanceUseCase::manualBackup() const
{
    return BackupService().manualBackup();
}

bool MainWindowMaintenanceUseCase::manualBackup(bool full) const
{
    return !BackupService().manualBackupTo(
        PathUtil::defaultBackupDirPath(),
        full ? BackupService::BackupMode::Full : BackupService::BackupMode::Simple).isEmpty();
}

QString MainWindowMaintenanceUseCase::manualBackupTo(const QString &destinationDir) const
{
    return BackupService().manualBackupTo(destinationDir);
}

bool MainWindowMaintenanceUseCase::restoreBackup(const QString &backupPath) const
{
    return BackupService().restoreFromBackup(backupPath);
}
