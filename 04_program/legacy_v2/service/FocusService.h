/**
 * @file FocusService.h
 * @brief 今日专注业务逻辑层（无状态），管理 task 表的 is_today_focus/focus_order 字段
 *
 * 依赖：database/TaskDao、database/SettingsDao、database/DatabaseManager、
 *       models/Task、utils/Logger
 * 文档：task.md T5（今日专注）、T5.4（跨天清零）、coding.md 第六章
 *
 * 关键约束：
 *  - FocusService 为普通 C++ 类，不继承 QObject，不持有 db_ 成员，所有 DAO 在方法内构造
 *  - 今日专注上限 3 个（kMaxFocusTasks），超出时拒绝新增
 *  - status 使用直接字符串值 '进行中'
 *  - 跨天清零：启动时与每分钟定时器检测 last_focus_clear_date，与 QDate::currentDate() 不同则清空所有专注
 *  - 全部 SQL 使用参数化查询（QSqlQuery::prepare + addBindValue），禁止字符串拼接
 */
#pragma once

#include <QList>
#include <QString>

#include "models/Task.h"

class FocusService {
public:
    struct FocusResult {
        bool ok = false;
        QString error;
    };

    FocusService() = default;
    ~FocusService() = default;

    /// 获取今日专注任务列表（status='进行中' AND is_today_focus=1，按 focus_order 排序）
    QList<Task> getTodayFocus();

    /**
     * @brief 设置任务的今日专注标记
     * @param taskId 任务 ID
     * @param focused true=加入专注，false=移出专注
     * - 加入时检查上限（kMaxFocusTasks=3），超出拒绝
     * - 移出始终允许
     */
    FocusResult setFocus(int taskId, bool focused);

    /**
     * @brief 调整专注任务顺序（拖拽排序用）
     * @param taskId 任务 ID
     * @param newOrder 新位置序号
     * @note 当前实现仅更新该任务的 focus_order，完整重排逻辑后续实现
     */
    FocusResult reorder(int taskId, int newOrder);

    /// 清空所有今日专注（跨天清零用）
    void clearAllTodayFocus();

    /// 判断自上次清零后是否已跨天
    bool isNewDay() const;

    /// 将 last_focus_clear_date 更新为今天
    void updateLastClearDate();

    /**
     * @brief 检测跨天并执行清零（启动与每分钟定时器调用）
     * @return true 表示执行了清零；false 表示未跨天，无需清零
     */
    bool clearIfNewDay();

private:
    static const int kMaxFocusTasks = 3;

    /// 统计当前今日专注任务数（status='进行中' AND is_today_focus=1）
    int countTodayFocus();
};
