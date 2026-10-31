/**
 * @file TitleBar.cpp
 * @brief 自定义标题栏实现
 *
 * 高度 52px，包含 NIUMA 标识、导航、搜索、新建和窗口控制按钮。
 * 窗口控制按钮使用等尺寸矢量线条，不依赖字体中的符号字形。
 *
 * 依赖：ui/widgets/TitleBar.h
 * 文档：PRD 14.1、qt_implementation_guide.md 3.1、coding.md 7.1
 */
#include "TitleBar.h"

#include <QFont>
#include <QAction>
#include <QIcon>
#include <QAbstractButton>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPushButton>
#include <QPainter>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <QStyleOptionButton>

namespace {
class AppLogoLabel final : public QLabel {
public:
    explicit AppLogoLabel(QWidget *parent) : QLabel(parent), icon_(":/icons/app-icon.svg") {}
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        icon_.paint(&painter, rect(), Qt::AlignCenter, QIcon::Normal);
    }
private:
    QIcon icon_;
};

class WindowControlButton final : public QPushButton {
public:
    enum Glyph { Minimize, Maximize, Close };
    WindowControlButton(Glyph glyph, QWidget *parent) : QPushButton(parent), glyph_(glyph) {}
protected:
    void paintEvent(QPaintEvent *event) override {
        Q_UNUSED(event)
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        // Do not delegate pressed/focused backgrounds to the OS button theme.
        if (isEnabled() && (isDown() || underMouse())) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(isDown() ? QColor(glyph_==Close ? "#D8DEE8" : "#E1E7F0") : QColor("#F5F7FA"));
            painter.drawRoundedRect(rect(),4,4);
        }
        QStyleOptionButton option;
        initStyleOption(&option);
        if ((option.state & QStyle::State_HasFocus) && (option.state & QStyle::State_KeyboardFocusChange)) {
            painter.setPen(QPen(QColor("#2463EB"),1));
            painter.setBrush(Qt::NoBrush);
            painter.drawRoundedRect(QRectF(rect()).adjusted(.5,.5,-.5,-.5),4,4);
        }
        if (glyph_ == Close) {
            static const QIcon closeIcon(":/icons/window-close.svg");
            closeIcon.paint(&painter, QRect((width()-14)/2, (height()-14)/2, 14, 14),
                            Qt::AlignCenter, QIcon::Normal);
            return;
        }
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(QPen(QColor("#171717"), 1));
        painter.setBrush(Qt::NoBrush);
        painter.translate((width()-10)/2.0+.5, (height()-10)/2.0+.5);
        if (glyph_ == Minimize) painter.drawLine(QPointF(0,5), QPointF(9,5));
        else if (property("windowControlRestore").toBool()) {
            painter.drawLine(QPointF(2,1), QPointF(2,0));
            painter.drawLine(QPointF(2,0), QPointF(9,0));
            painter.drawLine(QPointF(9,0), QPointF(9,7));
            painter.drawLine(QPointF(9,7), QPointF(8,7));
            painter.drawRect(QRectF(0,2,7,7));
        } else painter.drawRect(QRectF(0,0,9,9));
    }
private:
    Glyph glyph_;
};
}

TitleBar::TitleBar(QWidget *parent)
    : QWidget(parent)
    , titleLabel_(nullptr)
    , btnMinimize_(nullptr)
    , btnMaximize_(nullptr)
    , btnClose_(nullptr)
    , btnBack_(nullptr)
    , btnForward_(nullptr)
    , btnRefresh_(nullptr)
    , btnCreate_(nullptr)
    , searchEdit_(nullptr)
{
    setFixedHeight(52);
    setObjectName("titlebar");

    // NIUMA icon (blue rounded square with "N")
    QLabel *iconLabel = new AppLogoLabel(this);
    iconLabel->setObjectName("titleIcon");
    iconLabel->setFixedSize(30, 30);
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setProperty("appUiNoLabelReset", true);
    iconLabel->setStyleSheet("background:transparent; border:none; padding:0;");

    // NIUMA keeps the product mark regular-weight and reserves the same
    // title slot as the approved shell so navigation controls align consistently.
    titleLabel_ = new QLabel(QStringLiteral("NIUMA"), this);
    QFont titleFont(QStringLiteral("MiSans"), 17, QFont::Normal);
    titleLabel_->setFont(titleFont);
    titleLabel_->setFixedWidth(126);
    titleLabel_->setProperty("appUiNoLabelReset", true);
    titleLabel_->setStyleSheet("color:#000000; padding-left:10px; font-size:17pt;");

    // Window control buttons
    btnMinimize_ = new WindowControlButton(WindowControlButton::Minimize, this);
    btnMaximize_ = new WindowControlButton(WindowControlButton::Maximize, this);
    btnClose_ = new WindowControlButton(WindowControlButton::Close, this);
    btnMinimize_->setAccessibleName(QStringLiteral("最小化"));
    btnMaximize_->setAccessibleName(QStringLiteral("最大化"));
    btnClose_->setAccessibleName(QStringLiteral("关闭"));

    btnMinimize_->setObjectName("btnMinimize");
    btnMaximize_->setObjectName("btnMaximize");
    btnMaximize_->setToolTip(QStringLiteral("最大化"));
    btnClose_->setObjectName("btnClose");

    btnBack_ = new QPushButton(QStringLiteral("‹"), this);
    btnForward_ = new QPushButton(QStringLiteral("›"), this);
    btnRefresh_ = new QPushButton(QStringLiteral("↻"), this);
    btnBack_->setObjectName(QStringLiteral("shellBack"));
    btnForward_->setObjectName(QStringLiteral("shellForward"));
    btnRefresh_->setObjectName(QStringLiteral("shellRefresh"));
    for (QPushButton *button : {btnBack_, btnForward_, btnRefresh_}) {
        button->setFixedSize(32, 36);
        button->setCursor(Qt::PointingHandCursor);
    }
    btnBack_->setToolTip(QStringLiteral("后退"));
    btnForward_->setToolTip(QStringLiteral("前进"));
    btnRefresh_->setToolTip(QStringLiteral("刷新当前页面"));

    searchEdit_ = new QLineEdit(this);
    searchEdit_->setObjectName(QStringLiteral("shellSearch"));
    searchEdit_->setPlaceholderText(QStringLiteral("搜索名称、编号或附件名，回车搜索"));
    auto *clearSearch = searchEdit_->addAction(QIcon(":/icons/clear-red.svg"), QLineEdit::TrailingPosition);
    clearSearch->setObjectName("shellSearchClearAction");
    clearSearch->setToolTip(QStringLiteral("清除搜索"));
    clearSearch->setVisible(false);
    connect(clearSearch, &QAction::triggered, searchEdit_, &QLineEdit::clear);
    connect(searchEdit_, &QLineEdit::textChanged, clearSearch, [clearSearch](const QString &text) {
        clearSearch->setVisible(!text.isEmpty());
    });
    searchEdit_->setFixedHeight(38);
    searchEdit_->setMinimumWidth(210);
    searchEdit_->setMaximumWidth(340);

    btnCreate_ = new QPushButton(QStringLiteral("＋ 新建"), this);
    btnCreate_->setObjectName(QStringLiteral("shellCreate"));
    btnCreate_->setFixedSize(82, 38);
    btnCreate_->setCursor(Qt::PointingHandCursor);

    btnMinimize_->setFixedSize(46, 32);
    btnMaximize_->setFixedSize(46, 32);
    btnClose_->setFixedSize(46, 32);

    // Keep the window control glyphs black in every state.
    setStyleSheet(
        "QWidget#titlebar { background:#F5F6FA; border:none; }"
        "QPushButton#btnMinimize, QPushButton#btnMaximize {"
        "  border: none;"
        "  background: transparent;"
        "  font-size: 15px;"
        "  color: #000000;"
        "  border-radius: 4px;"
        "}"
        "QPushButton#btnMinimize:hover, QPushButton#btnMaximize:hover {"
        "  background: #F5F7FA;"
        "}"
        "QPushButton#btnMinimize:pressed, QPushButton#btnMaximize:pressed {"
        "  background: #E1E7F0;"
        "}"
        "QPushButton#btnClose {"
        "  border: none;"
        "  background: transparent;"
        "  font-size: 15px;"
        "  color: #000000;"
        "  border-radius: 4px;"
        "}"
        "QPushButton#btnClose:hover {"
        "  background: #F5F7FA;"
        "}"
        "QPushButton#btnClose:pressed {"
        "  background: #D8DEE8;"
        "}"
    );

    // Layout: icon + title on left, stretch, buttons on right (对齐 HTML 原型)
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(20, 0, 12, 0);
    layout->setSpacing(8);
    layout->addWidget(iconLabel);
    layout->addWidget(titleLabel_);
    layout->addWidget(btnBack_);
    layout->addWidget(btnForward_);
    layout->addWidget(btnRefresh_);
    layout->addWidget(searchEdit_, 1);
    layout->addWidget(btnCreate_);
    layout->addStretch();
    auto *group = new QWidget(this);
    group->setObjectName("windowControlsGroup");
    group->setFixedSize(138, 39);
    auto *column = new QVBoxLayout(group);
    column->setContentsMargins(0, 7, 0, 0);
    column->setSpacing(0);
    auto *controls = new QHBoxLayout;
    controls->setContentsMargins(0, 0, 0, 0);
    controls->setSpacing(0);
    controls->addWidget(btnMinimize_);
    controls->addWidget(btnMaximize_);
    controls->addWidget(btnClose_);
    column->addLayout(controls);
    layout->addWidget(group, 0, Qt::AlignTop);

    // Signal connections
    connect(btnMinimize_, &QPushButton::clicked, this, &TitleBar::minimizeClicked);
    connect(btnMaximize_, &QPushButton::clicked, this, &TitleBar::maximizeClicked);
    connect(btnClose_, &QPushButton::clicked, this, &TitleBar::closeClicked);
    connect(btnBack_, &QPushButton::clicked, this, &TitleBar::navigateBackClicked);
    connect(btnForward_, &QPushButton::clicked, this, &TitleBar::navigateForwardClicked);
    connect(btnRefresh_, &QPushButton::clicked, this, &TitleBar::refreshClicked);
    connect(btnCreate_, &QPushButton::clicked, this, &TitleBar::createTaskClicked);
    connect(searchEdit_, &QLineEdit::returnPressed, this, [this]() {
        emit searchSubmitted(searchEdit_->text());
    });
}

TitleBar::~TitleBar() = default;

void TitleBar::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    // Keep equal visible clearance, reduced to 60% of the previous spacing.
    const int center = qRound((height()/2.0-5.0)*0.6+5.0);
    const int right = qMax(0, center-btnClose_->width()/2);
    if (layout() && layout()->contentsMargins().right() != right)
        layout()->setContentsMargins(20, 0, right, 0);
    if (auto *group = findChild<QWidget *>("windowControlsGroup", Qt::FindDirectChildrenOnly)) {
        const int top = qMax(0, center-btnClose_->height()/2);
        if (group->layout()->contentsMargins().top() != top)
            group->layout()->setContentsMargins(0, top, 0, 0);
        if (group->height() != top+btnClose_->height())
            group->setFixedHeight(top+btnClose_->height());
    }
}

void TitleBar::setMaximized(bool maximized)
{
    btnMaximize_->setProperty("windowControlRestore", maximized);
    btnMaximize_->setAccessibleName(maximized ? QStringLiteral("还原") : QStringLiteral("最大化"));
    btnMaximize_->update();
    btnMaximize_->setToolTip(maximized ? QStringLiteral("还原") : QStringLiteral("最大化"));
}

void TitleBar::setNavigationEnabled(bool canGoBack, bool canGoForward)
{
    btnBack_->setEnabled(canGoBack);
    btnForward_->setEnabled(canGoForward);
}

void TitleBar::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && !isInteractiveArea(event->pos())) {
        dragPos_ = event->globalPos() - window()->frameGeometry().topLeft();
        draggingTitleBar_ = true;
        event->accept();
        return;
    }
    draggingTitleBar_ = false;
    QWidget::mousePressEvent(event);
}

void TitleBar::mouseMoveEvent(QMouseEvent *event)
{
    if (draggingTitleBar_ && (event->buttons() & Qt::LeftButton)) {
        window()->move(event->globalPos() - dragPos_);
        event->accept();
        return;
    }
    QWidget::mouseMoveEvent(event);
}

void TitleBar::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        draggingTitleBar_ = false;
    }
    QWidget::mouseReleaseEvent(event);
}

void TitleBar::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && !isInteractiveArea(event->pos())) {
        emit maximizeClicked();
        event->accept();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}

bool TitleBar::isInteractiveArea(const QPoint &position) const
{
    QWidget *child = childAt(position);
    while (child && child != this) {
        if (qobject_cast<QAbstractButton *>(child)
            || qobject_cast<QLineEdit *>(child)) {
            return true;
        }
        child = child->parentWidget();
    }
    return false;
}
