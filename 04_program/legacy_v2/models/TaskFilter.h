/**
 * @file TaskFilter.h
 * @brief 任务列表筛选条件数据模型
 *
 * 文档：TD 3.8、PRD 5.4、coding.md 5.5（TaskFilter 状态映射）
 * 约束：状态映射 进行中/已完成/已删除/他人办理，空=全部
 */
#pragma once
#include <QString>
#include <QDate>

struct TaskFilter {
    QString keyword;          // 搜索关键词（任务名称/编号）
    QString category;         // 分类筛选（空=全部）
    QString source;           // 来源筛选（空=全部）
    QString tag;              // 标签筛选（空=全部）
    QString handler;          // 办理人筛选（空=全部，非本人=所有办理人不为本人的任务）
    QString status;           // 状态筛选（空=全部/进行中/已完成/已删除/他人办理）
    QDate startDate;          // 开始日期（空=不限）
    QDate endDate;            // 结束日期（空=不限）
    int year = 0;            // 创建年度（0=全部；由列表页面设置默认本年）
    int page = 1;             // 当前页
    int pageSize = 10;        // 每页条数（任务列表 10）
    bool sortByDeadline = false; // 首页进行中任务：有效截止日升序、无截止日最后

    bool hasDateRange() const { return startDate.isValid() || endDate.isValid(); }
    bool isEmpty() const;     // 所有筛选条件为空
    void reset();             // 重置为默认
};
