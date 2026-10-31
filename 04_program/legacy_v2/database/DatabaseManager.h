/**
 * @file DatabaseManager.h
 * @brief 数据库管理器（全局单例），负责数据库连接、建表、索引、迁移、预置数据与完整性检查
 *
 * 依赖：QtSql（QSqlDatabase）、utils/Logger
 * 文档：TD 第二部分（9 张表 schema）、TD 1.2（对象生命周期）、coding.md 第五章（数据库访问规范）、task.md T1.5
 */
#pragma once

#include <QSqlDatabase>
#include <QString>

class DatabaseManager {
public:
    static DatabaseManager &instance();

    /**
     * @brief 初始化数据库，打开连接并建表
     * @param dbPath 数据库文件路径（如 Data/database.db）
     * @return true 成功；false 失败（连接打开失败）
     *
     * 流程：
     * 1. 检查 db_ 是否已打开，是则先关闭
     * 2. addDatabase("QSQLITE")，设置 dbPath（父目录不存在则创建）
     * 3. open()
     * 4. PRAGMA foreign_keys = ON（task_remark 外键 CASCADE 依赖此）
     * 5. PRAGMA journal_mode = WAL（提升性能）
     * 6. ensureTablesExist()
     * 7. runMigrations()
     * 8. seedInitialData()（幂等）
     * 9. runIntegrityCheck() 并记录日志
     * 10. initialized_ = true
     */
    bool initialize(const QString &dbPath);

    /**
     * @brief 关闭数据库连接
     */
    void close();

    /**
     * @brief 获取数据库连接（DAO 构造时使用）
     * @return QSqlDatabase 引用
     */
    QSqlDatabase database() const;

    /**
     * @brief 完整性检查（PRAGMA integrity_check）
     * @param errorMessage 若失败则写入错误信息
     * @return true 通过；false 失败
     */
    bool runIntegrityCheck(QString *errorMessage = nullptr);

    /**
     * @brief 建表（9 张表），每个方法执行 CREATE TABLE IF NOT EXISTS
     */
    void ensureTablesExist();

    /**
     * @brief 预置初始数据（幂等：先查询是否已存在再插入）
     *  - 来源字典：征管股/风险管理股/税政股/纳税服务/行政办公室/法制股/税源管理
     *  - 标签字典：股权转让
     *  - 3 条周期任务模板：日报/周报/月报
     *  - 默认 settings：work_dir/db_version/last_focus_clear_date/last_backup_date
     */
    void seedInitialData();

    /**
     * @brief 数据库迁移，检查 settings.db_version，低于 kCurrentVersion 则执行迁移
     */
    void runMigrations();

    /**
     * @brief 当前数据库版本
     */
    int currentVersion() const { return kCurrentVersion; }

private:
    DatabaseManager();
    ~DatabaseManager();
    DatabaseManager(const DatabaseManager &) = delete;
    DatabaseManager &operator=(const DatabaseManager &) = delete;

    QSqlDatabase db_;
    bool initialized_ = false;
    static const int kCurrentVersion = 1;

    void createTableTask();
    void createTableTaskRemark();
    void createTableAchievement();
    void createTablePeriodicTask();
    void createTableSourceDict();
    void createTableTagDict();
    void createTableHandlerDict();
    void createTableSettings();
    void createTableOperationLog();
    void createTableReferenceMaterial();
    void createIndexes();
    void migratePeriodicTaskAddTargetDay();
    void seedSourceDict();
    void seedTagDict();
    void seedHandlerDict();
    void seedPeriodicTaskTemplates();
    void seedDefaultSettings();
};
