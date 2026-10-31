/**
 * @file ToastWidget.cpp
 * @brief 轻提示组件实现（T8.11）
 *
 * 依赖：ui/widgets/ToastWidget.h、ui/MainWindow.h
 * 文档：task.md T8.11、qt_guide Prompt 6、2.4
 */

#include "ToastWidget.h"

#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QPaintEvent>
#include <QPainter>
#include <QPropertyAnimation>

#include "MainWindow.h"

// 静态成员初始化
QList<ToastWidget *> ToastWidget::activeToasts_;

// ============================================================================
// 构造 / 析构
// ============================================================================

ToastWidget::ToastWidget(QWidget *parent)
    : QWidget(parent)
    , animation_(nullptr)
    , opacityEffect_(nullptr)
{
    // 不抢焦点、不阻塞用户操作（鼠标点击穿透到下层控件）
    setAttribute(Qt::WA_TransparentForMouseEvents);
    // 不自动填充背景（由 paintEvent 绘制圆角矩形背景）
    setAutoFillBackground(false);
}

ToastWidget::~ToastWidget()
{
    // 从活跃列表移除并重新定位剩余 Toast
    activeToasts_.removeAll(this);
    if (parentWidget()) {
        repositionToasts(parentWidget());
    }
}

// ============================================================================
// 静态方法：显示 Toast
// ============================================================================

void ToastWidget::show(MainWindow *parent, Type type, const QString &message)
{
    if (!parent) {
        return;
    }

    ToastWidget *toast = new ToastWidget(parent);
    toast->setupContent(type, message);
    toast->adjustSize();

    // 加入活跃列表并定位
    activeToasts_.append(toast);
    repositionToasts(parent);

    // 显示并置顶（使用 QWidget:: 限定，避免被静态 show 方法遮蔽）
    toast->QWidget::show();
    toast->QWidget::raise();

    // 启动淡入淡出动画
    toast->animation_->start();
}

// ============================================================================
// 内容设置
// ============================================================================

void ToastWidget::setupContent(Type type, const QString &message)
{
    QString symbol;
    QColor bgColor;
    QColor foreground = QColor("#FFFFFF");

    switch (type) {
    case Success:
        bgColor = QColor("#D1FAE5");
        foreground = QColor("#166534");
        symbol = QStringLiteral("\u2713");  // ✓
        break;
    case Error:
        bgColor = QColor("#DC2626");
        symbol = QStringLiteral("\u2715");  // ✕
        break;
    case Info:
        bgColor = QColor("#475569");
        symbol = QStringLiteral("\u2139");  // ℹ
        break;
    case Warning:
        bgColor = QColor("#EAB308");
        symbol = QStringLiteral("\u26A0");  // ⚠
        break;
    }

    bgColor_ = bgColor;

    // 布局：符号 + 消息
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(20, 10, 20, 10);  // padding: 10px 20px
    layout->setSpacing(8);

    // 符号标签
    QLabel *symbolLabel = new QLabel(symbol, this);
    symbolLabel->setProperty("appUiNoLabelReset", true);
    symbolLabel->setStyleSheet(QStringLiteral(
        "color:%1;"
        "font-size:16px;"
        "font-family:'MiSans';"
        "border:none;"
        "background:transparent;"
    ).arg(foreground.name()));
    layout->addWidget(symbolLabel);

    // 消息标签（长文本自动换行）
    QLabel *messageLabel = new QLabel(message, this);
    messageLabel->setProperty("appUiNoLabelReset", true);
    messageLabel->setWordWrap(message.size() > 16);
    messageLabel->setMaximumWidth(360);  // 400(最大宽度) - 40(左右padding)
    messageLabel->setStyleSheet(QStringLiteral(
        "color:%1;"
        "font-size:15px;"
        "font-family:'MiSans';"
        "border:none;"
        "background:transparent;"
    ).arg(foreground.name()));
    layout->addWidget(messageLabel, 1);

    // 限制 Toast 最大宽度
    setMaximumWidth(400);

    // 透明度效果（用于淡入淡出动画）
    opacityEffect_ = new QGraphicsOpacityEffect(this);
    opacityEffect_->setOpacity(0.0);
    setGraphicsEffect(opacityEffect_);

    // 淡入淡出动画：0.0 → 1.0 → 1.0 → 0.0，总时长 3000ms
    animation_ = new QPropertyAnimation(opacityEffect_, "opacity", this);
    animation_->setDuration(3000);
    animation_->setKeyValueAt(0.0, 0.0);   // 起始：透明
    animation_->setKeyValueAt(0.1, 1.0);   // 300ms 淡入完成
    animation_->setKeyValueAt(0.9, 1.0);   // 保持显示至 2700ms
    animation_->setKeyValueAt(1.0, 0.0);   // 300ms 淡出完成

    // 动画结束后自动销毁
    connect(animation_, &QPropertyAnimation::finished, this, &QObject::deleteLater);
}

// ============================================================================
// 重新定位所有活跃 Toast
// ============================================================================

void ToastWidget::repositionToasts(QWidget *parent)
{
    if (!parent) {
        return;
    }

    // 清理已无效的指针
    activeToasts_.removeAll(nullptr);

    int parentWidth = parent->width();
    int parentHeight = parent->height();
    const int bottomMargin = 48;     // 距底部 48px
    const int toastGap = 8;          // Toast 间距

    int count = activeToasts_.size();
    for (int i = 0; i < count; ++i) {
        ToastWidget *t = activeToasts_[i];
        int toastWidth = t->width();
        int toastHeight = t->height();

        // 水平居中
        int x = (parentWidth - toastWidth) / 2;
        // 垂直堆叠：第一个在底部，后续向上排列
        int y = parentHeight - bottomMargin - toastHeight - i * (toastHeight + toastGap);

        t->move(x, y);
    }
}

// ============================================================================
// 绘制圆角背景
// ============================================================================

void ToastWidget::paintEvent(QPaintEvent * /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(bgColor_);
    painter.drawRoundedRect(rect(), 8, 8);
}
