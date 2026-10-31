/**
 * @file BackupService.cpp
 * @brief 备份业务逻辑层实现：自动备份、手动备份、恢复与 .niuma 包格式
 *
 * 依赖：database/DatabaseManager、database/SettingsDao、
 *       utils/FileUtil、utils/PathUtil、utils/Logger、utils/DateUtil
 * 文档：task.md T8.3/T8.4/T8.5、TD 4.3、PRD 11.2.1/11.2.2/11.2.3/11.2.4
 *
 * 实现说明：
 *  - .niuma 采用目录形式（简化方案，无 ZIP/miniz 依赖）
 *  - 备份前执行 PRAGMA wal_checkpoint(TRUNCATE) 合并 WAL，保证 database.db 自包含
 *  - SHA-256 使用 QCryptographicHash::Sha256
 *  - 恢复流程：关闭连接 → 复制文件 → 重新 initialize
 */

#include "BackupService.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

#include "DatabaseManager.h"
#include "FileService.h"
#include "SettingsDao.h"
#include "utils/DateUtil.h"
#include "utils/FileUtil.h"
#include "utils/Logger.h"
#include "utils/PathUtil.h"

// ============================================================================
// 常量定义
// ============================================================================

const int BackupService::kMaxBackupCount = 10;
const QString BackupService::kAutoNiumaName = QStringLiteral("NIUMA_auto.niuma");
const QString BackupService::kAutoBakNiumaName = QStringLiteral("NIUMA_auto.bak.niuma");
const QString BackupService::kManifestFile = QStringLiteral("manifest.json");
const QString BackupService::kConfigFile = QStringLiteral("config.json");
const QString BackupService::kDatabaseFile = QStringLiteral("database.db");
const QString BackupService::kSafetyBackupPrefix = QStringLiteral("NIUMA_SafetyBackup_");
const QString BackupService::kPreRestorePrefix = QStringLiteral("NIUMA_pre_restore_");
const QString BackupService::kNiumaSuffix = QStringLiteral(".niuma");
const QString BackupService::kBackupDateKey = QStringLiteral("last_backup_date");

namespace {
bool copyWorkTree(const QString &source, const QString &destination,
                  BackupService::BackupMode mode)
{
    if (!QDir(source).exists() || !QDir().mkpath(destination)) return false;
    QDirIterator it(source, QDir::Files | QDir::Dirs | QDir::Hidden | QDir::System
                    | QDir::NoDotAndDotDot | QDir::NoSymLinks,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString current = it.next();
        const QFileInfo info = it.fileInfo();
        const QString relative = QDir(source).relativeFilePath(current);
        const QString target = QDir(destination).filePath(relative);
        if (info.isDir()) {
            if (!QDir().mkpath(target)) return false;
        } else if (mode == BackupService::BackupMode::Full
                   || info.fileName() == QStringLiteral(".niuma-task.json")
                   || info.fileName() == QStringLiteral(".niuma-reference.json")) {
            if (!QDir().mkpath(QFileInfo(target).absolutePath())
                || !QFile::copy(current, target)) return false;
        }
    }
    return true;
}
}

// ============================================================================
// 私有辅助方法
// ============================================================================

bool BackupService::isDatabaseHealthy()
{
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen()) {
        return false;
    }
    QSqlQuery query(db);
    if (!query.exec(QStringLiteral("PRAGMA integrity_check;"))) {
        Logger::instance().error(QStringLiteral("BackupService::isDatabaseHealthy 执行失败: %1")
                                     .arg(query.lastError().text()));
        return false;
    }
    if (query.next()) {
        QString result = query.value(0).toString();
        return result.compare(QStringLiteral("ok"), Qt::CaseInsensitive) == 0;
    }
    return false;
}

bool BackupService::checkpointWal()
{
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen()) {
        return false;
    }
    QSqlQuery query(db);
    // TRUNCATE 模式：将 WAL 写入主库并截断 WAL 文件，保证 database.db 自包含
    if (!query.exec(QStringLiteral("PRAGMA wal_checkpoint(TRUNCATE);"))) {
        Logger::instance().warn(QStringLiteral("BackupService::checkpointWal 失败: %1")
                                    .arg(query.lastError().text()));
        return false;
    }
    return true;
}

bool BackupService::exportConfigJson(const QString &configPath)
{
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen()) {
        Logger::instance().error(QStringLiteral("BackupService::exportConfigJson 数据库未打开"));
        return false;
    }

    // 查询所有 settings 键值对
    QSqlQuery query(db);
    query.prepare(QStringLiteral("SELECT key, value FROM settings"));
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral("BackupService::exportConfigJson 查询失败: %1")
                                     .arg(query.lastError().text()));
        return false;
    }

    QJsonObject configObj;
    while (query.next()) {
        QString key = query.value(0).toString();
        QString value = query.value(1).toString();
        configObj[key] = value;
    }

    // 写入 config.json
    QFileInfo info(configPath);
    QString dirPath = info.absolutePath();
    if (!FileUtil::ensureDir(dirPath)) {
        Logger::instance().error(QStringLiteral("BackupService::exportConfigJson 创建目录失败: %1").arg(dirPath));
        return false;
    }

    QFile file(configPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        Logger::instance().error(QStringLiteral("BackupService::exportConfigJson 打开文件失败: %1").arg(configPath));
        return false;
    }
    QJsonDocument doc(configObj);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

QString BackupService::fileSha256(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        Logger::instance().error(QStringLiteral("BackupService::fileSha256 打开文件失败: %1").arg(filePath));
        return QString();
    }
    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(&file)) {
        file.close();
        return QString();
    }
    file.close();
    return QString::fromUtf8(hash.result().toHex());
}

QString BackupService::timestampStr()
{
    return QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss"));
}

void BackupService::cleanupOldBackups(const QString &dir, const QString &prefix, int keepCount)
{
    QDir backupDir(dir);
    if (!backupDir.exists()) {
        return;
    }

    // 筛选匹配前缀 + .niuma 后缀的目录
    QStringList filters;
    filters << prefix + QStringLiteral("*") + kNiumaSuffix;
    QStringList entries = backupDir.entryList(filters, QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);

    // entryList 按 Name 升序排列，时间戳在文件名中，升序 = 旧→新
    // 需要保留最新 keepCount 份，即删除前 (count - keepCount) 份
    int total = entries.size();
    if (total <= keepCount) {
        return;
    }

    int deleteCount = total - keepCount;
    for (int i = 0; i < deleteCount; ++i) {
        QString path = PathUtil::join(dir, entries.at(i));
        if (removeDir(path)) {
            Logger::instance().info(QStringLiteral("BackupService::cleanupOldBackups 删除旧备份: %1").arg(path));
        } else {
            Logger::instance().warn(QStringLiteral("BackupService::cleanupOldBackups 删除失败: %1").arg(path));
        }
    }
}

bool BackupService::removeDir(const QString &path)
{
    return FileUtil::removeDir(path);
}

bool BackupService::renameDir(const QString &src, const QString &dst)
{
    if (src.isEmpty() || dst.isEmpty()) {
        return false;
    }
    // 先尝试直接重命名（同盘符，最快）
    if (QDir().rename(src, dst)) {
        return true;
    }
    // 跨盘符则复制后删除
    if (!FileUtil::copyDir(src, dst)) {
        return false;
    }
    return removeDir(src);
}

// ============================================================================
// .niuma 文件格式（T8.4）
// ============================================================================

bool BackupService::createNiumaPackage(const QString &dbPath, const QString &configPath, const QString &outputPath)
{
    if (dbPath.isEmpty() || outputPath.isEmpty()) {
        Logger::instance().error(QStringLiteral("BackupService::createNiumaPackage 参数为空"));
        return false;
    }
    if (!QFile::exists(dbPath)) {
        Logger::instance().error(QStringLiteral("BackupService::createNiumaPackage 数据库文件不存在: %1").arg(dbPath));
        return false;
    }

    // 创建 .niuma 目录
    if (!FileUtil::ensureDir(outputPath)) {
        Logger::instance().error(QStringLiteral("BackupService::createNiumaPackage 创建目录失败: %1").arg(outputPath));
        return false;
    }

    // 1. 复制 database.db
    QString destDbPath = PathUtil::join(outputPath, kDatabaseFile);
    if (!FileUtil::copyFile(dbPath, destDbPath)) {
        Logger::instance().error(QStringLiteral("BackupService::createNiumaPackage 复制数据库失败: %1 -> %2").arg(dbPath).arg(destDbPath));
        return false;
    }

    // 2. 复制 config.json（若 configPath 为空则从 SettingsDao 导出）
    QString destConfigPath = PathUtil::join(outputPath, kConfigFile);
    if (!configPath.isEmpty()) {
        if (QFile::exists(configPath)) {
            if (!FileUtil::copyFile(configPath, destConfigPath)) {
                Logger::instance().warn(QStringLiteral("BackupService::createNiumaPackage 复制配置失败: %1").arg(configPath));
            }
        } else {
            Logger::instance().warn(QStringLiteral("BackupService::createNiumaPackage 配置文件不存在: %1").arg(configPath));
        }
    } else {
        // 从当前数据库导出配置
        if (!exportConfigJson(destConfigPath)) {
            Logger::instance().warn(QStringLiteral("BackupService::createNiumaPackage 导出配置失败"));
        }
    }

    // 3. 生成 manifest.json（含 SHA-256 校验）
    QString sha256 = fileSha256(destDbPath);
    qint64 dbSize = QFileInfo(destDbPath).size();

    QJsonObject manifest;
    manifest[QStringLiteral("version")] = QStringLiteral("2.0");
    manifest[QStringLiteral("format")] = QStringLiteral("niuma-directory-v1");
    manifest[QStringLiteral("created_at")] = DateUtil::currentDateTimeStr();
    manifest[QStringLiteral("database_sha256")] = sha256;
    manifest[QStringLiteral("database_size")] = dbSize;
    manifest[QStringLiteral("database_file")] = kDatabaseFile;
    manifest[QStringLiteral("config_file")] = kConfigFile;

    QString manifestPath = PathUtil::join(outputPath, kManifestFile);
    QFile manifestFile(manifestPath);
    if (!manifestFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        Logger::instance().error(QStringLiteral("BackupService::createNiumaPackage 打开 manifest 失败: %1").arg(manifestPath));
        return false;
    }
    QJsonDocument doc(manifest);
    manifestFile.write(doc.toJson(QJsonDocument::Indented));
    manifestFile.close();

    Logger::instance().info(QStringLiteral("BackupService::createNiumaPackage 成功: %1 (sha256=%2, size=%3)")
                                .arg(outputPath).arg(sha256.left(16)).arg(dbSize));
    return true;
}

bool BackupService::extractNiumaPackage(const QString &niumaPath, const QString &extractDir)
{
    if (niumaPath.isEmpty() || extractDir.isEmpty()) {
        return false;
    }
    if (!QFile::exists(niumaPath)) {
        Logger::instance().error(QStringLiteral("BackupService::extractNiumaPackage 包不存在: %1").arg(niumaPath));
        return false;
    }

    if (!FileUtil::ensureDir(extractDir)) {
        Logger::instance().error(QStringLiteral("BackupService::extractNiumaPackage 创建目录失败: %1").arg(extractDir));
        return false;
    }

    // 复制 database.db
    QString srcDb = PathUtil::join(niumaPath, kDatabaseFile);
    QString dstDb = PathUtil::join(extractDir, kDatabaseFile);
    if (QFile::exists(srcDb)) {
        if (!FileUtil::copyFile(srcDb, dstDb)) {
            Logger::instance().error(QStringLiteral("BackupService::extractNiumaPackage 复制数据库失败"));
            return false;
        }
    }

    // 复制 config.json
    QString srcConfig = PathUtil::join(niumaPath, kConfigFile);
    QString dstConfig = PathUtil::join(extractDir, kConfigFile);
    if (QFile::exists(srcConfig)) {
        FileUtil::copyFile(srcConfig, dstConfig);
    }

    // 复制 manifest.json
    QString srcManifest = PathUtil::join(niumaPath, kManifestFile);
    QString dstManifest = PathUtil::join(extractDir, kManifestFile);
    if (QFile::exists(srcManifest)) {
        FileUtil::copyFile(srcManifest, dstManifest);
    }

    Logger::instance().info(QStringLiteral("BackupService::extractNiumaPackage 成功: %1 -> %2").arg(niumaPath).arg(extractDir));
    return true;
}

QJsonObject BackupService::readNiumaMeta(const QString &niumaPath)
{
    QString manifestPath = PathUtil::join(niumaPath, kManifestFile);
    QFile file(manifestPath);
    if (!file.open(QIODevice::ReadOnly)) {
        Logger::instance().warn(QStringLiteral("BackupService::readNiumaMeta 打开失败: %1").arg(manifestPath));
        return QJsonObject();
    }
    QByteArray data = file.readAll();
    file.close();
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        Logger::instance().warn(QStringLiteral("BackupService::readNiumaMeta 解析失败: %1").arg(parseError.errorString()));
        return QJsonObject();
    }
    return doc.object();
}

bool BackupService::verifyNiuma(const QString &niumaPath)
{
    if (niumaPath.isEmpty() || !QFile::exists(niumaPath)) {
        return false;
    }

    // 读取 manifest
    QJsonObject meta = readNiumaMeta(niumaPath);
    if (meta.isEmpty()) {
        Logger::instance().warn(QStringLiteral("BackupService::verifyNiuma manifest 为空: %1").arg(niumaPath));
        return false;
    }

    QString storedSha256 = meta.value(QStringLiteral("database_sha256")).toString();
    if (storedSha256.isEmpty()) {
        Logger::instance().warn(QStringLiteral("BackupService::verifyNiuma manifest 缺少 database_sha256"));
        return false;
    }

    // 计算 database.db 的 SHA-256
    QString dbPath = PathUtil::join(niumaPath, kDatabaseFile);
    if (!QFile::exists(dbPath)) {
        Logger::instance().warn(QStringLiteral("BackupService::verifyNiuma database.db 不存在: %1").arg(dbPath));
        return false;
    }

    QString actualSha256 = fileSha256(dbPath);
    if (actualSha256.isEmpty()) {
        return false;
    }

    if (actualSha256.compare(storedSha256, Qt::CaseInsensitive) != 0) {
        Logger::instance().error(QStringLiteral("BackupService::verifyNiuma SHA-256 不匹配: stored=%1 actual=%2")
                                     .arg(storedSha256.left(16)).arg(actualSha256.left(16)));
        return false;
    }
    return true;
}

// ============================================================================
// 自动备份（T8.3）
// ============================================================================

bool BackupService::autoBackup()
{
    // 检查今日是否已备份
    SettingsDao settingsDao(DatabaseManager::instance().database());
    QString lastBackupDate = settingsDao.getValue(kBackupDateKey);
    QString today = DateUtil::currentDateStr();

    if (lastBackupDate == today) {
        Logger::instance().info(QStringLiteral("BackupService::autoBackup 今日已备份，跳过"));
        return false;
    }

    // 执行备份到 Data/backups/
    QString backupDir = PathUtil::defaultBackupDirPath();
    QString niumaPath = manualBackupTo(backupDir);
    if (niumaPath.isEmpty()) {
        Logger::instance().error(QStringLiteral("BackupService::autoBackup 备份失败"));
        return false;
    }

    // 更新 last_backup_date
    if (!settingsDao.setValue(kBackupDateKey, today)) {
        Logger::instance().warn(QStringLiteral("BackupService::autoBackup 更新 last_backup_date 失败"));
    }

    Logger::instance().info(QStringLiteral("BackupService::autoBackup 成功: %1").arg(niumaPath));
    return true;
}

bool BackupService::autoExportToInstallDir()
{
    // 1. 旧文件重命名为 .bak
    if (!syncAutoNiuma()) {
        Logger::instance().warn(QStringLiteral("BackupService::autoExportToInstallDir syncAutoNiuma 失败，继续生成新包"));
    }

    // 2. WAL checkpoint 保证 database.db 自包含
    checkpointWal();

    // 3. 生成 NIUMA_auto.niuma 到 Data 目录
    QString dbPath = DatabaseManager::instance().database().databaseName();
    QString outputPath = PathUtil::join(PathUtil::dataDirPath(), kAutoNiumaName);

    // 若已存在则先删除（避免 copyFile 失败）
    if (QFile::exists(outputPath)) {
        removeDir(outputPath);
    }

    if (!createNiumaPackage(dbPath, QString(), outputPath)) {
        Logger::instance().error(QStringLiteral("BackupService::autoExportToInstallDir 生成失败: %1").arg(outputPath));
        return false;
    }

    Logger::instance().info(QStringLiteral("BackupService::autoExportToInstallDir 成功: %1").arg(outputPath));
    return true;
}

bool BackupService::syncAutoNiuma()
{
    QString dataDir = PathUtil::dataDirPath();
    QString autoPath = PathUtil::join(dataDir, kAutoNiumaName);
    QString bakPath = PathUtil::join(dataDir, kAutoBakNiumaName);

    // 若 NIUMA_auto.niuma 不存在，无需重命名
    if (!QFile::exists(autoPath)) {
        return true;
    }

    // 若 NIUMA_auto.bak.niuma 已存在则先删除（仅保留 1 份）
    if (QFile::exists(bakPath)) {
        if (!removeDir(bakPath)) {
            Logger::instance().error(QStringLiteral("BackupService::syncAutoNiuma 删除旧 .bak 失败: %1").arg(bakPath));
            return false;
        }
    }

    // 重命名 NIUMA_auto.niuma -> NIUMA_auto.bak.niuma
    if (!renameDir(autoPath, bakPath)) {
        Logger::instance().error(QStringLiteral("BackupService::syncAutoNiuma 重命名失败: %1 -> %2").arg(autoPath).arg(bakPath));
        return false;
    }

    Logger::instance().info(QStringLiteral("BackupService::syncAutoNiuma 成功: %1 -> %2").arg(autoPath).arg(bakPath));
    return true;
}

bool BackupService::tryAutoRestoreFromInstallDir()
{
    // 检测当前数据库完整性
    if (isDatabaseHealthy()) {
        return false;  // 数据库健康，无需恢复
    }

    Logger::instance().error(QStringLiteral("BackupService::tryAutoRestoreFromInstallDir 数据库完整性检查失败，尝试恢复"));

    QString bakPath = PathUtil::join(PathUtil::dataDirPath(), kAutoBakNiumaName);
    QString bakDbPath = PathUtil::join(bakPath, kDatabaseFile);

    if (!QFile::exists(bakDbPath)) {
        Logger::instance().error(QStringLiteral("BackupService::tryAutoRestoreFromInstallDir 备份不存在: %1").arg(bakDbPath));
        return false;
    }

    // 获取当前数据库路径（需在 close 之前获取）
    QString dbPath = DatabaseManager::instance().database().databaseName();

    // 关闭当前数据库连接
    DatabaseManager::instance().close();

    // 备份当前损坏的数据库（留痕）
    QString corruptBakPath = PathUtil::join(
        PathUtil::dataDirPath(),
        kPreRestorePrefix + timestampStr() + QStringLiteral(".corrupt.bak"));
    if (QFile::exists(dbPath)) {
        FileUtil::copyFile(dbPath, corruptBakPath);
    }

    // 用备份覆盖当前数据库
    if (!FileUtil::copyFile(bakDbPath, dbPath)) {
        Logger::instance().error(QStringLiteral("BackupService::tryAutoRestoreFromInstallDir 恢复失败: %1 -> %2")
                                     .arg(bakDbPath).arg(dbPath));
        // 尝试重新初始化（即便恢复失败）
        DatabaseManager::instance().initialize(dbPath);
        return false;
    }

    // 重新初始化数据库
    if (!DatabaseManager::instance().initialize(dbPath)) {
        Logger::instance().error(QStringLiteral("BackupService::tryAutoRestoreFromInstallDir 重新初始化失败"));
        return false;
    }

    Logger::instance().info(QStringLiteral("BackupService::tryAutoRestoreFromInstallDir 恢复成功"));
    return true;
}

// ============================================================================
// 手动备份与恢复（T8.5）
// ============================================================================

bool BackupService::manualBackup()
{
    QString backupDir = PathUtil::defaultBackupDirPath();
    QString niumaPath = manualBackupTo(backupDir);
    return !niumaPath.isEmpty();
}

qint64 BackupService::workDirSize(const QString &workDir)
{
    if (workDir.isEmpty() || !QDir(workDir).exists()) return 0;
    qint64 bytes = 0;
    QDirIterator it(workDir, QDir::Files | QDir::Hidden | QDir::System
                    | QDir::NoSymLinks, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        bytes += it.fileInfo().size();
    }
    return bytes;
}

QString BackupService::manualBackupTo(const QString &destDir, BackupMode mode)
{
    if (destDir.isEmpty()) {
        Logger::instance().error(QStringLiteral("BackupService::manualBackupTo 目标目录为空"));
        return QString();
    }

    if (!FileUtil::ensureDir(destDir)) {
        Logger::instance().error(QStringLiteral("BackupService::manualBackupTo 创建目录失败: %1").arg(destDir));
        return QString();
    }

    // WAL checkpoint
    checkpointWal();

    // 生成带时间戳的 .niuma
    QString name = kSafetyBackupPrefix + timestampStr() + kNiumaSuffix;
    QString outputPath = PathUtil::join(destDir, name);
    for (int suffix = 1; QFileInfo::exists(outputPath); ++suffix) {
        name = kSafetyBackupPrefix + timestampStr()
            + QStringLiteral("_%1").arg(suffix) + kNiumaSuffix;
        outputPath = PathUtil::join(destDir, name);
    }

    QString dbPath = DatabaseManager::instance().database().databaseName();
    if (!createNiumaPackage(dbPath, QString(), outputPath)) {
        Logger::instance().error(QStringLiteral("BackupService::manualBackupTo 创建包失败: %1").arg(outputPath));
        return QString();
    }

    const QString workDir = FileService().getWorkDir();
    const bool hasWorkDir = !workDir.isEmpty() && QDir(workDir).exists();
    if (mode == BackupMode::Full && !hasWorkDir) {
        removeDir(outputPath);
        return {};
    }
    if (hasWorkDir) {
        const QString canonicalWorkDir = QDir::fromNativeSeparators(
            QFileInfo(workDir).canonicalFilePath());
        const QString canonicalDestination = QDir::fromNativeSeparators(
            QFileInfo(destDir).canonicalFilePath());
        if (canonicalDestination.compare(canonicalWorkDir, Qt::CaseInsensitive) == 0
            || canonicalDestination.startsWith(canonicalWorkDir + QLatin1Char('/'), Qt::CaseInsensitive)
            || !copyWorkTree(workDir, PathUtil::join(outputPath, QStringLiteral("workdir")), mode)) {
            removeDir(outputPath);
            Logger::instance().error(QStringLiteral("BackupService::manualBackupTo 工作目录打包失败"));
            return {};
        }
    }
    QJsonObject manifest = readNiumaMeta(outputPath);
    manifest[QStringLiteral("backup_type")] = !hasWorkDir
        ? QStringLiteral("database_only")
        : mode == BackupMode::Full ? QStringLiteral("full") : QStringLiteral("simple");
    manifest[QStringLiteral("workdir_size")] = static_cast<double>(workDirSize(workDir));
    QFile manifestFile(PathUtil::join(outputPath, kManifestFile));
    if (!manifestFile.open(QIODevice::WriteOnly | QIODevice::Truncate)
        || manifestFile.write(QJsonDocument(manifest).toJson(QJsonDocument::Indented)) < 0) {
        removeDir(outputPath);
        return {};
    }
    manifestFile.close();

    // 清理旧备份，保留 10 份
    cleanupOldBackups(destDir, kSafetyBackupPrefix, kMaxBackupCount);

    Logger::instance().info(QStringLiteral("BackupService::manualBackupTo 成功: %1").arg(outputPath));
    return outputPath;
}

bool BackupService::restoreFromBackup(const QString &backupPath)
{
    if (backupPath.isEmpty()) {
        Logger::instance().error(QStringLiteral("BackupService::restoreFromBackup 备份路径为空"));
        return false;
    }
    if (!QFile::exists(backupPath)) {
        Logger::instance().error(QStringLiteral("BackupService::restoreFromBackup 备份不存在: %1").arg(backupPath));
        return false;
    }

    QString srcDbPath = PathUtil::join(backupPath, kDatabaseFile);
    if (!QFile::exists(srcDbPath)) {
        Logger::instance().error(QStringLiteral("BackupService::restoreFromBackup 备份中 database.db 不存在: %1").arg(srcDbPath));
        return false;
    }
    if (!verifyNiuma(backupPath)) return false;

    const QJsonObject manifest = readNiumaMeta(backupPath);
    const QString type = manifest.value(QStringLiteral("backup_type")).toString();
    const QString backupWorkDir = PathUtil::join(backupPath, QStringLiteral("workdir"));
    const QString targetWorkDir = FileService().getWorkDir();
    const bool hasWorkTree = type == QStringLiteral("simple") || type == QStringLiteral("full");
    if (hasWorkTree) {
        const QString source = QDir::fromNativeSeparators(QFileInfo(backupPath).canonicalFilePath());
        const QString target = QDir::fromNativeSeparators(QFileInfo(targetWorkDir).canonicalFilePath());
        if (!target.isEmpty() && source.startsWith(target + QLatin1Char('/'), Qt::CaseInsensitive))
            return false;
    }
    QString stagedWorkDir;
    QString displacedWorkDir;
    if (hasWorkTree) {
        if (targetWorkDir.isEmpty() || !QDir(backupWorkDir).exists()) return false;
        const QString restoreId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        stagedWorkDir = targetWorkDir + QStringLiteral(".restore_stage_") + restoreId;
        displacedWorkDir = targetWorkDir + QStringLiteral(".pre_restore_") + restoreId;
        if (QDir(stagedWorkDir).exists() || QDir(displacedWorkDir).exists()
            || !copyWorkTree(backupWorkDir, stagedWorkDir, BackupMode::Full)) {
            if (QDir(stagedWorkDir).exists()) removeDir(stagedWorkDir);
            return false;
        }
    }

    // 1. 恢复前自动备份当前数据库为回退保障（单文件 .bak）
    QString dbPath = DatabaseManager::instance().database().databaseName();
    QString dataDir = PathUtil::dataDirPath();
    QString preRestoreBak = PathUtil::join(
        dataDir, kPreRestorePrefix + timestampStr() + QStringLiteral("_")
            + QUuid::createUuid().toString(QUuid::WithoutBraces) + QStringLiteral(".bak"));

    if (QFile::exists(dbPath)) {
        // WAL checkpoint 保证当前 db 自包含
        checkpointWal();
        if (!FileUtil::copyFile(dbPath, preRestoreBak)) {
            Logger::instance().error(QStringLiteral("BackupService::restoreFromBackup 回退备份失败: %1").arg(preRestoreBak));
            if (hasWorkTree) removeDir(stagedWorkDir);
            return false;
        } else {
            Logger::instance().info(QStringLiteral("BackupService::restoreFromBackup 已创建回退备份: %1").arg(preRestoreBak));
        }
    }

    // 先替换目录；旧目录保留在同级供人工回退。
    if (hasWorkTree) {
        if ((QDir(targetWorkDir).exists() && !QDir().rename(targetWorkDir, displacedWorkDir))
            || !QDir().rename(stagedWorkDir, targetWorkDir)) {
            if (QDir(displacedWorkDir).exists()) QDir().rename(displacedWorkDir, targetWorkDir);
            if (QDir(stagedWorkDir).exists()) removeDir(stagedWorkDir);
            return false;
        }
    }

    // 2. 关闭数据库连接
    DatabaseManager::instance().close();

    // 3. 用备份覆盖当前数据库
    if (!FileUtil::copyFile(srcDbPath, dbPath)) {
        Logger::instance().error(QStringLiteral("BackupService::restoreFromBackup 覆盖失败: %1 -> %2").arg(srcDbPath).arg(dbPath));
        // 尝试从回退备份恢复
        if (QFile::exists(preRestoreBak)) {
            FileUtil::copyFile(preRestoreBak, dbPath);
        }
        DatabaseManager::instance().initialize(dbPath);
        if (hasWorkTree) {
            QDir().rename(targetWorkDir, stagedWorkDir);
            QDir().rename(displacedWorkDir, targetWorkDir);
            removeDir(stagedWorkDir);
        }
        return false;
    }

    // 4. 重新初始化数据库
    if (!DatabaseManager::instance().initialize(dbPath)) {
        Logger::instance().error(QStringLiteral("BackupService::restoreFromBackup 重新初始化失败"));
        if (QFile::exists(preRestoreBak)) {
            FileUtil::copyFile(preRestoreBak, dbPath);
            DatabaseManager::instance().initialize(dbPath);
        }
        if (hasWorkTree) {
            QDir().rename(targetWorkDir, stagedWorkDir);
            QDir().rename(displacedWorkDir, targetWorkDir);
            removeDir(stagedWorkDir);
        }
        return false;
    }

    const QString oldWorkDir = FileService().getWorkDir();

    // 5. 恢复 config.json 中的 work_dir 等设置（若备份中包含 config.json）
    QString srcConfigPath = PathUtil::join(backupPath, kConfigFile);
    if (QFile::exists(srcConfigPath)) {
        QFile configFile(srcConfigPath);
        if (configFile.open(QIODevice::ReadOnly)) {
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(configFile.readAll(), &parseError);
            configFile.close();
            if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
                QJsonObject configObj = doc.object();
                SettingsDao settingsDao(DatabaseManager::instance().database());
                for (const QString &key : configObj.keys()) {
                    QString value = configObj.value(key).toString();
                    settingsDao.setValue(key, value);
                }
                Logger::instance().info(QStringLiteral("BackupService::restoreFromBackup 已恢复配置项 %1 个").arg(configObj.size()));
            }
        }
    }
    if (!targetWorkDir.isEmpty()) {
        FileService files;
        files.setWorkDir(targetWorkDir);
        if (!oldWorkDir.isEmpty() && oldWorkDir != targetWorkDir)
            files.batchUpdatePaths(oldWorkDir, targetWorkDir);
    }

    Logger::instance().info(QStringLiteral("BackupService::restoreFromBackup 恢复成功: %1").arg(backupPath));
    return true;
}
