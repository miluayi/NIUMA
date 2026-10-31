#pragma once

#include <QString>
#include <functional>

struct StartupResult {
    bool databaseInitialized = false;
    bool workDirInitialized = false;
    QString dataDir;
    QString databasePath;
    QString workDir;
    int cleanedTaskCount = 0;
    int importedTaskCount = 0;
    int pendingConflictCount = 0;
};

class StartupUseCase {
public:
    StartupResult prepare(const std::function<QString()> &selectWorkDir) const;
    bool exportOnExit() const;
    void closeDatabase() const;
};
