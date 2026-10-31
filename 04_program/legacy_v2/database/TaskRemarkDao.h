/**
 * @file TaskRemarkDao.h
 * @brief 任务备注数据访问层（无状态），提供 task_remark 表的 CRUD 方法
 *
 * 依赖：QtSql（QSqlDatabase/QSqlQuery）、models/TaskRemark、utils/Logger、utils/DateUtil
 * 文档：TD 2.10（task_remark 表 schema）、TD 3.2（TaskRemarkDao 接口）、coding.md 第五章（数据库访问规范）、task.md T2.2
 *
 * 关键约束：
 *  - 全部 SQL 使用参数化查询（QSqlQuery::prepare + addBindValue），禁止字符串拼接
 *  - DAO 无状态，db_ 为构造时传入的连接，不持有业务状态
 *  - 时间格式：yyyy-MM-dd HH:mm:ss
 *  - findByTaskId 按 create_time ASC 排序
 *  - 外键 ON DELETE CASCADE 会随 task 删除自动清理，deleteByTaskId 提供显式物理删除入口
 */
#pragma once

#include <QSqlDatabase>
#include <QList>
#include <optional>

#include "models/TaskRemark.h"

class TaskRemarkDao {
public:
    explicit TaskRemarkDao(QSqlDatabase db);

    // 按 task_id 查询备注（按 create_time ASC 排序）
    QList<TaskRemark> findByTaskId(int taskId);

    // 插入备注（返回新 ID，失败返回 -1）
    int insert(const TaskRemark &remark);

    // 更新备注内容
    bool update(const TaskRemark &remark);

    // 按 ID 删除（物理删除）
    bool deleteById(int id);

    // 按 task_id 删除全部备注（物理删除，外键 CASCADE 也会处理）
    bool deleteByTaskId(int taskId);

private:
    QSqlDatabase db_;

    // 辅助：从 QSqlQuery 读取一行构造 TaskRemark
    TaskRemark remarkFromQuery(QSqlQuery &query);
};
