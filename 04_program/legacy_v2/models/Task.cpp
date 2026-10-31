/**
 * @file Task.cpp
 * @brief Task 结构体便捷方法实现
 *
 * 文档：TD 3.8、coding.md 6.2（办理天数计算）
 */
#include "Task.h"
#include "utils/DateUtil.h"

bool Task::isOverdue() const
{
    if (!deadline.isValid()) {
        return false;
    }
    return deadline < QDate::currentDate();
}

int Task::daysLeft() const
{
    if (!deadline.isValid()) {
        return INT_MAX;  // 无截止日期返回最大值
    }
    return DateUtil::daysLeft(deadline);
}
