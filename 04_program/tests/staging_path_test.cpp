#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

#include "DatabaseManager.h"
#include "FileService.h"
#include "PathUtil.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    const QString dataDir = PathUtil::dataDirPath();
    const QString expected = QDir::cleanPath(qEnvironmentVariable("NIUMA_DATA_DIR"));
    if (expected.isEmpty() || dataDir != expected) {
        return 1;
    }
    if (!QDir().mkpath(dataDir)) {
        return 2;
    }

    const QString databasePath = QDir(dataDir).filePath("database.db");
    if (!DatabaseManager::instance().initialize(databasePath)) {
        return 3;
    }

    FileService fileService;
    const FileService::InitResult workDirResult = fileService.initializeWorkDir(
        QDir(dataDir).filePath("ui-e2e-root"));
    if (!workDirResult.success || fileService.getWorkDir() != workDirResult.workDir) {
        DatabaseManager::instance().close();
        return 4;
    }

    const QString fixturePath = QDir(dataDir).absolutePath();
    const bool insideFixture = QFileInfo(databasePath).absoluteFilePath().startsWith(fixturePath)
        && workDirResult.workDir.startsWith(fixturePath);
    DatabaseManager::instance().close();
    return insideFixture ? 0 : 5;
}
