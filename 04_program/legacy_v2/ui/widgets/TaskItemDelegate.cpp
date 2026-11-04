/**
 * @file TaskItemDelegate.cpp
 * @brief 任务表格自定义委托实现
 *
 * 绘制规范（coding.md 9.2）：
 *   - 分类列：催 #FEF3C7/#B45309、查 #EFF6FF/#1D4ED8、报 #FED7AA/#C2410C、办 #DCFCE7/#15803D
 *   - 状态列：进行中 #FDE68A/#92400E、已完成 #BBF7D0/#14532D、他人办理 #E2E8F0/#475569、已删除 #FECACA/#991B1B
 *   - 任务名称列：【重点】前缀 #FA423E + 任务名称
 *   - 截止列：daysLeft <= 3 → #DC2626 加粗；无效 → 灰色 "—"
 *   - 编号列：Segoe UI 字体，14px
 *
 * 依赖：ui/widgets/TaskItemDelegate.h、models/Task、utils/DateUtil
 * 文档：task.md T1.9、qt_implementation_guide.md 3.3、coding.md 9.2
 */
#include "TaskItemDelegate.h"

#include <QDate>
#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QStyle>
#include <QStyleOptionViewItem>
#include <QTextDocument>

#include "models/Task.h"
#include "models/TaskTableModel.h"
#include "utils/DateUtil.h"

// ============================================================================
// 构造
// ============================================================================

TaskItemDelegate::TaskItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

CopyTextLayout TaskItemDelegate::copyTextLayout(const QStyleOptionViewItem &option,const QModelIndex &index) const
{
    QFont font(QStringLiteral("Segoe UI"));
    font.setPixelSize(14);
    font.setWeight(QFont::Normal);
    font.setStyleName(QStringLiteral("Normal"));
    QString text=index.data().toString();
    QRect rect=option.rect;
    Qt::Alignment alignment=Qt::AlignCenter;
    if (index.column()==TaskTableModel::ColTitle) {
        const Task task=index.data(Qt::UserRole).value<Task>();
        text=(task.isKeyTask ? QStringLiteral("【重点】") : QString())+task.title;
        rect.adjust(14,0,-14,0);
        alignment=Qt::AlignLeft|Qt::AlignVCenter;
    }
    if(index.column()==TaskTableModel::ColCategory || index.column()==TaskTableModel::ColStatus) {
        const int width=qMax(28,QFontMetrics(font).horizontalAdvance(text)+16);
        rect=QRect(rect.center().x()-width/2,rect.center().y()-12,width,24);
    }
    return {text,font,rect,alignment,false,0};
}

// ============================================================================
// paint
// ============================================================================

void TaskItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                             const QModelIndex &index) const
{
    if (!index.isValid()) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    // 获取 UserRole 中的 Task 结构体
    QVariant userVar = index.data(Qt::UserRole);
    Task task;
    if (userVar.isValid() && userVar.canConvert<Task>()) {
        task = userVar.value<Task>();
    }

    int col = index.column();

    // 先绘制背景（选中态/悬停态）
    painter->save();
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    // 绘制选中/悬停背景
    if (option.state & QStyle::State_Selected) {
        painter->fillRect(option.rect, QColor("#F5F7FA"));
    } else if (option.state & QStyle::State_MouseOver) {
        painter->fillRect(option.rect, QColor("#F5F7FA"));
    }

    // 绘制底部分隔线（对应 QSS border-bottom: 1px solid #F1F5F9）
    painter->setPen(QPen(QColor("#F1F5F9"), 1));
    painter->drawLine(option.rect.bottomLeft(), option.rect.bottomRight());
    painter->restore();

    // 内容区域（减去 padding）
    QRect contentRect = option.rect.adjusted(14, 0, -14, 0);

    switch (col) {
        case TaskTableModel::ColTaskNo:
            paintTaskNo(painter, option.rect, task.taskNo);
            break;
        case TaskTableModel::ColTitle:
            paintTitle(painter, contentRect, task.title, task.isKeyTask);
            break;
        case TaskTableModel::ColCategory:
            paintCategory(painter, option.rect, task.category);
            break;
        case TaskTableModel::ColSource:
        case TaskTableModel::ColTag:
        case TaskTableModel::ColFinishTime:
        case TaskTableModel::ColHandler: {
            // 普通文本，居中对齐
            QString text = index.data(Qt::DisplayRole).toString();
            painter->save();
            painter->setPen(QColor("#0F172A"));
            QFont font = option.font;
            font.setPixelSize(14);
            painter->setFont(font);
            painter->drawText(option.rect, Qt::AlignCenter, text);
            painter->restore();
            break;
        }
        case TaskTableModel::ColStatus:
            paintStatus(painter, option.rect, task.status);
            break;
        case TaskTableModel::ColDeadline:
            paintDeadline(painter, option.rect,
                          task.status == QStringLiteral("进行中") ? task.deadline : QDate(),
                          index.data(Qt::DisplayRole).toString());
            break;
        case TaskTableModel::ColDays: {
            // 天数列：居中显示
            QString text = index.data(Qt::DisplayRole).toString();
            painter->save();
            painter->setPen(QColor("#0F172A"));
            QFont font = option.font;
            font.setPixelSize(14);
            painter->setFont(font);
            painter->drawText(option.rect, Qt::AlignCenter, text);
            painter->restore();
            break;
        }
        default:
            QStyledItemDelegate::paint(painter, option, index);
            break;
    }
}

// ============================================================================
// sizeHint
// ============================================================================

QSize TaskItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                                 const QModelIndex &index) const
{
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    // 行高 48px（与 HTML 原型对齐：padding 12px + 内容 24px）
    if (size.height() < 48) {
        size.setHeight(48);
    }
    return size;
}

// ============================================================================
// 私有：分类列绘制（pill 标签）
// ============================================================================

void TaskItemDelegate::paintCategory(QPainter *painter, const QRect &rect,
                                     const QString &category) const
{
    paintCategoryBadge(painter, rect, category);
}

int TaskItemDelegate::categoryBadgeWidth(const QString &category)
{
    QFont font(QStringLiteral("Segoe UI"));
    font.setPixelSize(14);
    font.setWeight(QFont::Normal);
    font.setStyleName(QStringLiteral("Normal"));
    return qMax(28, QFontMetrics(font).horizontalAdvance(category) + 16);
}

void TaskItemDelegate::paintCategoryBadge(QPainter *painter, const QRect &rect,
                                          const QString &category)
{
    if (category.isEmpty()) return;
    paintPill(painter, rect, category, categoryBgColor(category),
              categoryTextColor(category));
}

// ============================================================================
// 私有：状态列绘制（pill 标签）
// ============================================================================

void TaskItemDelegate::paintStatus(QPainter *painter, const QRect &rect,
                                   const QString &status) const
{
    if (status.isEmpty()) {
        return;
    }
    QColor bg = statusBgColor(status);
    QColor fg = statusTextColor(status);
    paintPill(painter, rect, status, bg, fg);
}

// ============================================================================
// 私有：任务名称列绘制（重点前缀 + 富文本）
// ============================================================================

void TaskItemDelegate::paintTitle(QPainter *painter, const QRect &rect,
                                  const QString &title, bool isKeyTask) const
{
    painter->save();
    painter->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

    QTextDocument doc;
    QFont titleFont(QStringLiteral("Segoe UI"));
    titleFont.setPixelSize(14);
    titleFont.setWeight(QFont::Normal);
    titleFont.setStyleName(QStringLiteral("Normal"));
    doc.setDefaultFont(titleFont);
    doc.setDocumentMargin(0);

    QString html;
    if (isKeyTask) {
        // 【重点】前缀：红色 #FA423E + 任务名称
        html = QStringLiteral(
            "<span style=\"color:#FF0000;font-family:Segoe UI;font-size:14px;font-weight:400\">"
            "\u3010\u91CD\u70B9\u3011</span>"  // 【重点】
            "<span style=\"color:#0F172A;font-family:Segoe UI;font-size:14px;font-weight:400\">%1</span>"
        ).arg(title.toHtmlEscaped());
    } else {
        html = QStringLiteral(
            "<span style=\"color:#0F172A;font-family:Segoe UI;font-size:14px;font-weight:400\">%1</span>"
        ).arg(title.toHtmlEscaped());
    }
    doc.setHtml(html);

    // 垂直居中
    int docHeight = doc.size().height();
    int yOffset = rect.top() + (rect.height() - docHeight) / 2;
    if (yOffset < rect.top()) {
        yOffset = rect.top();
    }

    painter->translate(rect.left(), yOffset);
    doc.drawContents(painter, QRect(0, 0, rect.width(), rect.height()));
    painter->restore();
}

// ============================================================================
// 私有：截止日期列绘制
// ============================================================================

void TaskItemDelegate::paintDeadline(QPainter *painter, const QRect &rect,
                                     const QDate &deadline, const QString &text) const
{
    painter->save();

    QFont font = painter->font();
    font.setPixelSize(14);

    if (!deadline.isValid()) {
        // 空值使用灰色占位符；非进行中任务的日期使用普通文本色。
        painter->setPen(QColor(text == QStringLiteral("—") ? "#94A3B8" : "#0F172A"));
        painter->setFont(font);
        painter->drawText(rect, Qt::AlignCenter, text);
    } else {
        int left = DateUtil::daysLeft(deadline);
        if (left <= 1) {
            font.setWeight(QFont::Normal);
            painter->setPen(QColor("#FF0000"));
        } else if (left <= 3) {
            font.setWeight(QFont::Normal);
            painter->setPen(QColor("#FAAD14"));
        } else {
            // 正常显示
            painter->setPen(QColor("#0F172A"));
        }
        painter->setFont(font);
        painter->drawText(rect, Qt::AlignCenter, text);
    }

    painter->restore();
}

// ============================================================================
// 私有：编号列绘制（等宽字体）
// ============================================================================

void TaskItemDelegate::paintTaskNo(QPainter *painter, const QRect &rect,
                                   const QString &taskNo) const
{
    painter->save();
    QFont font(QStringLiteral("Segoe UI"));
    font.setPixelSize(14);
    font.setStyleName(QStringLiteral("Normal"));
    painter->setFont(font);
    painter->setPen(QColor("#0F172A"));
    painter->drawText(rect, Qt::AlignCenter, taskNo);
    painter->restore();
}

// ============================================================================
// 私有：pill 标签绘制
// ============================================================================

void TaskItemDelegate::paintPill(QPainter *painter, const QRect &rect,
                                 const QString &text, const QColor &bgColor,
                                 const QColor &textColor)
{
    painter->save();
    painter->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

    QFont font = painter->font();
    font.setFamily(QStringLiteral("Segoe UI"));
    font.setPixelSize(14);
    font.setWeight(QFont::Normal);
    font.setStyleName(QStringLiteral("Normal"));
    painter->setFont(font);

    // 测量文本宽度
    QFontMetrics fm(font);
    int textWidth = fm.horizontalAdvance(text);
    int pillHeight = 24;
    int pillWidth = textWidth + 16;  // 左右各 8px padding
    if (pillWidth < 28) {
        pillWidth = 28;
    }

    // 垂直居中，水平居中
    int pillX = rect.center().x() - pillWidth / 2;
    int pillY = rect.center().y() - pillHeight / 2;

    QRect pillRect(pillX, pillY, pillWidth, pillHeight);

    // 绘制圆角背景
    painter->setPen(Qt::NoPen);
    painter->setBrush(bgColor);
    painter->drawRoundedRect(pillRect, 6, 6);  // Radius::SM = 6

    // 绘制文字
    painter->setPen(textColor);
    painter->drawText(pillRect, Qt::AlignCenter, text);

    painter->restore();
}

// ============================================================================
// 私有：颜色映射
// ============================================================================

QColor TaskItemDelegate::categoryBgColor(const QString &category)
{
    if (category == QStringLiteral("\u50AC")) {  // 催
        return QColor("#C586C0");
    }
    if (category == QStringLiteral("\u67E5")) {  // 查
        return QColor("#DCDCAA");
    }
    if (category == QStringLiteral("\u62A5")) {  // 报
        return QColor("#569CD6");
    }
    if (category == QStringLiteral("\u529E")) {  // 办
        return QColor("#6A9955");
    }
    return QColor("#EFF6FF");  // 默认蓝调
}

QColor TaskItemDelegate::categoryTextColor(const QString &category)
{
    Q_UNUSED(category);
    return QColor("#000000");
}

QColor TaskItemDelegate::statusBgColor(const QString &status) const
{
    // 进行中: #FDE68A / 已完成: #BBF7D0 / 他人办理: #E2E8F0 / 已删除: #FECACA
    if (status == QStringLiteral("\u8FDB\u884C\u4E2D")) {  // 进行中
        return QColor("#FFF3C4");
    }
    if (status == QStringLiteral("\u5DF2\u5B8C\u6210")) {  // 已完成
        return QColor("#DDF8E7");
    }
    if (status == QStringLiteral("\u4ED6\u4EBA\u529E\u7406")) {  // 他人办理
        return QColor("#E2E8F0");
    }
    if (status == QStringLiteral("\u5DF2\u5220\u9664")) {  // 已删除
        return QColor("#FFE7E6");
    }
    return QColor("#E2E8F0");
}

QColor TaskItemDelegate::statusTextColor(const QString &status) const
{
    // 进行中: #92400E / 已完成: #14532D / 他人办理: #475569 / 已删除: #991B1B
    if (status == QStringLiteral("\u8FDB\u884C\u4E2D")) {  // 进行中
        return QColor("#92400E");
    }
    if (status == QStringLiteral("\u5DF2\u5B8C\u6210")) {  // 已完成
        return QColor("#14532D");
    }
    if (status == QStringLiteral("\u4ED6\u4EBA\u529E\u7406")) {  // 他人办理
        return QColor("#475569");
    }
    if (status == QStringLiteral("\u5DF2\u5220\u9664")) {  // 已删除
        return QColor("#991B1B");
    }
    return QColor("#475569");
}
