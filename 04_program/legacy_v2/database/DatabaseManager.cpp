/**
 * @file DatabaseManager.cpp
 * @brief 数据库管理器实现：建表、索引、迁移、预置数据、完整性检查
 *
 * 依赖：QtSql、utils/Logger
 * 文档：TD 第二部分（9 张表 schema）、coding.md 第五章（数据库访问规范）、task.md T1.5
 *
 * 关键约束：
 *  - PRAGMA foreign_keys = ON（task_remark 外键 CASCADE 依赖此）
 *  - PRAGMA journal_mode = WAL（提升并发性能）
 *  - status 字段直接字符串值 '进行中'/'已完成'/'已删除'/'他人办理'，绝不使用 NULL 表示他人办理
 *  - handler 字段 TEXT NOT NULL DEFAULT '本人'
 *  - result_amount 存储单位为分（整数），禁止 REAL/FLOAT
 *  - 所有 DML 使用参数化查询（QSqlQuery::prepare + addBindValue）
 */

#include "DatabaseManager.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>

#include "utils/Logger.h"

// ============================================================================
// 构造 / 析构 / 单例
// ============================================================================

DatabaseManager &DatabaseManager::instance() {
    static DatabaseManager instance;
    return instance;
}

DatabaseManager::DatabaseManager() {
}

DatabaseManager::~DatabaseManager() {
    close();
}

// ============================================================================
// initialize / close / database
// ============================================================================

bool DatabaseManager::initialize(const QString &dbPath) {
    // 1. 检查 db_ 是否已打开，是则先关闭
    if (db_.isOpen()) {
        close();
    }

    // 2. addDatabase，设置 dbPath（父目录不存在则创建）
    db_ = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"));
    db_.setDatabaseName(dbPath);

    QFileInfo info(dbPath);
    QDir parentDir = info.dir();
    if (!parentDir.exists()) {
        parentDir.mkpath(QStringLiteral("."));
    }

    // 3. open
    if (!db_.open()) {
        Logger::instance().error(QStringLiteral("数据库打开失败: %1").arg(db_.lastError().text()));
        return false;
    }

    // 4. PRAGMA foreign_keys = ON（重要！task_remark 外键 CASCADE 依赖此）
    {
        QSqlQuery pragmaQuery(db_);
        if (!pragmaQuery.exec(QStringLiteral("PRAGMA foreign_keys = ON;"))) {
            Logger::instance().error(QStringLiteral("启用外键约束失败: %1").arg(pragmaQuery.lastError().text()));
        }
    }

    // 5. PRAGMA journal_mode = WAL（提升性能）
    {
        QSqlQuery pragmaQuery(db_);
        if (!pragmaQuery.exec(QStringLiteral("PRAGMA journal_mode = WAL;"))) {
            Logger::instance().warn(QStringLiteral("设置 WAL 模式失败: %1").arg(pragmaQuery.lastError().text()));
        } else if (pragmaQuery.next()) {
            QString mode = pragmaQuery.value(0).toString();
            Logger::instance().info(QStringLiteral("日志模式: %1").arg(mode));
        }
    }

    // 6. 建表
    ensureTablesExist();

    // 7. 迁移
    runMigrations();

    // 8. 预置初始数据（幂等）
    seedInitialData();

    // 9. 完整性检查
    QString integrityError;
    if (runIntegrityCheck(&integrityError)) {
        Logger::instance().info(QStringLiteral("数据库完整性检查通过"));
    } else {
        Logger::instance().error(QStringLiteral("数据库完整性检查失败: %1").arg(integrityError));
    }

    // 10. 标记已初始化
    initialized_ = true;
    Logger::instance().info(QStringLiteral("数据库初始化完成: %1").arg(dbPath));
    return true;
}

void DatabaseManager::close() {
    if (db_.isOpen()) {
        db_.close();
    }
    QString connName = db_.connectionName();
    db_ = QSqlDatabase();
    if (!connName.isEmpty()) {
        QSqlDatabase::removeDatabase(connName);
    }
    initialized_ = false;
}

QSqlDatabase DatabaseManager::database() const {
    return db_;
}

// ============================================================================
// runIntegrityCheck
// ============================================================================

bool DatabaseManager::runIntegrityCheck(QString *errorMessage) {
    QSqlQuery query(db_);
    if (!query.exec(QStringLiteral("PRAGMA integrity_check;"))) {
        if (errorMessage) {
            *errorMessage = query.lastError().text();
        }
        return false;
    }
    if (query.next()) {
        QString result = query.value(0).toString();
        if (result.compare(QStringLiteral("ok"), Qt::CaseInsensitive) == 0) {
            return true;
        }
        if (errorMessage) {
            *errorMessage = result;
        }
        return false;
    }
    if (errorMessage) {
        *errorMessage = QStringLiteral("integrity_check 无结果返回");
    }
    return false;
}

// ============================================================================
// ensureTablesExist
// ============================================================================

void DatabaseManager::ensureTablesExist() {
    createTableTask();
    createTableTaskRemark();
    createTableAchievement();
    createTablePeriodicTask();
    createTableSourceDict();
    createTableTagDict();
    createTableHandlerDict();
    createTableSettings();
    createTableOperationLog();
    createTableReferenceMaterial();
    createIndexes();
    migratePeriodicTaskAddTargetDay();
}

// ============================================================================
// 9 张表建表（CREATE TABLE IF NOT EXISTS）
// ============================================================================

void DatabaseManager::createTableTask() {
    QSqlQuery query(db_);
    bool ok = query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS task ("
        "    id              INTEGER PRIMARY KEY AUTOINCREMENT, "
        "    task_no         TEXT    NOT NULL UNIQUE, "
        "    title           TEXT    NOT NULL, "
        "    category        TEXT, "
        "    source          TEXT, "
        "    tag             TEXT, "
        "    handler         TEXT    NOT NULL DEFAULT '本人', "
        "    status          TEXT    NOT NULL DEFAULT '进行中', "
        "    original_status TEXT, "
        "    folder_name     TEXT, "
        "    folder_path     TEXT, "
        "    is_key_task     INTEGER NOT NULL DEFAULT 0, "
        "    has_subdirs     INTEGER NOT NULL DEFAULT 0, "
        "    is_today_focus  INTEGER NOT NULL DEFAULT 0, "
        "    focus_order     INTEGER NOT NULL DEFAULT 0, "
        "    deadline        TEXT, "
        "    description     TEXT, "
        "    is_from_periodic INTEGER NOT NULL DEFAULT 0, "
        "    processing_days INTEGER NOT NULL DEFAULT 0, "
        "    create_time     TEXT    NOT NULL, "
        "    finish_time     TEXT, "
        "    delete_time     TEXT, "
        "    update_time     TEXT"
        ");"));
    if (!ok) {
        Logger::instance().error(QStringLiteral("建表 task 失败: %1").arg(query.lastError().text()));
    }
}

void DatabaseManager::createTableTaskRemark() {
    QSqlQuery query(db_);
    bool ok = query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS task_remark ("
        "    id          INTEGER PRIMARY KEY AUTOINCREMENT, "
        "    task_id     INTEGER NOT NULL, "
        "    content     TEXT    NOT NULL, "
        "    create_time TEXT    NOT NULL, "
        "    FOREIGN KEY (task_id) REFERENCES task(id) ON DELETE CASCADE"
        ");"));
    if (!ok) {
        Logger::instance().error(QStringLiteral("建表 task_remark 失败: %1").arg(query.lastError().text()));
    }
}

void DatabaseManager::createTableAchievement() {
    QSqlQuery query(db_);
    bool ok = query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS achievement ("
        "    id              INTEGER PRIMARY KEY AUTOINCREMENT, "
        "    task_id         INTEGER NOT NULL, "
        "    work_content    TEXT    NOT NULL, "
        "    work_object     TEXT, "
        "    result_count    INTEGER NOT NULL DEFAULT 0, "
        "    result_amount   INTEGER NOT NULL DEFAULT 0, "
        "    description     TEXT, "
        "    is_annual       INTEGER NOT NULL DEFAULT 0, "
        "    is_key_achievement INTEGER NOT NULL DEFAULT 0, "
        "    create_time     TEXT    NOT NULL, "
        "    FOREIGN KEY (task_id) REFERENCES task(id) ON DELETE CASCADE"
        ");"));
    if (!ok) {
        Logger::instance().error(QStringLiteral("建表 achievement 失败: %1").arg(query.lastError().text()));
    }
}

void DatabaseManager::createTablePeriodicTask() {
    QSqlQuery query(db_);
    bool ok = query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS periodic_task ("
        "    id                  INTEGER PRIMARY KEY AUTOINCREMENT, "
        "    name                TEXT    NOT NULL, "
        "    category            TEXT, "
        "    period_type         TEXT    NOT NULL, "
        "    period_value        INTEGER NOT NULL DEFAULT 1, "
        "    source              TEXT, "
        "    tag                 TEXT, "
        "    handler             TEXT    NOT NULL DEFAULT '本人', "
        "    reminder_time       TEXT, "
        "    last_reminded_date  TEXT, "
        "    is_enabled          INTEGER NOT NULL DEFAULT 1, "
        "    create_time         TEXT    NOT NULL, "
        "    target_day          INTEGER NOT NULL DEFAULT 0"
        ");"));
    if (!ok) {
        Logger::instance().error(QStringLiteral("建表 periodic_task 失败: %1").arg(query.lastError().text()));
    }
}

void DatabaseManager::createTableSourceDict() {
    QSqlQuery query(db_);
    bool ok = query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS source_dict ("
        "    id          INTEGER PRIMARY KEY AUTOINCREMENT, "
        "    name        TEXT    NOT NULL UNIQUE, "
        "    is_enabled  INTEGER NOT NULL DEFAULT 1, "
        "    sort_order  INTEGER NOT NULL DEFAULT 0, "
        "    create_time TEXT    NOT NULL"
        ");"));
    if (!ok) {
        Logger::instance().error(QStringLiteral("建表 source_dict 失败: %1").arg(query.lastError().text()));
    }
}

void DatabaseManager::createTableTagDict() {
    QSqlQuery query(db_);
    bool ok = query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS tag_dict ("
        "    id          INTEGER PRIMARY KEY AUTOINCREMENT, "
        "    name        TEXT    NOT NULL UNIQUE, "
        "    is_enabled  INTEGER NOT NULL DEFAULT 1, "
        "    sort_order  INTEGER NOT NULL DEFAULT 0, "
        "    create_time TEXT    NOT NULL"
        ");"));
    if (!ok) {
        Logger::instance().error(QStringLiteral("建表 tag_dict 失败: %1").arg(query.lastError().text()));
    }
}

void DatabaseManager::createTableHandlerDict() {
    QSqlQuery query(db_);
    bool ok = query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS handler_dict ("
        "    id          INTEGER PRIMARY KEY AUTOINCREMENT, "
        "    name        TEXT    NOT NULL UNIQUE, "
        "    is_enabled  INTEGER NOT NULL DEFAULT 1, "
        "    sort_order  INTEGER NOT NULL DEFAULT 0, "
        "    create_time TEXT    NOT NULL"
        ");"));
    if (!ok) {
        Logger::instance().error(QStringLiteral("建表 handler_dict 失败: %1").arg(query.lastError().text()));
    }
}

void DatabaseManager::createTableSettings() {
    QSqlQuery query(db_);
    bool ok = query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS settings ("
        "    key         TEXT    PRIMARY KEY, "
        "    value       TEXT, "
        "    update_time TEXT"
        ");"));
    if (!ok) {
        Logger::instance().error(QStringLiteral("建表 settings 失败: %1").arg(query.lastError().text()));
    }
}

void DatabaseManager::createTableOperationLog() {
    QSqlQuery query(db_);
    bool ok = query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS operation_log ("
        "    id          INTEGER PRIMARY KEY AUTOINCREMENT, "
        "    task_id     INTEGER, "
        "    action      TEXT    NOT NULL, "
        "    description TEXT, "
        "    operator    TEXT    NOT NULL DEFAULT '本人', "
        "    create_time TEXT    NOT NULL, "
        "    FOREIGN KEY (task_id) REFERENCES task(id) ON DELETE SET NULL"
        ");"));
    if (!ok) {
        Logger::instance().error(QStringLiteral("建表 operation_log 失败: %1").arg(query.lastError().text()));
    }
}

void DatabaseManager::createTableReferenceMaterial() {
    QSqlQuery query(db_);
    bool ok = query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS reference_material ("
        "    id          INTEGER PRIMARY KEY AUTOINCREMENT, "
        "    ref_no      TEXT    NOT NULL UNIQUE, "
        "    name        TEXT    NOT NULL, "
        "    source      TEXT, "
        "    tag         TEXT, "
        "    description TEXT, "
        "    folder_name TEXT, "
        "    folder_path TEXT, "
        "    create_time TEXT    NOT NULL, "
        "    update_time TEXT"
        ");"));
    if (!ok) {
        Logger::instance().error(QStringLiteral("建表 reference_material 失败: %1").arg(query.lastError().text()));
    }
}

// ============================================================================
// 索引
// ============================================================================

void DatabaseManager::createIndexes() {
    QStringList indexSqls = {
        QStringLiteral("CREATE INDEX IF NOT EXISTS idx_task_status ON task(status);"),
        QStringLiteral("CREATE INDEX IF NOT EXISTS idx_task_task_no ON task(task_no);"),
        QStringLiteral("CREATE INDEX IF NOT EXISTS idx_task_create_time ON task(create_time);"),
        QStringLiteral("CREATE INDEX IF NOT EXISTS idx_task_source ON task(source);"),
        QStringLiteral("CREATE INDEX IF NOT EXISTS idx_task_tag ON task(tag);"),
        QStringLiteral("CREATE INDEX IF NOT EXISTS idx_task_handler ON task(handler);"),
        QStringLiteral("CREATE INDEX IF NOT EXISTS idx_task_remark_task_id ON task_remark(task_id);"),
        QStringLiteral("CREATE INDEX IF NOT EXISTS idx_achievement_task_id ON achievement(task_id);"),
        QStringLiteral("CREATE INDEX IF NOT EXISTS idx_operation_log_task_id ON operation_log(task_id);"),
        QStringLiteral("CREATE INDEX IF NOT EXISTS idx_operation_log_create_time ON operation_log(create_time);"),
        QStringLiteral("CREATE INDEX IF NOT EXISTS idx_reference_material_ref_no ON reference_material(ref_no);")
    };
    for (const QString &sql : indexSqls) {
        QSqlQuery query(db_);
        if (!query.exec(sql)) {
            Logger::instance().error(QStringLiteral("创建索引失败 [%1]: %2").arg(sql, query.lastError().text()));
        }
    }
}

// ============================================================================
// 迁移：为已有 periodic_task 表添加 target_day 列
// ============================================================================

void DatabaseManager::migratePeriodicTaskAddTargetDay() {
    // CREATE TABLE IF NOT EXISTS 不会修改已存在的表结构
    // 已有数据库需要通过 ALTER TABLE 添加新列
    QSqlQuery checkQuery(db_);
    if (!checkQuery.exec(QStringLiteral("PRAGMA table_info(periodic_task)"))) {
        Logger::instance().warn(QStringLiteral("检查 periodic_task 表结构失败: %1")
            .arg(checkQuery.lastError().text()));
        return;
    }
    bool hasTargetDay = false;
    while (checkQuery.next()) {
        if (checkQuery.value(1).toString() == QStringLiteral("target_day")) {
            hasTargetDay = true;
            break;
        }
    }
    if (!hasTargetDay) {
        QSqlQuery alterQuery(db_);
        if (!alterQuery.exec(QStringLiteral(
                "ALTER TABLE periodic_task ADD COLUMN target_day INTEGER NOT NULL DEFAULT 0"))) {
            Logger::instance().error(QStringLiteral("添加 target_day 列失败: %1")
                .arg(alterQuery.lastError().text()));
        } else {
            Logger::instance().info(QStringLiteral("已为 periodic_task 表添加 target_day 列"));
        }
    }
}

// ============================================================================
// seedInitialData（幂等：先查询是否已存在再插入）
// ============================================================================

void DatabaseManager::seedInitialData() {
    seedSourceDict();
    seedTagDict();
    seedHandlerDict();
    seedPeriodicTaskTemplates();
    seedDefaultSettings();
}

void DatabaseManager::seedSourceDict() {
    // 预置来源字典（幂等：INSERT OR IGNORE）
    // 注意：不禁用用户自定义来源，仅插入标准条目
    QStringList sources = {
        QStringLiteral("征收管理股"),
        QStringLiteral("风险管理股"),
        QStringLiteral("税政股"),
        QStringLiteral("纳税服务"),
        QStringLiteral("法制股"),
        QStringLiteral("收入核算股"),
        QStringLiteral("党委办公室"),
        QStringLiteral("人事股"),
        QStringLiteral("税源管理"),
        QStringLiteral("社保和非税股"),
        QStringLiteral("办公室")
    };
    QString now = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));

    // 插入缺失的条目（INSERT OR IGNORE 保证幂等）
    for (int i = 0; i < sources.size(); ++i) {
        QSqlQuery insertQuery(db_);
        insertQuery.prepare(QStringLiteral(
            "INSERT OR IGNORE INTO source_dict (name, is_enabled, sort_order, create_time) "
            "VALUES (?, 1, ?, ?)"));
        insertQuery.addBindValue(sources[i]);
        insertQuery.addBindValue(i);
        insertQuery.addBindValue(now);
        if (!insertQuery.exec()) {
            Logger::instance().error(QStringLiteral("插入来源字典失败: %1").arg(insertQuery.lastError().text()));
        }
    }
}

void DatabaseManager::seedTagDict() {
    // 预置标签字典（幂等：INSERT OR IGNORE）
    QStringList tags = {
        QStringLiteral("股权转让"),
        QStringLiteral("欠税管理")
    };
    QString now = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));

    for (int i = 0; i < tags.size(); ++i) {
        QSqlQuery insertQuery(db_);
        insertQuery.prepare(QStringLiteral(
            "INSERT OR IGNORE INTO tag_dict (name, is_enabled, sort_order, create_time) "
            "VALUES (?, 1, ?, ?)"));
        insertQuery.addBindValue(tags[i]);
        insertQuery.addBindValue(i);
        insertQuery.addBindValue(now);
        if (!insertQuery.exec()) {
            Logger::instance().error(QStringLiteral("插入标签字典失败: %1").arg(insertQuery.lastError().text()));
        }
    }
}

void DatabaseManager::seedHandlerDict() {
    // 预置办理人字典（幂等：INSERT OR IGNORE）
    // "非本人"仅在他人办理且具体办理人未知时作为兜底值。
    QStringList handlers = {
        QStringLiteral("本人"),
        QStringLiteral("非本人")
    };
    QString now = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));

    for (int i = 0; i < handlers.size(); ++i) {
        QSqlQuery insertQuery(db_);
        insertQuery.prepare(QStringLiteral(
            "INSERT OR IGNORE INTO handler_dict (name, is_enabled, sort_order, create_time) "
            "VALUES (?, 1, ?, ?)"));
        insertQuery.addBindValue(handlers[i]);
        insertQuery.addBindValue(i);
        insertQuery.addBindValue(now);
        if (!insertQuery.exec()) {
            Logger::instance().error(QStringLiteral("插入办理人字典失败: %1").arg(insertQuery.lastError().text()));
        }
    }
}

void DatabaseManager::seedPeriodicTaskTemplates() {
    // 预置 3 条周期任务模板（幂等）：日报/周报/月报
    struct PeriodicTemplate {
        QString name;
        QString periodType;
        int periodValue;
    };
    QList<PeriodicTemplate> templates = {
        { QStringLiteral("日报"), QStringLiteral("daily"), 1 },
        { QStringLiteral("周报"), QStringLiteral("weekly"), 1 },
        { QStringLiteral("月报"), QStringLiteral("monthly"), 1 }
    };
    QString now = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));

    for (const PeriodicTemplate &t : templates) {
        // 先查询是否已存在（参数化查询）
        QSqlQuery checkQuery(db_);
        checkQuery.prepare(QStringLiteral("SELECT COUNT(*) FROM periodic_task WHERE name = ?"));
        checkQuery.addBindValue(t.name);
        if (!checkQuery.exec()) {
            Logger::instance().error(QStringLiteral("查询周期任务模板失败: %1").arg(checkQuery.lastError().text()));
            continue;
        }
        if (checkQuery.next() && checkQuery.value(0).toInt() > 0) {
            continue;  // 已存在，跳过
        }
        // 插入（参数化查询）
        QSqlQuery insertQuery(db_);
        insertQuery.prepare(QStringLiteral(
            "INSERT INTO periodic_task (name, category, period_type, period_value, source, tag, handler, "
            "reminder_time, last_reminded_date, is_enabled, create_time) "
            "VALUES (?, ?, ?, ?, NULL, NULL, '本人', NULL, NULL, 1, ?)"));
        insertQuery.addBindValue(t.name);
        insertQuery.addBindValue(QStringLiteral("报"));  // 默认分类"报"
        insertQuery.addBindValue(t.periodType);
        insertQuery.addBindValue(t.periodValue);
        insertQuery.addBindValue(now);
        if (!insertQuery.exec()) {
            Logger::instance().error(QStringLiteral("插入周期任务模板失败: %1").arg(insertQuery.lastError().text()));
        }
    }
}

void DatabaseManager::seedDefaultSettings() {
    // 默认 settings（幂等）：work_dir=空、db_version=1、last_focus_clear_date=今天、last_backup_date=空
    QString today = QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd"));
    QString now = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));

    struct SettingItem {
        QString key;
        QString value;
    };
    QList<SettingItem> defaultSettings = {
        { QStringLiteral("work_dir"), QString() },                          // 空
        { QStringLiteral("db_version"), QString::number(kCurrentVersion) }, // "1"
        { QStringLiteral("last_focus_clear_date"), today },                  // 今天
        { QStringLiteral("last_backup_date"), QString() }                    // 空
    };

    for (const SettingItem &item : defaultSettings) {
        // 先查询是否已存在（参数化查询）
        QSqlQuery checkQuery(db_);
        checkQuery.prepare(QStringLiteral("SELECT COUNT(*) FROM settings WHERE key = ?"));
        checkQuery.addBindValue(item.key);
        if (!checkQuery.exec()) {
            Logger::instance().error(QStringLiteral("查询设置项失败: %1").arg(checkQuery.lastError().text()));
            continue;
        }
        if (checkQuery.next() && checkQuery.value(0).toInt() > 0) {
            continue;  // 已存在，跳过
        }
        // 插入（参数化查询）
        QSqlQuery insertQuery(db_);
        insertQuery.prepare(QStringLiteral(
            "INSERT INTO settings (key, value, update_time) VALUES (?, ?, ?)"));
        insertQuery.addBindValue(item.key);
        insertQuery.addBindValue(item.value);
        insertQuery.addBindValue(now);
        if (!insertQuery.exec()) {
            Logger::instance().error(QStringLiteral("插入设置项失败: %1").arg(insertQuery.lastError().text()));
        }
    }
}

// ============================================================================
// runMigrations
// ============================================================================

void DatabaseManager::runMigrations() {
    // 检查 settings 表的 db_version
    QSqlQuery query(db_);
    query.prepare(QStringLiteral("SELECT value FROM settings WHERE key = ?"));
    query.addBindValue(QStringLiteral("db_version"));
    if (!query.exec()) {
        Logger::instance().warn(QStringLiteral("查询 db_version 失败: %1").arg(query.lastError().text()));
        return;
    }

    int dbVersion = 0;
    if (query.next()) {
        dbVersion = query.value(0).toInt();
    }

    // 全新数据库（dbVersion == 0）：表已是最新 schema，无需迁移
    // 已有数据库版本低于当前版本：执行迁移
    if (dbVersion > 0 && dbVersion < kCurrentVersion) {
        Logger::instance().info(QStringLiteral("数据库需要从版本 %1 迁移到 %2")
            .arg(dbVersion).arg(kCurrentVersion));
        // 目前 kCurrentVersion = 1，无迁移逻辑
        // 未来版本在此添加迁移逻辑：
        // if (dbVersion < 2) { /* v1 -> v2 迁移 */ }
        // if (dbVersion < 3) { /* v2 -> v3 迁移 */ }
    }
}
