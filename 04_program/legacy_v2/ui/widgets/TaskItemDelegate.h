/**
 * @file TaskItemDelegate.h
 * @brief 任务表格自定义委托，负责单元格着色和重点前缀绘制
 *
 * 绘制内容：
 *   - 分类列：圆角 pill 标签，按 催/查/报/办 着色
 *   - 状态列：圆角 pill 标签，按 进行中/已完成/他人办理/已删除 着色
 *   - 任务名称列：重点任务前缀【重点】（红色 #FA423E），QTextDocument 富文本
 *   - 截止日期列：daysLeft <= 3 红色 #DC2626 加粗，无效日期灰色 "—"
 *   - 编号列：Segoe UI，14px
 *   - 天数列：居中
 *
 * 依赖：Qt5 Widgets（QStyledItemDelegate）
 * 文档：task.md T1.9、qt_implementation_guide.md 3.3、coding.md 9.2
 */
#pragma once

#include <QStyledItemDelegate>

class TaskItemDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    explicit TaskItemDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;
    static int categoryBadgeWidth(const QString &category);
    static void paintCategoryBadge(QPainter *painter, const QRect &rect,
                                   const QString &category);

private:
    void paintCategory(QPainter *painter, const QRect &rect, const QString &category) const;
    void paintStatus(QPainter *painter, const QRect &rect, const QString &status) const;
    void paintTitle(QPainter *painter, const QRect &rect, const QString &title, bool isKeyTask) const;
    void paintDeadline(QPainter *painter, const QRect &rect, const QDate &deadline, const QString &text) const;
    void paintTaskNo(QPainter *painter, const QRect &rect, const QString &taskNo) const;
    static void paintPill(QPainter *painter, const QRect &rect, const QString &text,
                          const QColor &bgColor, const QColor &textColor);

    /**
     * @brief 返回分类对应的背景色
     * 催: #FEF3C7 / 查: #EFF6FF / 报: #FED7AA / 办: #DCFCE7
     */
    static QColor categoryBgColor(const QString &category);
    /**
     * @brief 返回分类对应的文字色
     * 催: #B45309 / 查: #1D4ED8 / 报: #C2410C / 办: #15803D
     */
    static QColor categoryTextColor(const QString &category);
    /**
     * @brief 返回状态对应的背景色
     * 进行中: #FDE68A / 已完成: #BBF7D0 / 他人办理: #E2E8F0 / 已删除: #FECACA
     */
    QColor statusBgColor(const QString &status) const;
    /**
     * @brief 返回状态对应的文字色
     * 进行中: #92400E / 已完成: #14532D / 他人办理: #475569 / 已删除: #991B1B
     */
    QColor statusTextColor(const QString &status) const;
};
