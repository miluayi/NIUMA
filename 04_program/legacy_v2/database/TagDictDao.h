/**
 * @file TagDictDao.h
 * @brief 事项标签字典数据访问层（无状态），提供 tag_dict 表的 CRUD 与逻辑删除
 *
 * 依赖：QtSql（QSqlDatabase/QSqlQuery）、models/TagDict、utils/Logger
 * 文档：task.md T8.9、TD 2.6（tag_dict 表 schema）、TD 3.3、TD 3.6.1、coding.md 第五章
 *
 * 关键约束：
 *  - 全部 SQL 使用参数化查询（QSqlQuery::prepare + addBindValue），禁止字符串拼接
 *  - DAO 无状态，db_ 为构造时传入的连接，不持有业务状态
 *  - 逻辑删除（setEnabled 设置 is_enabled=0），历史任务保留原记录
 *  - 物理删除（deleteById）前需调用 isUsed 检查是否被 task/reference_material 引用
 *  - task.tag 与 reference_material.tag 存储 name（TEXT），非外键 id
 *  - isUsed 通过 name 匹配 task.tag 和 reference_material.tag
 */
#pragma once

#include <QSqlDatabase>
#include <QList>
#include <QString>
#include <optional>

#include "models/TagDict.h"

class TagDictDao {
public:
    explicit TagDictDao(QSqlDatabase db);

    // 查询全部（按 id 正序）
    QList<TagDict> findAll();

    // 查询启用的标签（is_enabled=1，按 sort_order、id 正序）
    QList<TagDict> findEnabled();
    QList<TagDict> findEnabledByRecentUse();

    // 按 ID 查询
    std::optional<TagDict> findById(int id);

    // 按名称查询
    std::optional<TagDict> findByName(const QString &name);

    // 插入标签（返回新 ID，失败返回 -1）
    int insert(const TagDict &tag);

    // 更新标签（更新 name 和 sort_order）
    bool update(const TagDict &tag);

    // 逻辑删除/启用（设置 is_enabled）
    bool setEnabled(int id, bool enabled);

    // 物理删除（调用前需确认 isUsed 返回 false）
    bool deleteById(int id);

    // 检查是否被 task 或 reference_material 表引用（按 name 匹配）
    bool isUsed(int id);

private:
    QSqlDatabase db_;

    // 辅助：从 QSqlQuery 读取一行构造 TagDict
    TagDict tagFromQuery(QSqlQuery &query);
};
