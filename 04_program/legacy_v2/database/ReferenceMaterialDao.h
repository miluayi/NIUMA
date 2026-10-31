/**
 * @file ReferenceMaterialDao.h
 * @brief 参考资料数据访问层（无状态），提供 reference_material 表的 CRUD、筛选查询、编号序列等方法
 *
 * 依赖：QtSql（QSqlDatabase/QSqlQuery）、models/ReferenceMaterial、utils/Logger
 * 文档：TD 2.9（reference_material 表）、TD 3.5（ReferenceMaterialDao 接口）、coding.md 第五章（数据库访问规范）
 *
 * 关键约束：
 *  - 全部 SQL 使用参数化查询（QSqlQuery::prepare + addBindValue），禁止字符串拼接
 *  - DAO 无状态，db_ 为构造时传入的连接，不持有业务状态
 *  - create_time/update_time 使用 ISO 格式存储（QDateTime::toString(Qt::ISODate)）
 *  - 物理删除（无回收站）
 *  - getMaxSequenceForDate 同时查询 task 和 reference_material 表（共用每日序列）
 *  - keyword 在 name 和 description 两个字段中搜索（LIKE '%keyword%'）
 */
#pragma once

#include <QSqlDatabase>
#include <QList>
#include <QStringList>
#include <optional>

#include "models/ReferenceMaterial.h"
#include "models/ReferenceFilter.h"

class ReferenceMaterialDao {
public:
    explicit ReferenceMaterialDao(QSqlDatabase db);

    // 按 ID 查询
    std::optional<ReferenceMaterial> findById(int id);

    // 按编号查询
    std::optional<ReferenceMaterial> findByRefNo(const QString &refNo);

    // 按筛选条件查询（含分页）
    QList<ReferenceMaterial> findByFilter(const ReferenceFilter &filter);

    // 按筛选条件查询总数
    int countByFilter(const ReferenceFilter &filter);

    // 查询全部（按创建时间倒序）
    QList<ReferenceMaterial> findAll();

    // 插入参考资料（返回新 ID，失败返回 -1）
    int insert(const ReferenceMaterial &ref);

    // 更新参考资料（更新除 id、create_time 外的所有字段）
    bool update(const ReferenceMaterial &ref);

    // 删除（物理删除，无回收站）
    bool deleteById(int id);

    // 获取指定日期的最大序列号（与 task 共用同一每日序列）
    // dateStr 格式：yyyyMMdd
    int getMaxSequenceForDate(const QString &dateStr);

private:
    QSqlDatabase db_;

    // 辅助：从 QSqlQuery 读取一行构造 ReferenceMaterial
    ReferenceMaterial refFromQuery(QSqlQuery &query);

    // 辅助：构造 WHERE 子句和绑定参数（已含 WHERE 前缀，无条件时返回空串）
    QString buildWhereClause(const ReferenceFilter &filter, QStringList &bindValues);
};
