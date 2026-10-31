/**
 * @file TaskNumberGenerator.cpp
 * @brief 任务编号生成器实现，格式 YYYYMMDD-NNN
 *
 * 依赖：database/DatabaseManager、database/TaskDao
 * 文档：TD 5.1（编号规则）、coding.md 6.3（编号生成规则）、task.md T1.7
 *
 * 关键约束：
 *  - 编号不复用：删除任务后当日序列出现空缺属正常，新建任务按当日最大序列号+1续编
 *  - getMaxSequenceForDate 已包含 status='已删除' 行防止回绕（在 TaskDao 中实现）
 *  - 任务和参考资料共用同一每日序列
 */

#include "TaskNumberGenerator.h"

#include "database/DatabaseManager.h"
#include "database/ReferenceMaterialDao.h"
#include "database/TaskDao.h"

QString TaskNumberGenerator::nextTaskNo()
{
    return nextTaskNoForDate(QDate::currentDate());
}

QString TaskNumberGenerator::nextRefNo()
{
    // 参考资料与任务共用同一每日序列
    return nextTaskNoForDate(QDate::currentDate());
}

QString TaskNumberGenerator::nextTaskNoForDate(const QDate &date)
{
    // 任务和参考资料共用同一每日序列，取两表 MAX 的较大值 +1
    // getMaxSequenceForDate 已包含 status='已删除' 行防止回绕
    TaskDao taskDao(DatabaseManager::instance().database());
    int taskMax = taskDao.getMaxSequenceForDate(date.toString("yyyyMMdd"));

    ReferenceMaterialDao refDao(DatabaseManager::instance().database());
    int refMax = refDao.getMaxSequenceForDate(date.toString("yyyyMMdd"));

    int nextSeq = qMax(taskMax, refMax) + 1;
    return format(date, nextSeq);
}

QString TaskNumberGenerator::format(const QDate &date, int seq)
{
    return date.toString("yyyyMMdd") + "-" + QString("%1").arg(seq, 3, 10, QChar('0'));
}
