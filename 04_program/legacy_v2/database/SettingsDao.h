/**
 * @file SettingsDao.h
 * @brief 系统设置数据访问层（无状态），提供 settings 表的读写方法
 *
 * 依赖：QtSql（QSqlDatabase/QSqlQuery）、utils/Logger、utils/DateUtil
 * 文档：task.md T8.10（SettingsDao 实现）、TD 2.7（settings 表 schema）、TD 3.6.1（SettingsDao 接口）、coding.md 第五章（数据库访问规范）
 *
 * 关键约束：
 *  - 全部 SQL 使用参数化查询（QSqlQuery::prepare + addBindValue），禁止字符串拼接
 *  - DAO 无状态，db_ 为构造时传入的连接，不持有业务状态
 *  - 时间格式：yyyy-MM-dd HH:mm:ss
 *  - settings 表 schema: key TEXT PRIMARY KEY, value TEXT, update_time TEXT
 *  - setValue 使用 INSERT OR REPLACE 语义（key 存在则更新，不存在则插入）
 *  - getValue 找不到 key 时返回 defaultValue，不报错
 *  - 供 Phase 3 FileService 读取 work_dir 等配置项使用
 */
#pragma once

#include <QSqlDatabase>
#include <QString>

class SettingsDao {
public:
    explicit SettingsDao(QSqlDatabase db);

    // 读取配置项（key 不存在时返回 defaultValue，不报错）
    QString getValue(const QString &key, const QString &defaultValue = QString()) const;

    // 写入配置项（key 存在则更新，不存在则插入；update_time 刷新为当前时间）
    bool setValue(const QString &key, const QString &value);

    // 判断配置项是否存在
    bool hasKey(const QString &key) const;

    // 删除配置项（key 不存在时返回 false）
    bool removeKey(const QString &key);

private:
    QSqlDatabase db_;
};
