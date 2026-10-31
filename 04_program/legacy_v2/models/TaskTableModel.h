/**
 * @file TaskTableModel.h
 * @brief 任务表格数据模型（QAbstractTableModel 子类），10 列
 *
 * 列顺序：编号 / 任务名称 / 分类 / 来源 / 标签 / 办理人 / 状态 / 截止 / 完成日期 / 天数
 * data() 只返回数据文本，不着色。着色由 TaskItemDelegate 处理。
 * UserRole 返回 Task 结构体，供 delegate 获取 isKeyTask / deadline 等额外信息。
 *
 * 依赖：Qt5 Core（QAbstractTableModel）、models/Task
 * 文档：task.md T1.9、qt_implementation_guide.md 3.3、coding.md 第九章
 */
#pragma once

#include <QAbstractTableModel>
#include <QList>

#include "models/Task.h"

class TaskTableModel : public QAbstractTableModel {
    Q_OBJECT

public:
    enum Column {
        ColTaskNo = 0,
        ColTitle,
        ColCategory,
        ColSource,
        ColTag,
        ColHandler,
        ColStatus,
        ColDeadline,
        ColFinishTime,
        ColDays,
        ColumnCount
    };

    explicit TaskTableModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    void setTasks(const QList<Task> &tasks);
    void clear();
    Task getTask(int row) const;
    int getTaskId(int row) const;

private:
    QList<Task> tasks_;

    /**
     * @brief 计算办理天数显示文本
     * - 进行中：已办理天数（daysBetween(createTime, now) + 1）
     * - 已完成：总周期（processingDays）
     * - 他人办理："—"
     */
    QString daysText(const Task &task) const;
};
