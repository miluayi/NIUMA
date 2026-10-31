/**
 * @file BackupService.h
 * @brief 备份业务逻辑层（无状态），负责自动备份、手动备份、恢复与 .niuma 包格式
 *
 * 依赖：database/DatabaseManager、database/SettingsDao、
 *       utils/FileUtil、utils/PathUtil、utils/Logger、utils/DateUtil
 * 文档：task.md T8.3/T8.4/T8.5、TD 4.3、PRD 11.2.1/11.2.2/11.2.3/11.2.4
 *
 * 关键约束：
 *  - BackupService 为普通 C++ 类，不继承 QObject，不持有 db_ 成员，所有 DAO 在方法内构造
 *  - .niuma 备份采用目录形式，不使用 ZIP：
 *      {name}.niuma/
 *        - database.db      SQLite 数据库文件副本
 *        - config.json      SettingsDao 导出的配置（work_dir 等）
 *        - manifest.json    清单（SHA-256、备份类型、工作目录体积等）
 *        - workdir/         可选；简易备份为目录结构及 info，全量备份含附件
 *  - 触发时机仅 3 个（不恢复 30 分钟定时备份）：
 *      1. 每日首次启动（main.cpp 调用 autoBackup + tryAutoRestoreFromInstallDir）
 *      2. 手动备份（SettingsView 触发 manualBackup/manualBackupTo）
 *      3. 退出补生成（QApplication::aboutToQuit 触发 autoExportToInstallDir）
 *  - 备份保留策略：自动备份保留 10 份，超出删除最旧
 *  - 自动导出文件 NIUMA_auto.niuma 覆盖前旧文件重命名为 NIUMA_auto.bak.niuma（仅 1 份）
 *  - 恢复前自动备份当前数据库为 NIUMA_pre_restore_*.bak 作为回退保障
 *  - SHA-256 使用 QCryptographicHash::Sha256
 *  - 备份前执行 PRAGMA wal_checkpoint(TRUNCATE) 保证 database.db 自包含
 */
#pragma once

#include <QJsonObject>
#include <QString>

class BackupService {
public:
    enum class BackupMode { Simple, Full };
    BackupService() = default;
    ~BackupService() = default;

    // ===== 自动备份（T8.3） =====

    /**
     * @brief 每日首次启动备份
     *
     * 读取 settings.last_backup_date，若与今天不同则执行备份到 Data/backups/ 目录，
     * 文件名 NIUMA_SafetyBackup_yyyyMMdd_HHmmss.niuma，保留 10 份，超出删除最旧。
     * 备份完成后更新 last_backup_date 为今天。
     * @return true 成功；false 跳过（今日已备份）或失败
     */
    bool autoBackup();

    /**
     * @brief 生成 NIUMA_auto.niuma 到 Data 目录（退出补生成 / 启动自动导出）
     *
     * 流程：
     *   1. 调用 syncAutoNiuma() 将旧 NIUMA_auto.niuma 重命名为 NIUMA_auto.bak.niuma
     *   2. 在 Data/ 下生成 NIUMA_auto.niuma 目录（含 database.db + config.json + manifest.json）
     * @return true 成功；false 失败
     */
    bool autoExportToInstallDir();

    /**
     * @brief 覆盖前旧文件重命名为 .bak.niuma（仅保留 1 份）
     *
     * 若 NIUMA_auto.bak.niuma 已存在则先删除，再将 NIUMA_auto.niuma 重命名为
     * NIUMA_auto.bak.niuma。若 NIUMA_auto.niuma 不存在则直接返回 true。
     * @return true 成功；false 重命名失败
     */
    bool syncAutoNiuma();

    /**
     * @brief 启动时检测 database.db 损坏自动恢复
     *
     * 执行 PRAGMA integrity_check，若结果非 "ok" 且 NIUMA_auto.bak.niuma 存在，
     * 则关闭数据库连接，将备份的 database.db 覆盖当前 database.db，再重新初始化。
     * @return true 已执行恢复；false 无需恢复或恢复失败
     */
    bool tryAutoRestoreFromInstallDir();

    // ===== 手动备份与恢复（T8.5） =====

    /**
     * @brief 立即备份到默认位置（Data/backups/）
     *
     * 生成 NIUMA_SafetyBackup_yyyyMMdd_HHmmss.niuma，保留 10 份。
     * @return true 成功；false 失败
     */
    bool manualBackup();

    /**
     * @brief 备份到指定目录，生成带时间戳 .niuma
     * @param destDir 目标目录（不存在则创建）
     * @return 生成的 .niuma 目录绝对路径；失败返回空字符串
     */
    QString manualBackupTo(const QString &destDir, BackupMode mode = BackupMode::Simple);

    /**
     * @brief 从备份恢复（恢复前自动备份当前数据库为回退保障）
     *
     * 流程：
     *   1. 将当前 database.db 复制为 NIUMA_pre_restore_yyyyMMdd_HHmmss.bak
     *   2. 关闭数据库连接
     *   3. 将 backupPath 中的 database.db 覆盖当前 database.db
     *   4. 重新初始化数据库
     * @param backupPath .niuma 目录路径
     * @return true 成功；false 失败
     */
    bool restoreFromBackup(const QString &backupPath);

    static qint64 workDirSize(const QString &workDir);

    // ===== .niuma 文件格式（T8.4） =====

    /**
     * @brief 创建 .niuma 包（目录形式）
     * @param dbPath 源 database.db 路径
     * @param configPath 源 config.json 路径（为空则从 SettingsDao 导出）
     * @param outputPath 输出 .niuma 目录路径（如 .../NIUMA_SafetyBackup_20260712_143000.niuma）
     * @return true 成功；false 失败
     *
     * 生成内容：
     *   {outputPath}/database.db    - 数据库副本
     *   {outputPath}/config.json    - 配置副本
     *   {outputPath}/manifest.json  - 清单（含 database.db 的 SHA-256、版本、创建时间）
     */
    static bool createNiumaPackage(const QString &dbPath, const QString &configPath, const QString &outputPath);

    /**
     * @brief 解压 .niuma 包到指定目录
     * @param niumaPath .niuma 目录路径
     * @param extractDir 解压目标目录（不存在则创建）
     * @return true 成功；false 失败
     *
     * 将 niumaPath 中的 database.db 与 config.json 复制到 extractDir。
     */
    static bool extractNiumaPackage(const QString &niumaPath, const QString &extractDir);

    /**
     * @brief 读取 .niuma 包的 manifest 元信息
     * @param niumaPath .niuma 目录路径
     * @return QJsonObject，失败返回空对象
     *
     * manifest 字段：version / format / created_at / database_sha256 / database_size
     */
    static QJsonObject readNiumaMeta(const QString &niumaPath);

    /**
     * @brief 校验 .niuma 包完整性（SHA-256）
     * @param niumaPath .niuma 目录路径
     * @return true 校验通过；false 校验失败或文件缺失
     *
     * 读取 manifest.json 中的 database_sha256，重新计算 database.db 的 SHA-256 并比对。
     */
    static bool verifyNiuma(const QString &niumaPath);

    // ===== 常量 =====
    static const int kMaxBackupCount;            // 10
    static const QString kAutoNiumaName;         // NIUMA_auto.niuma
    static const QString kAutoBakNiumaName;      // NIUMA_auto.bak.niuma
    static const QString kManifestFile;          // manifest.json
    static const QString kConfigFile;            // config.json
    static const QString kDatabaseFile;          // database.db
    static const QString kSafetyBackupPrefix;    // NIUMA_SafetyBackup_
    static const QString kPreRestorePrefix;      // NIUMA_pre_restore_
    static const QString kNiumaSuffix;           // .niuma
    static const QString kBackupDateKey;         // last_backup_date

private:
    /**
     * @brief 执行 PRAGMA integrity_check
     * @return true 结果为 "ok"；false 失败或结果异常
     */
    static bool isDatabaseHealthy();

    /**
     * @brief 执行 WAL checkpoint（TRUNCATE），保证 database.db 自包含
     * @return true 成功；false 失败
     */
    static bool checkpointWal();

    /**
     * @brief 从 SettingsDao 导出所有配置到 config.json
     * @param configPath 输出 config.json 路径
     * @return true 成功；false 失败
     */
    static bool exportConfigJson(const QString &configPath);

    /**
     * @brief 计算文件 SHA-256
     * @param filePath 文件路径
     * @return 十六进制 SHA-256 字符串；失败返回空字符串
     */
    static QString fileSha256(const QString &filePath);

    /**
     * @brief 生成时间戳字符串 yyyyMMdd_HHmmss
     */
    static QString timestampStr();

    /**
     * @brief 清理旧备份，保留最新 keepCount 份
     * @param dir 备份目录
     * @param prefix 文件名前缀（如 NIUMA_SafetyBackup_）
     * @param keepCount 保留份数
     */
    static void cleanupOldBackups(const QString &dir, const QString &prefix, int keepCount);

    /**
     * @brief 递归删除目录（封装 FileUtil::removeDir）
     */
    static bool removeDir(const QString &path);

    /**
     * @brief 重命名目录，失败时回退到复制+删除
     */
    static bool renameDir(const QString &src, const QString &dst);
};
