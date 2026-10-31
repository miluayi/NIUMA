/**
 * @file TaskDao.h
 * @brief 任务数据访问层（无状态），提供 task 表的 CRUD、筛选查询、编号序列、统计等方法
 *
 * 依赖：QtSql（QSqlDatabase/QSqlQuery）、models/Task、models/TaskFilter、utils/Logger
 * 文档：TD 3.1（TaskDao 接口）、TD 5.1（编号规则）、coding.md 第五章（数据库访问规范）、task.md T1.7
 *
 * 关键约束：
 *  - 全部 SQL 使用参数化查询（QSqlQuery::prepare + addBindValue），禁止字符串拼接
 *  - DAO 无状态，db_ 为构造时传入的连接，不持有业务状态
 *  - status 使用直接字符串值 '进行中'/'已完成'/'已删除'/'他人办理'
 *  - getMaxSequenceForDate 必须包含 status='已删除' 行防止序列回绕（不添加额外 WHERE 条件）
 *  - activeTaskCondition() 和 buildOrderByClause() 为全项目唯一出处
 */
#pragma once

#include <QSqlDatabase>
#include <QList>
#include <QDate>
#include <QStringList>
#include <optional>

#include "models/Task.h"
#include "models/TaskFilter.h"
#include "models/Statistics.h"

class TaskDao {
public:
    explicit TaskDao(QSqlDatabase db);

    // 按 ID 查询
    std::optional<Task> findById(int id);

    // 按编号查询
    std::optional<Task> findByTaskNo(const QString &taskNo);

    // 按筛选条件查询（含分页）
    QList<Task> findByFilter(const TaskFilter &filter);

    // 按筛选条件查询总数
    int countByFilter(const TaskFilter &filter);

    // 查询进行中任务（首页用，按创建时间倒序），limit=0 表示不限
    QList<Task> findActive(int limit = 0);

    // 查询今日专注任务（status='进行中' AND is_today_focus=1，按 focus_order 排序）
    QList<Task> findTodayFocus();

    // 查询已删除任务（回收站用，按删除时间倒序）
    QList<Task> findDeleted();

    // 查询去重的来源列表（从已存在任务中，排除已删除和空值）
    QStringList getDistinctSources();

    // 查询去重的标签列表（从已存在任务中，排除已删除和空值）
    QStringList getDistinctTags();
    QStringList getDistinctHandlers();
    QList<int> getDistinctYears();

    // 查询日期范围内的任务（统计用）
    QList<Task> findByDateRange(const QDate &start, const QDate &end);

    // 插入任务（返回新 ID，失败返回 -1）
    int insert(const Task &task);

    // 更新任务（更新除 id、create_time 外的所有字段）
    bool update(const Task &task);

    // 更新状态
    bool updateStatus(int id, const QString &status);

    // 更新办理人（办理人变更时）
    bool updateHandler(int id, const QString &handler);

    // 更新重点任务标记
    bool updateKeyTask(int id, bool isKey);

    // 更新今日专注
    bool updateTodayFocus(int id, bool focused, int focusOrder = 0);

    // 删除（物理删除）
    bool deleteById(int id);

    // 获取指定日期的最大序列号（必须包含 status='已删除' 行防止回绕）
    // dateStr 格式：yyyyMMdd
    int getMaxSequenceForDate(const QString &dateStr);

    // 统计相关
    int countByStatus(const QString &status);
    int countByDateRange(const QDate &start, const QDate &end);

    // Phase 6 统计方法
    // 按日期范围汇总统计（create_time 在范围内，排除已删除）
    TaskStats getStatsByDateRange(const QDate &start, const QDate &end);
    // 分类分布（按 count 倒序）
    QList<CategoryStat> getCategoryDistribution(const QDate &start, const QDate &end);
    // 来源分布（按 count 倒序）
    QList<SourceStat> getSourceDistribution(const QDate &start, const QDate &end);
    // 月度趋势（固定返回 12 条，无数据月份 count=0）
    QList<MonthlyTrend> getMonthlyTrend(int year);
    // 热力图数据（按日期分组，dimension 为 "create_time" 或 "finish_time"）
    QList<HeatmapData> getHeatmapData(int year, const QString &dimension = "create_time");

    // 进行中任务条件（唯一出处）
    static QString activeTaskCondition();

    // 排序子句（三档排序，唯一出处）
    static QString buildOrderByClause();

    // 最近一次 SQL 错误信息（insert/update 失败后可读取，用于上层向用户展示具体原因）
    QString lastError() const;

private:
    QSqlDatabase db_;
    QString lastError_;

    // 辅助：从 QSqlQuery 读取一行构造 Task
    Task taskFromQuery(QSqlQuery &query);

    // 辅助：构造 WHERE 子句和绑定参数（已含 WHERE 前缀，无条件时返回空串）
    QString buildWhereClause(const TaskFilter &filter, QStringList &bindValues);
};
