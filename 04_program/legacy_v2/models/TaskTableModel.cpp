/**
 * @file TaskTableModel.cpp
 * @brief 任务表格数据模型实现
 *
 * 依赖：models/TaskTableModel.h、utils/DateUtil
 * 文档：task.md T1.9、qt_implementation_guide.md 3.3、coding.md 第九章
 */
#include "TaskTableModel.h"

#include <QDateTime>

#include "utils/DateUtil.h"

// ============================================================================
// 构造
// ============================================================================

TaskTableModel::TaskTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

// ============================================================================
// 行列数
// ============================================================================

int TaskTableModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return tasks_.size();
}

int TaskTableModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return ColumnCount;
}

// ============================================================================
// 数据
// ============================================================================

QVariant TaskTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }
    if (index.row() < 0 || index.row() >= tasks_.size()) {
        return QVariant();
    }
    if (index.column() < 0 || index.column() >= ColumnCount) {
        return QVariant();
    }

    const Task &task = tasks_.at(index.row());

    // UserRole：返回 Task 结构体，供 delegate 获取 isKeyTask / deadline 等信息
    if (role == Qt::UserRole) {
        return QVariant::fromValue(task);
    }

    // TextAlignmentRole：任务名称列左对齐，其他列居中对齐
    if (role == Qt::TextAlignmentRole) {
        if (index.column() == ColTitle) {
            return static_cast<int>(Qt::AlignVCenter | Qt::AlignLeft);
        }
        return static_cast<int>(Qt::AlignCenter);
    }

    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    switch (index.column()) {
        case ColTaskNo:
            return task.taskNo;
        case ColTitle:
            return task.title;
        case ColCategory:
            return task.category;
        case ColSource:
            return task.source;
        case ColTag:
            return task.tag;
        case ColHandler:
            return task.handler;
        case ColStatus:
            return task.status;
        case ColDeadline:
            if (task.deadline.isValid()) {
                if (task.status == QStringLiteral("进行中")
                    && task.deadline < QDate::currentDate()) return QStringLiteral("逾期");
                return task.deadline.toString("yyyy-MM-dd");
            }
            return QStringLiteral("\u2014");  // "—"
        case ColDays:
            return daysText(task);
        case ColFinishTime:
            return task.status == QStringLiteral("已完成") && task.finishTime.isValid()
                ? task.finishTime.toString("yyyy-MM-dd") : QStringLiteral("—");
        default:
            return QVariant();
    }
}

QVariant TaskTableModel::headerData(int section, Qt::Orientation orientation,
                                    int role) const
{
    if (role != Qt::DisplayRole) {
        return QVariant();
    }
    if (orientation != Qt::Horizontal) {
        return QVariant();
    }
    if (section < 0 || section >= ColumnCount) {
        return QVariant();
    }

    switch (section) {
        case ColTaskNo:   return QStringLiteral("\u7F16\u53F7");       // 编号
        case ColTitle:    return QStringLiteral("\u4EFB\u52A1\u540D\u79F0");  // 任务名称
        case ColCategory: return QStringLiteral("\u5206\u7C7B");       // 分类
        case ColSource:   return QStringLiteral("\u6765\u6E90");       // 来源
        case ColTag:      return QStringLiteral("\u6807\u7B7E");       // 标签
        case ColHandler:  return QStringLiteral("\u529E\u7406\u4EBA"); // 办理人
        case ColStatus:   return QStringLiteral("\u72B6\u6001");       // 状态
        case ColDeadline: return QStringLiteral("\u622A\u6B62");       // 截止
        case ColDays:     return QStringLiteral("\u5929\u6570");       // 天数
        case ColFinishTime: return QStringLiteral("完成日期");
        default:          return QVariant();
    }
}

// ============================================================================
// 数据更新
// ============================================================================

void TaskTableModel::setTasks(const QList<Task> &tasks)
{
    beginResetModel();
    tasks_ = tasks;
    endResetModel();
}

void TaskTableModel::clear()
{
    beginResetModel();
    tasks_.clear();
    endResetModel();
}

Task TaskTableModel::getTask(int row) const
{
    if (row < 0 || row >= tasks_.size()) {
        return Task();
    }
    return tasks_.at(row);
}

int TaskTableModel::getTaskId(int row) const
{
    if (row < 0 || row >= tasks_.size()) {
        return -1;
    }
    return tasks_.at(row).id;
}

// ============================================================================
// 私有辅助
// ============================================================================

QString TaskTableModel::daysText(const Task &task) const
{
    // 他人办理：显示 "—"
    if (task.status == QStringLiteral("\u4ED6\u4EBA\u529E\u7406")) {
        return QStringLiteral("\u2014");  // "—"
    }
    // 已完成：显示总周期（processingDays）
    if (task.status == QStringLiteral("\u5DF2\u5B8C\u6210")) {
        if (task.processingDays > 0) {
            return QString::number(task.processingDays);
        }
        // 若 processingDays 为 0，则按 createTime 到 finishTime 计算
        if (task.createTime.isValid() && task.finishTime.isValid()) {
            int days = DateUtil::daysBetween(task.createTime, task.finishTime) + 1;
            return QString::number(days);
        }
        return QStringLiteral("\u2014");
    }
    // 进行中：已办理天数（daysBetween(createTime, now) + 1）
    if (task.status == QStringLiteral("\u8FDB\u884C\u4E2D")) {
        if (task.createTime.isValid()) {
            int days = DateUtil::daysBetween(task.createTime, QDateTime::currentDateTime()) + 1;
            return QString::number(days);
        }
        return QStringLiteral("1");
    }
    // 其他状态（如已删除）
    return QStringLiteral("\u2014");
}
