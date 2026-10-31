/**
 * @file TaskFilter.cpp
 * @brief TaskFilter 结构体方法实现
 *
 * 文档：TD 3.8、coding.md 5.5
 */
#include "TaskFilter.h"

bool TaskFilter::isEmpty() const
{
    return keyword.isEmpty() &&
           category.isEmpty() &&
           source.isEmpty() &&
           tag.isEmpty() &&
           handler.isEmpty() &&
           status.isEmpty() &&
           year == 0 &&
           !startDate.isValid() &&
           !endDate.isValid();
}

void TaskFilter::reset()
{
    keyword.clear();
    category.clear();
    source.clear();
    tag.clear();
    handler.clear();
    status.clear();
    startDate = QDate();
    endDate = QDate();
    year = 0;
    page = 1;
    pageSize = 10;
    sortByDeadline = false;
}
