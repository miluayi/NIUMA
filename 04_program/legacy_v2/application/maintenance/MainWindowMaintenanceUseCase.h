#pragma once

#include <QString>

struct MainWindowStatus {
    int taskCount = 0;
    QString workDir;
    qint64 workDirBytes = 0;
};

struct WorkDirChangeResult {
    bool success = false;
    QString errorMessage;
    QString workDir;
    QString previousWorkDir;
};

class MainWindowMaintenanceUseCase {
public:
    MainWindowStatus status() const;
    bool clearFocusIfNewDay() const;
    WorkDirChangeResult changeWorkDir(const QString &rootPath) const;
    bool manualBackup() const;
    bool manualBackup(bool full) const;
    QString manualBackupTo(const QString &destinationDir) const;
    bool restoreBackup(const QString &backupPath) const;
};
