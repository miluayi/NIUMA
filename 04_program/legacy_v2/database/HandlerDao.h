/**
 * @file HandlerDao.h
 * @brief 办理人字典数据访问层（无状态），提供 handler_dict 表的 CRUD 与逻辑删除
 *
 * 关键约束：
 *  - 全部 SQL 使用参数化查询
 *  - 逻辑删除（setEnabled 设置 is_enabled=0），历史任务保留原记录
 *  - 物理删除（deleteById）前需调用 isUsed 检查是否被 task 引用
 *  - task.handler 存储 name（TEXT），非外键 id
 */
#pragma once

#include <QSqlDatabase>
#include <QList>
#include <QString>
#include <optional>

#include "models/Handler.h"

class HandlerDao {
public:
    explicit HandlerDao(QSqlDatabase db);

    // 查询全部（按 id 正序）
    QList<Handler> findAll();

    // 查询启用的办理人（is_enabled=1，按 sort_order、id 正序）
    QList<Handler> findEnabled();
    QList<Handler> findEnabledByRecentUse();

    // 按 ID 查询
    std::optional<Handler> findById(int id);

    // 按名称查询
    std::optional<Handler> findByName(const QString &name);

    // 插入办理人（返回新 ID，失败返回 -1）
    int insert(const Handler &handler);

    // 更新办理人（更新 name 和 sort_order）
    bool update(const Handler &handler);

    // 逻辑删除/启用（设置 is_enabled）
    bool setEnabled(int id, bool enabled);

    // 物理删除（调用前需确认 isUsed 返回 false）
    bool deleteById(int id);

    // 检查是否被 task 表引用（按 name 匹配）
    bool isUsed(int id);

private:
    QSqlDatabase db_;

    // 辅助：从 QSqlQuery 读取一行构造 Handler
    Handler handlerFromQuery(QSqlQuery &query);
};
