/**
 * @file TaskDetailView.cpp
 * @brief 任务详情弹窗实现（T2.4）
 *
 * 实现：setupUI 搭建弹窗布局 / loadTaskData 加载任务数据 /
 *       loadRemarks 加载备注时间轴 / loadHistory 加载操作历史 /
 *       onCompleteClicked 完成/取消完成 / onDeleteClicked 删除任务 /
 *       onAddRemark 添加备注 / onRemarkDeleted 删除备注
 *
 * 依赖：ui/TaskDetailView.h、ui/ColorPalette.h、ui/Spacing.h、ui/Radius.h、
 *       application/task/TaskDetailUseCase、
 *       models/Task、models/TaskRemark、models/OperationLog、utils/Logger
 * 文档：PRD 5.5、qt_implementation_guide.md 3.4、task.md T2.4
 */

#include "TaskDetailView.h"

#include <QAbstractItemModel>
#include <QDesktopServices>
#include <QDir>
#include <QEvent>
#include <QFileInfo>
#include <QFont>
#include <QFontMetrics>
#include <QFrame>
#include <QGridLayout>
#include <QGuiApplication>
#include <QHash>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QRegion>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScreen>
#include <QSizePolicy>
#include <QStyledItemDelegate>
#include <QTextEdit>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QUrl>
#include <QVBoxLayout>
#include <functional>

#include "AchievementDialog.h"
#include "ColorPalette.h"
#include "Radius.h"
#include "Spacing.h"
#include "widgets/WindowDragger.h"
#include "widgets/StyledMessageBox.h"
#include "widgets/RoundedCornerFilter.h"
#include "application/task/TaskDetailUseCase.h"
#include "models/Achievement.h"
#include "models/OperationLog.h"
#include "models/Task.h"
#include "models/TaskRemark.h"
#include "utils/AmountUtil.h"
#include "utils/Logger.h"

// ============================================================================
// 时间轴自定义委托（文件内局部类，无需 Q_OBJECT）
// ============================================================================

/**
 * @brief 时间轴列表委托
 *
 * 绘制：左侧时间轴竖线 + 圆点 / 时间文字 / 内容文字（自动换行） / 右侧删除按钮（可选）
 * 数据角色：
 *   - Qt::DisplayRole  → 内容文字
 *   - Qt::UserRole     → 时间字符串
 *   - Qt::UserRole + 1 → remarkId（仅备注列表）
 */
class TimelineDelegate : public QStyledItemDelegate {
public:
    using DeleteCallback = std::function<void(int)>;

    explicit TimelineDelegate(bool showDeleteButton, QObject *parent = nullptr)
        : QStyledItemDelegate(parent)
        , showDeleteButton_(showDeleteButton)
    {}

    void setDeleteCallback(DeleteCallback cb) { deleteCb_ = std::move(cb); }

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);

        QRect rect = option.rect;

        // ---- 背景（hover/selected）----
        if (option.state & QStyle::State_Selected) {
            painter->fillRect(rect, QColor(0xF5, 0xF7, 0xFA));
        } else if (option.state & QStyle::State_MouseOver) {
            painter->fillRect(rect, QColor(0xF8, 0xFA, 0xFC));
        }

        // ---- 时间轴竖线 ----
        const int lineX = rect.left() + 8;
        painter->setPen(QPen(QColor(0xE2, 0xE8, 0xF0), 2));
        painter->drawLine(lineX, rect.top(), lineX, rect.bottom());

        // ---- 时间轴圆点 ----
        QRectF dotRect(lineX - 4, rect.top() + 6, 8, 8);
        painter->setBrush(QColor(0xEF, 0xF6, 0xFF));
        painter->setPen(QPen(QColor(0x24, 0x63, 0xEB), 2));
        painter->drawEllipse(dotRect);

        // ---- 文字区域 ----
        const int textX = rect.left() + 24;
        const int rightMargin = showDeleteButton_ ? 36 : 8;
        const int textWidth = rect.width() - 24 - rightMargin;

        // 时间文字
        QString timeText = index.data(Qt::UserRole).toString();
        QFont timeFont(QStringLiteral("MiSans"), 9);
        timeFont.setStyleName(QStringLiteral("Normal"));
        painter->setFont(timeFont);
        painter->setPen(QColor(0x94, 0xA3, 0xB8));
        QRect timeRect(textX, rect.top() + 4, textWidth, 18);
        painter->drawText(timeRect, Qt::AlignLeft | Qt::AlignVCenter, timeText);

        // 内容文字（自动换行）
        QString content = index.data(Qt::DisplayRole).toString();
        QFont contentFont(QStringLiteral("MiSans"), 10);
        contentFont.setStyleName(QStringLiteral("Normal"));
        QRect contentRect(textX, rect.top() + 24, textWidth, rect.height() - 28);
        if (!content.isEmpty()) {
            QTextDocument doc;
            doc.setDefaultFont(contentFont);
            doc.setPlainText(content);
            doc.setTextWidth(contentRect.width());
            // 设置文字颜色
            QTextCharFormat fmt;
            fmt.setForeground(QColor(0x47, 0x55, 0x69));  // #475569 次要文字色
            QTextCursor cursor(&doc);
            cursor.select(QTextCursor::Document);
            cursor.mergeCharFormat(fmt);

            painter->translate(contentRect.topLeft());
            doc.drawContents(painter, QRect(0, 0, contentRect.width(), contentRect.height()));
            painter->resetTransform();
        }

        // ---- 删除按钮 ----
        if (showDeleteButton_) {
            QRect btnRect = deleteButtonRect(rect);
            // 绘制删除按钮背景
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor(0xFE, 0xF2, 0xF2));
            painter->drawRoundedRect(btnRect, 4, 4);
            // 绘制 "−" 符号
            painter->setPen(QColor(0xDC, 0x26, 0x26));
            QFont btnFont(QStringLiteral("MiSans"), 13, QFont::Normal);
            btnFont.setStyleName(QStringLiteral("Normal"));
            painter->setFont(btnFont);
            painter->drawText(btnRect, Qt::AlignCenter, QStringLiteral("\u2212"));
        }

        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override
    {
        QString content = index.data(Qt::DisplayRole).toString();
        int availableWidth = option.rect.width();
        if (availableWidth <= 0) {
            availableWidth = 600;
        }
        int textWidth = availableWidth - 24 - (showDeleteButton_ ? 36 : 8);
        if (textWidth <= 0) {
            textWidth = 500;
        }

        QTextDocument doc;
        QFont contentFont(QStringLiteral("MiSans"), 10);
        contentFont.setStyleName(QStringLiteral("Normal"));
        doc.setDefaultFont(contentFont);
        doc.setPlainText(content);
        doc.setTextWidth(textWidth);

        int contentHeight = static_cast<int>(doc.size().height());
        int totalHeight = 4 + 18 + 4 + contentHeight + 8;
        return QSize(availableWidth, qMax(totalHeight, 44));
    }

    bool editorEvent(QEvent *event, QAbstractItemModel * /*model*/,
                     const QStyleOptionViewItem &option,
                     const QModelIndex &index) override
    {
        if (!showDeleteButton_ || !deleteCb_) {
            return false;
        }
        if (event->type() == QEvent::MouseButtonPress) {
            auto *mouseEvent = static_cast<QMouseEvent *>(event);
            QRect btnRect = deleteButtonRect(option.rect);
            if (btnRect.contains(mouseEvent->pos())) {
                int remarkId = index.data(Qt::UserRole + 1).toInt();
                deleteCb_(remarkId);
                return true;
            }
        }
        return false;
    }

private:
    bool showDeleteButton_;
    DeleteCallback deleteCb_;

    QRect deleteButtonRect(const QRect &itemRect) const {
        return QRect(itemRect.right() - 30, itemRect.top() + 3, 24, 24);
    }
};

// ============================================================================
// 构造
// ============================================================================

TaskDetailView::TaskDetailView(int taskId, QWidget *parent)
    : QDialog(parent)
    , taskId_(taskId)
{
    setupUI();
    refreshData();
}

void TaskDetailView::resizeEvent(QResizeEvent *event)
{
    QDialog::resizeEvent(event);
    // 矩形圆角遮罩
}

// ============================================================================
// UI 搭建
// ============================================================================

void TaskDetailView::setupUI()
{
    // 窗口属性：无边框、模态、可调整大小
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    installEventFilter(new RoundedCornerFilter(12, this));
    setModal(true);
    setMinimumWidth(760);
    setSizeGripEnabled(true);

    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 标题栏
    mainLayout->addWidget(buildHeader());

    // 内容区（可滚动）
    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setObjectName("detailScroll");
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setStyleSheet(
        "#detailScroll { background: #F8FAFC; border: none; }");

    QWidget *bodyWidget = new QWidget(scrollArea);
    bodyWidget->setObjectName("detailBody");
    bodyWidget->setStyleSheet("#detailBody { background: #F8FAFC; }");
    QVBoxLayout *bodyLayout = new QVBoxLayout(bodyWidget);
    bodyLayout->setContentsMargins(Spacing::XXL, Spacing::LG, Spacing::XXL, Spacing::XL);
    bodyLayout->setSpacing(Spacing::LG);

    bodyLayout->addWidget(buildBasicInfoSection());
    bodyLayout->addWidget(buildDescriptionSection());
    bodyLayout->addWidget(buildAttachmentSection());
    bodyLayout->addWidget(buildRemarksSection());
    bodyLayout->addWidget(buildHistorySection());
    bodyLayout->addWidget(buildAchievementSection());
    bodyLayout->addStretch();

    scrollArea->setWidget(bodyWidget);
    mainLayout->addWidget(scrollArea, 1);

    // 限制最大高度为屏幕的 82%
    int maxH = static_cast<int>(QGuiApplication::primaryScreen()->geometry().height() * 0.82);
    setMaximumHeight(maxH);
}

// ============================================================================
// 标题栏
// ============================================================================

QFrame* TaskDetailView::buildHeader()
{
    QFrame *header = new QFrame(this);
    header->setObjectName("detailHeader");
    header->setStyleSheet(
        "#detailHeader { background: #FFFFFF; border-bottom: none; }");
    header->installEventFilter(new WindowDragger(this));
    QVBoxLayout *headerLayout = new QVBoxLayout(header);
    headerLayout->setContentsMargins(Spacing::XXL, Spacing::LG, Spacing::XL, Spacing::LG);
    headerLayout->setSpacing(Spacing::SM);

    // ---- 第一行：编号 + 标签 + 关闭按钮 ----
    QHBoxLayout *topRow = new QHBoxLayout();
    topRow->setSpacing(Spacing::SM);

    taskNoLabel_ = new QLabel(header);
    taskNoLabel_->setStyleSheet(
        "QLabel { font-family: MiSans; font-size: 13px; "
        "color: #0F172A; }");

    statusTagLabel_ = new QLabel(header);
    statusTagLabel_->setStyleSheet(
        "QLabel { font-size: 14px; font-weight: 400; padding: 4px 10px; border-radius: 4px; }");

    keyTagLabel_ = new QLabel(QStringLiteral("\u3010\u91cd\u70b9\u3011"), header);  // 【重点】
    keyTagLabel_->setObjectName(QStringLiteral("keyTagLabel"));
    keyTagLabel_->setProperty("appUiNoLabelReset", true);
    keyTagLabel_->setStyleSheet(
        "QLabel { background: transparent; color: #FF0000; font-size: 22px; font-weight: 400; "
        "padding: 0; border-radius: 0; }");
    keyTagLabel_->setFixedSize(keyTagLabel_->sizeHint());
    keyTagLabel_->hide();

    periodicTagLabel_ = createTagLabel(QStringLiteral("\u3010\u5468\u671f\u3011"),
                                       "#FFF7ED", "#F97316");  // 【周期】
    periodicTagLabel_->hide();

    closeBtn_ = new QPushButton(QStringLiteral("\u2715"), header);  // ✕
    closeBtn_->setObjectName("headerCloseBtn");
    closeBtn_->setFixedSize(32, 32);
    closeBtn_->setCursor(Qt::PointingHandCursor);
    closeBtn_->setFocusPolicy(Qt::NoFocus);
    closeBtn_->setStyleSheet(
        "#headerCloseBtn { border: none; background: transparent; "
        "font-size: 16px; color: #94A3B8; border-radius: 6px; }"
        "#headerCloseBtn:hover { color: #0F172A; background: #F5F7FA; }");

    taskNoLabel_->hide();
    statusTagLabel_->hide();
    topRow->addWidget(keyTagLabel_);
    topRow->addWidget(periodicTagLabel_);
    titleLabel_ = new QLabel(header);
    titleLabel_->setObjectName("appUiDetailTitle");
    QFont titleFont(QStringLiteral("MiSans"));
    titleFont.setPixelSize(22);
    titleFont.setWeight(QFont::Normal);
    titleFont.setStyleName(QStringLiteral("Normal"));
    titleLabel_->setFont(titleFont);
    titleLabel_->setStyleSheet("font-family: MiSans; font-size: 22px; font-weight: 400; color: #0F172A; border: none;");
    titleLabel_->setWordWrap(true);
    titleLabel_->setMinimumHeight(QFontMetrics(titleLabel_->font()).lineSpacing() * 2 + 12);
    titleLabel_->setMaximumHeight(QFontMetrics(titleLabel_->font()).lineSpacing() * 2 + 12);
    titleLabel_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    topRow->addWidget(titleLabel_);
    topRow->addStretch();
    topRow->addWidget(closeBtn_);

    headerLayout->addLayout(topRow);

    // ---- 第二行：任务名称 + 操作按钮 ----
    QHBoxLayout *titleRow = new QHBoxLayout();
    titleRow->setSpacing(Spacing::SM);

    // 编辑按钮（蓝色实心按钮，白色字体）
    editBtn_ = new QPushButton(QStringLiteral("编辑"), header);
    editBtn_->setObjectName("headerEditBtn");
    editBtn_->setCursor(Qt::PointingHandCursor);
    editBtn_->setFocusPolicy(Qt::NoFocus);
    editBtn_->setStyleSheet(
        "#headerEditBtn { background: #2463EB; color: #FFFFFF; border: none; "
        "font-size: 14px; font-weight: 400; padding: 6px 16px; border-radius: 6px; }"
        "#headerEditBtn:hover { background: #1D4ED8; }");

    // 删除按钮（红色实心按钮，白色字体）
    deleteBtn_ = new QPushButton(QStringLiteral("删除"), header);
    deleteBtn_->setObjectName("headerDeleteBtn");
    deleteBtn_->setCursor(Qt::PointingHandCursor);
    deleteBtn_->setFocusPolicy(Qt::NoFocus);
    deleteBtn_->setStyleSheet(
        "#headerDeleteBtn { background: #DC2626; color: #FFFFFF; border: none; "
        "font-size: 14px; font-weight: 400; padding: 6px 16px; border-radius: 6px; }"
        "#headerDeleteBtn:hover { background: #B91C1C; }");

    // 完成按钮（三态，updateCompleteButton 中设置样式）
    completeBtn_ = new QPushButton(header);
    completeBtn_->setCursor(Qt::PointingHandCursor);
    completeBtn_->setFocusPolicy(Qt::NoFocus);

    titleRow->addStretch();
    titleRow->addWidget(editBtn_);
    titleRow->addWidget(deleteBtn_);
    titleRow->addWidget(completeBtn_);

    headerLayout->addLayout(titleRow);

    // 信号连接
    connect(closeBtn_, &QPushButton::clicked, this, &QDialog::reject);
    connect(editBtn_, &QPushButton::clicked, this, &TaskDetailView::onEditClicked);
    connect(deleteBtn_, &QPushButton::clicked, this, &TaskDetailView::onDeleteClicked);
    connect(completeBtn_, &QPushButton::clicked, this, &TaskDetailView::onCompleteClicked);

    return header;
}

// ============================================================================
// 基本信息区
// ============================================================================

QFrame* TaskDetailView::buildBasicInfoSection()
{
    QFrame *section = new QFrame(this);
    section->setObjectName("basicInfoSection");
    section->setStyleSheet(
        "#basicInfoSection { background: #FFFFFF; border: none; border-radius: 12px; }");
    QVBoxLayout *layout = new QVBoxLayout(section);
    layout->setContentsMargins(Spacing::XL, Spacing::LG, Spacing::XL, Spacing::LG);
    layout->setSpacing(Spacing::MD);

    // 区块标题
    QLabel *title = new QLabel(QStringLiteral("\u57fa\u672c\u4fe1\u606f"), section);  // 基本信息
    title->setStyleSheet("font-size: 14px; font-weight: 400; color: #0F172A; border: none;");
    layout->addWidget(title);

    // 4 列网格
    QGridLayout *grid = new QGridLayout();
    grid->setSpacing(Spacing::MD);
    for (int i = 0; i < 4; ++i) {
        grid->setColumnStretch(i, 1);
    }

    // 字段定义：(标签, 控件指针引用, 行, 列)
    struct FieldDef { QString label; QLabel **valuePtr; int row; int col; };
    FieldDef fields[] = {
        {QStringLiteral("\u7f16\u53f7"),       &taskNoValue_,        0, 0},  // 编号
        {QStringLiteral("\u5206\u7c7b"),       &categoryValue_,      0, 1},  // 分类
        {QStringLiteral("\u6765\u6e90"),       &sourceValue_,        0, 2},  // 来源
        {QStringLiteral("\u6807\u7b7e"),       &tagValue_,           0, 3},  // 标签
        {QStringLiteral("\u529e\u7406\u4eba"), &handlerValue_,       1, 0},  // 办理人
        {QStringLiteral("\u72b6\u6001"),       &statusValue_,        1, 1},  // 状态
        {QStringLiteral("\u521b\u5efa\u65f6\u95f4"), &createTimeValue_, 1, 2},  // 创建时间
        {QStringLiteral("\u5b8c\u6210\u65f6\u95f4"), &finishTimeValue_, 1, 3},  // 完成时间
        {QStringLiteral("\u622a\u6b62\u65e5\u671f"), &deadlineValue_, 2, 0},  // 截止日期
        {QStringLiteral("\u529e\u7406\u5468\u671f"), &processingDaysValue_, 2, 1},  // 办理周期
    };

    for (const auto &f : fields) {
        QLabel *label = createInfoLabel(f.label);
        *f.valuePtr = createInfoValue(QStringLiteral("-"));
        grid->addWidget(label, f.row * 2, f.col);
        grid->addWidget(*f.valuePtr, f.row * 2 + 1, f.col);
    }

    finishTimeValue_->setObjectName(QStringLiteral("finishTimeValue"));
    deadlineValue_->setObjectName(QStringLiteral("deadlineValue"));
    processingDaysValue_->setObjectName(QStringLiteral("processingDaysValue"));

    auto *openFolderBtn = new QPushButton(QStringLiteral("打开文件夹"), section);
    openFolderBtn->setObjectName(QStringLiteral("openFolderBtn"));
    openFolderBtn->setCursor(Qt::PointingHandCursor);
    openFolderBtn->setFocusPolicy(Qt::NoFocus);
    openFolderBtn->setStyleSheet(
        "#openFolderBtn { background:#2463EB; color:#FFFFFF; border:none; "
        "font-size:13px; font-weight:400; padding:6px 14px; border-radius:6px; }"
        "#openFolderBtn:hover { background:#1D4ED8; }");
    connect(openFolderBtn, &QPushButton::clicked, this, &TaskDetailView::onOpenFolderClicked);
    grid->addWidget(openFolderBtn, 5, 2, 1, 2, Qt::AlignRight | Qt::AlignVCenter);
    layout->addLayout(grid);
    return section;
}

// ============================================================================
// 任务说明区
// ============================================================================

QFrame* TaskDetailView::buildDescriptionSection()
{
    QFrame *section = new QFrame(this);
    section->setObjectName("descSection");
    section->setStyleSheet(
        "#descSection { background: #FFFFFF; border: none; "
        "border-radius: 12px; }");
    QVBoxLayout *layout = new QVBoxLayout(section);
    layout->setContentsMargins(Spacing::XL, Spacing::LG, Spacing::XL, Spacing::LG);
    layout->setSpacing(Spacing::SM);

    // 区块标题（无按钮）
    QLabel *title = new QLabel(QStringLiteral("\u4efb\u52a1\u8bf4\u660e"), section);  // 任务说明
    title->setStyleSheet("font-size: 14px; font-weight: 400; color: #0F172A; border: none;");
    layout->addWidget(title);

    // 只读文本框
    descEdit_ = new QTextEdit(section);
    descEdit_->setReadOnly(true);
    descEdit_->setMinimumHeight(80);
    descEdit_->setMaximumHeight(200);
    descEdit_->setStyleSheet(
        "QTextEdit { background: #F8FAFC; border: 1px solid #F1F5F9; "
        "border-radius: 8px; padding: 10px 12px; font-size: 14px; "
        "color: #0F172A; line-height: 1.6; }");
    layout->addWidget(descEdit_);

    return section;
}

// ============================================================================
// 任务附件区
// ============================================================================

QFrame* TaskDetailView::buildAttachmentSection()
{
    QFrame *section = new QFrame(this);
    section->setObjectName("attachmentSection");
    section->setStyleSheet(
        "#attachmentSection { background: #FFFFFF; border: none; "
        "border-radius: 12px; }");
    QVBoxLayout *layout = new QVBoxLayout(section);
    layout->setContentsMargins(Spacing::XL, Spacing::LG, Spacing::XL, Spacing::LG);
    layout->setSpacing(Spacing::SM);

    // 区块标题
    QHBoxLayout *titleRow = new QHBoxLayout();
    titleRow->setSpacing(Spacing::SM);
    QLabel *title = new QLabel(QStringLiteral("\u4efb\u52a1\u9644\u4ef6"), section);  // 任务附件
    title->setStyleSheet("font-size: 14px; font-weight: 400; color: #0F172A; border: none;");
    titleRow->addWidget(title);
    layout->addLayout(titleRow);

    // 附件名称显示
    attachmentLabel_ = new QLabel(section);
    attachmentLabel_->setWordWrap(true);
    attachmentLabel_->setStyleSheet(
        "QLabel { background: #F8FAFC; border: 1px solid #F1F5F9; border-radius: 8px; "
        "padding: 10px 12px; font-size: 14px; color: #0F172A; }");
    attachmentLabel_->setTextFormat(Qt::PlainText);
    layout->addWidget(attachmentLabel_);

    return section;
}

// ============================================================================
// 备注时间轴区
// ============================================================================

QFrame* TaskDetailView::buildRemarksSection()
{
    QFrame *section = new QFrame(this);
    section->setObjectName("remarksSection");
    section->setStyleSheet(
        "#remarksSection { background: #FFFFFF; border: none; "
        "border-radius: 12px; }");
    QVBoxLayout *layout = new QVBoxLayout(section);
    layout->setContentsMargins(Spacing::XL, Spacing::LG, Spacing::XL, Spacing::LG);
    layout->setSpacing(Spacing::SM);

    // 区块标题
    QLabel *title = new QLabel(QStringLiteral("\u4efb\u52a1\u5907\u6ce8"), section);  // 任务备注
    title->setStyleSheet("font-size: 14px; font-weight: 400; color: #0F172A; border: none;");
    layout->addWidget(title);

    // 备注列表（自定义委托）
    remarksList_ = new QListWidget(section);
    remarksList_->setObjectName("remarksList");
    remarksList_->setMouseTracking(true);
    remarksList_->setSelectionMode(QAbstractItemView::NoSelection);
    remarksList_->setFocusPolicy(Qt::NoFocus);
    remarksList_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    remarksList_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    remarksList_->setFrameShape(QFrame::NoFrame);
    remarksList_->setResizeMode(QListWidget::Adjust);  // 窗口大小变化时重算项高度
    remarksList_->setStyleSheet(
        "#remarksList { background: #FFFFFF; border: none; }"
        "#remarksList::item { border-bottom: 1px solid #F1F5F9; padding: 4px 0px; }");

    auto *remarkDelegate = new TimelineDelegate(true, remarksList_);
    remarkDelegate->setDeleteCallback([this](int remarkId) {
        onRemarkDeleted(remarkId);
    });
    remarksList_->setItemDelegate(remarkDelegate);

    layout->addWidget(remarksList_);

    // 底部输入框 + 添加按钮
    QHBoxLayout *inputRow = new QHBoxLayout();
    inputRow->setSpacing(Spacing::SM);

    remarkInput_ = new QLineEdit(section);
    remarkInput_->setPlaceholderText(QStringLiteral("\u8f93\u5165\u5907\u6ce8\u5185\u5bb9\uff0c\u56de\u8f66\u6dfb\u52a0..."));  // 输入备注内容，回车添加...
    remarkInput_->setStyleSheet(
        "QLineEdit { background: #F8FAFC; border: 1px solid #E2E8F0; "
        "border-radius: 8px; padding: 8px 12px; font-size: 13px; color: #0F172A; }"
        "QLineEdit:focus { border-color: #2463EB; background: #FFFFFF; }");

    addRemarkBtn_ = new QPushButton(QStringLiteral("\u6dfb\u52a0\u5907\u6ce8"), section);  // 添加备注
    addRemarkBtn_->setObjectName("addRemarkBtn");
    addRemarkBtn_->setCursor(Qt::PointingHandCursor);
    addRemarkBtn_->setFocusPolicy(Qt::NoFocus);
    addRemarkBtn_->setStyleSheet(
        "#addRemarkBtn { background: #2463EB; color: #FFFFFF; border: none; "
        "font-size: 13px; font-weight: 500; padding: 8px 16px; border-radius: 8px; }"
        "#addRemarkBtn:hover { background: #1D4ED8; }");

    inputRow->addWidget(remarkInput_, 1);
    inputRow->addWidget(addRemarkBtn_);
    layout->addLayout(inputRow);

    connect(addRemarkBtn_, &QPushButton::clicked, this, &TaskDetailView::onAddRemark);
    connect(remarkInput_, &QLineEdit::returnPressed, this, &TaskDetailView::onAddRemark);

    return section;
}

// ============================================================================
// 操作历史区
// ============================================================================

QFrame* TaskDetailView::buildHistorySection()
{
    QFrame *section = new QFrame(this);
    section->setObjectName("historySection");
    section->setStyleSheet(
        "#historySection { background: #FFFFFF; border: none; "
        "border-radius: 12px; }");
    QVBoxLayout *layout = new QVBoxLayout(section);
    layout->setContentsMargins(Spacing::XL, Spacing::LG, Spacing::XL, Spacing::LG);
    layout->setSpacing(Spacing::SM);

    // 区块标题
    QLabel *title = new QLabel(QStringLiteral("\u64cd\u4f5c\u5386\u53f2"), section);  // 操作历史
    title->setStyleSheet("font-size: 14px; font-weight: 400; color: #0F172A; border: none;");
    layout->addWidget(title);

    // 历史列表（只读，无删除按钮）
    historyList_ = new QListWidget(section);
    historyList_->setObjectName("historyList");
    historyList_->setMouseTracking(true);
    historyList_->setSelectionMode(QAbstractItemView::NoSelection);
    historyList_->setFocusPolicy(Qt::NoFocus);
    historyList_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    historyList_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    historyList_->setFrameShape(QFrame::NoFrame);
    historyList_->setResizeMode(QListWidget::Adjust);  // 窗口大小变化时重算项高度
    historyList_->setStyleSheet(
        "#historyList { background: #FFFFFF; border: none; }"
        "#historyList::item { border-bottom: 1px solid #F1F5F9; padding: 4px 0px; }");

    // 历史用只读委托（无删除按钮）
    historyList_->setItemDelegate(new TimelineDelegate(false, historyList_));

    layout->addWidget(historyList_);

    return section;
}

// ============================================================================
// 成果信息区
// ============================================================================

QFrame* TaskDetailView::buildAchievementSection()
{
    achievementFrame_ = new QFrame(this);
    achievementFrame_->setObjectName("achievementSection");
    achievementFrame_->setStyleSheet(
        "#achievementSection { background: #FFFFFF; border: none; "
        "border-radius: 12px; }");
    QVBoxLayout *layout = new QVBoxLayout(achievementFrame_);
    layout->setContentsMargins(Spacing::XL, Spacing::LG, Spacing::XL, Spacing::LG);
    layout->setSpacing(Spacing::SM);

    // 区块标题
    QLabel *title = new QLabel(QStringLiteral("\u6210\u679c\u4fe1\u606f"), achievementFrame_);  // 成果信息
    title->setStyleSheet("font-size: 14px; font-weight: 400; color: #0F172A; border: none;");
    layout->addWidget(title);

    // 成果内容容器
    QHBoxLayout *contentRow = new QHBoxLayout();
    contentRow->setSpacing(Spacing::SM);

    achievementLabel_ = new QLabel(achievementFrame_);
    achievementLabel_->setWordWrap(true);
    achievementLabel_->setStyleSheet("font-size: 14px; color: #0F172A; border: none;");

    addAchievementBtn_ = new QPushButton(QStringLiteral("\u8865\u5f55\u6210\u679c"), achievementFrame_);  // 补录成果
    addAchievementBtn_->setObjectName("addAchievementBtn");
    addAchievementBtn_->setCursor(Qt::PointingHandCursor);
    addAchievementBtn_->setFocusPolicy(Qt::NoFocus);
    addAchievementBtn_->setStyleSheet(
        "#addAchievementBtn { border: 1px solid #2463EB; background: #FFFFFF; color: #2463EB; "
        "font-size: 12px; font-weight: 500; padding: 4px 10px; border-radius: 6px; }"
        "#addAchievementBtn:hover { background: #EFF6FF; }");
    addAchievementBtn_->hide();

    contentRow->addWidget(achievementLabel_, 1);
    contentRow->addWidget(addAchievementBtn_);
    layout->addLayout(contentRow);

    // T4.4: 补录成果 — 打开成果填写弹窗
    connect(addAchievementBtn_, &QPushButton::clicked, this, [this]() {
        TaskDetailUseCase useCase;
        auto optTask = useCase.findTask(taskId_);
        if (!optTask) return;

        AchievementDialog dialog(taskId_, optTask->category, optTask->title, this);
        if (dialog.exec() == QDialog::Accepted) {
            auto achievement = dialog.getAchievement();
            if (achievement) {
                loadAchievement();  // 刷新成果区，显示新录入的成果
                emit taskUpdated();  // 通知父窗口刷新
            }
        }
    });

    return achievementFrame_;
}

// ============================================================================
// 数据加载
// ============================================================================

void TaskDetailView::loadTaskData()
{
    TaskDetailUseCase useCase;
    auto optTask = useCase.findTask(taskId_);
    if (!optTask.has_value()) {
        StyledMessageBox::warning(this, QStringLiteral("\u63d0\u793a"),
                             QStringLiteral("\u4efb\u52a1\u4e0d\u5b58\u5728\u6216\u5df2\u88ab\u5220\u9664"));  // 任务不存在或已被删除
        reject();
        return;
    }

    const Task &task = optTask.value();

    // ---- 标题栏 ----
    taskNoLabel_->setText(task.taskNo);
    titleLabel_->setText(task.title);

    // 状态标签
    QString statusBg, statusFg;
    if (task.status == QStringLiteral("\u8fdb\u884c\u4e2d")) {  // 进行中
        statusBg = "#FDE68A"; statusFg = "#92400E";
    } else if (task.status == QStringLiteral("\u5df2\u5b8c\u6210")) {  // 已完成
        statusBg = "#BBF7D0"; statusFg = "#14532D";
    } else if (task.status == QStringLiteral("\u4ed6\u4eba\u529e\u7406")) {  // 他人办理
        statusBg = "#E2E8F0"; statusFg = "#475569";
    } else if (task.status == QStringLiteral("\u5df2\u5220\u9664")) {  // 已删除
        statusBg = "#FECACA"; statusFg = "#991B1B";
    } else {
        statusBg = "#E2E8F0"; statusFg = "#475569";
    }

    // 重建状态标签样式
    statusTagLabel_->setText(task.status);
    statusTagLabel_->setStyleSheet(QString(
        "QLabel { background: %1; color: %2; font-size: 13px; font-weight: 400; "
        "padding: 4px 10px; border-radius: 4px; }").arg(statusBg, statusFg));
    statusTagLabel_->setFixedSize(statusTagLabel_->sizeHint());

    // 重点标签
    keyTagLabel_->setVisible(task.isKeyTask);

    // 周期标签
    periodicTagLabel_->setVisible(task.isFromPeriodic);

    // 完成按钮三态
    updateCompleteButton(task.status);

    // ---- 基本信息区 ----
    taskNoValue_->setText(task.taskNo);
    taskNoValue_->setStyleSheet("font-family: MiSans; font-size: 14px; color: #0F172A; border: none;");

    // 分类标签（带颜色）
    QString catBg;
    if (task.category == QStringLiteral("\u50ac")) { catBg = "#C586C0"; }      // 催
    else if (task.category == QStringLiteral("\u67e5")) { catBg = "#DCDCAA"; }  // 查
    else if (task.category == QStringLiteral("\u62a5")) { catBg = "#569CD6"; }  // 报
    else if (task.category == QStringLiteral("\u529e")) { catBg = "#6A9955"; }  // 办
    else { catBg = "#F1F5F9"; }
    categoryValue_->setText(task.category.isEmpty() ? QStringLiteral("-") : task.category);
    categoryValue_->setStyleSheet(QString(
        "QLabel { background: %1; color: %2; font-size: 14px; font-weight: 400; "
        "border-radius: 6px; border: none; }").arg(catBg, QStringLiteral("#000000")));
    categoryValue_->setAlignment(Qt::AlignCenter);
    categoryValue_->setObjectName(QStringLiteral("appUiCategoryTag"));
    categoryValue_->setFixedSize(30, 24);

    sourceValue_->setText(task.source.isEmpty() ? QStringLiteral("-") : task.source);
    tagValue_->setText(task.tag.isEmpty() ? QStringLiteral("-") : task.tag);
    handlerValue_->setText(task.handler.isEmpty() ? QStringLiteral("-") : task.handler);
    statusValue_->setText(task.status);
    statusValue_->setObjectName(QStringLiteral("appUiDetailStatus"));
    statusValue_->setProperty("appUiNoLabelReset", true);
    const QString detailStatusBg = task.status == QStringLiteral("进行中") ? QStringLiteral("#FFF3C4")
        : task.status == QStringLiteral("已完成") ? QStringLiteral("#DDF8E7")
        : task.status == QStringLiteral("已删除") ? QStringLiteral("#FFE7E6")
        : QStringLiteral("#E2E8F0");
    const QString detailStatusFg = task.status == QStringLiteral("进行中") ? QStringLiteral("#92400E")
        : task.status == QStringLiteral("已完成") ? QStringLiteral("#14532D")
        : task.status == QStringLiteral("已删除") ? QStringLiteral("#991B1B")
        : QStringLiteral("#475569");
    statusValue_->setStyleSheet(QStringLiteral(
        "QLabel { background:%1; color:%2; font-size:14px; font-weight:400;"
        " border-radius:6px; border:none; padding:0 8px; }").arg(detailStatusBg, detailStatusFg));
    statusValue_->setAlignment(Qt::AlignCenter);
    statusValue_->setFixedSize(statusValue_->sizeHint());

    createTimeValue_->setText(formatDateTime(task.createTime));
    finishTimeValue_->setText(task.finishTime.isValid()
                                  ? formatDateTime(task.finishTime)
                                  : QStringLiteral("-"));
    deadlineValue_->setText(task.deadline.isValid()
                                ? task.deadline.toString("yyyy-MM-dd")
                                : QStringLiteral("-"));
    const int daysLeft = QDate::currentDate().daysTo(task.deadline);
    const bool colorDeadline = task.status == QStringLiteral("进行中") && task.deadline.isValid();
    const char *deadlineColor = colorDeadline && daysLeft <= 1 ? "#FF0000"
        : colorDeadline && daysLeft <= 3 ? "#FAAD14" : "#0F172A";
    deadlineValue_->setStyleSheet(QStringLiteral("color:%1; font-size:14px;").arg(deadlineColor));
    processingDaysValue_->setText(task.processingDays > 0
                                      ? QStringLiteral("%1 \u5929").arg(task.processingDays)  // X 天
                                      : QStringLiteral("-"));

    // ---- 任务说明 ----
    descEdit_->setPlainText(task.description.isEmpty()
                                ? QStringLiteral("\uff08\u6682\u65e0\u8bf4\u660e\uff09")  // （暂无说明）
                                : task.description);

    // ---- 任务附件 ----
    if (attachmentLabel_) {
        if (task.folderPath.isEmpty()) {
            // 无附件文件夹记录
            attachmentLabel_->setText(QStringLiteral("\uff08\u6682\u65e0\u9644\u4ef6\uff09"));  // （暂无附件）
            attachmentLabel_->setStyleSheet(
                "QLabel { background: #F8FAFC; border: 1px solid #F1F5F9; border-radius: 8px; "
                "padding: 10px 12px; font-size: 14px; color: #0F172A; }");
        } else if (!QDir(task.folderPath).exists()) {
            // 附件文件夹不存在（可能未随软件迁移或工作目录变更未修复）
            attachmentLabel_->setText(QStringLiteral(
                "\u26a0 \u9644\u4ef6\u6587\u4ef6\u5939\u4e0d\u5b58\u5728\uff0c\u53ef\u80fd\u672a\u968f\u8f6f\u4ef6\u8fc1\u79fb\n"
                "\u8def\u5f84\uff1a%1").arg(task.folderPath));  // ⚠ 附件文件夹不存在，可能未随软件迁移\n路径：xxx
            attachmentLabel_->setStyleSheet(
                "QLabel { background: #FEF2F2; border: 1px solid #FECACA; border-radius: 8px; "
                "padding: 10px 12px; font-size: 14px; color: #DC2626; }");
        } else {
            // 附件文件夹存在：列出内容
            QDir dir(task.folderPath);
            QStringList attachmentNames;
            QFileInfoList entries = dir.entryInfoList(
                QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot,
                QDir::DirsFirst);
            for (const QFileInfo &fi : entries) {
                if (fi.isDir()) {
                    attachmentNames.append(QStringLiteral("\u3010\u6587\u4ef6\u5939\u3011%1").arg(fi.fileName()));  // 【文件夹】
                } else {
                    attachmentNames.append(fi.fileName());
                }
            }
            if (attachmentNames.isEmpty()) {
                attachmentLabel_->setText(QStringLiteral("\uff08\u6682\u65e0\u9644\u4ef6\uff09"));  // （暂无附件）
            } else {
                attachmentLabel_->setText(attachmentNames.join(QStringLiteral("\n")));
            }
            attachmentLabel_->setStyleSheet(
                "QLabel { background: #F8FAFC; border: 1px solid #F1F5F9; border-radius: 8px; "
                "padding: 10px 12px; font-size: 14px; color: #0F172A; }");
        }
    }
}

void TaskDetailView::loadRemarks()
{
    remarksList_->clear();

    const QList<TaskRemark> remarks = TaskDetailUseCase().findRemarks(taskId_);

    for (const TaskRemark &remark : remarks) {
        auto *item = new QListWidgetItem(remarksList_);
        item->setData(Qt::DisplayRole, remark.content);
        item->setData(Qt::UserRole, formatDateTime(remark.createTime));
        item->setData(Qt::UserRole + 1, remark.id);
    }
}

void TaskDetailView::loadHistory()
{
    historyList_->clear();

    const QList<OperationLog> logs = TaskDetailUseCase().findHistory(taskId_);

    for (const OperationLog &log : logs) {
        // 组合显示：[action中文] description
        QString actionCN = actionToChinese(log.action);
        QString displayText;
        if (!log.description.isEmpty()) {
            displayText = QStringLiteral("[%1] %2").arg(actionCN, log.description);
        } else {
            displayText = QStringLiteral("[%1]").arg(actionCN);
        }

        auto *item = new QListWidgetItem(historyList_);
        item->setData(Qt::DisplayRole, displayText);
        item->setData(Qt::UserRole, formatDateTime(log.createTime));
    }
}

void TaskDetailView::loadAchievement()
{
    TaskDetailUseCase useCase;
    auto optTask = useCase.findTask(taskId_);
    if (!optTask.has_value()) {
        return;
    }
    const Task &task = optTask.value();

    // 三态：
    // 1. 未完成 → "(尚未完成)" 灰色文字
    // 2. 已完成有成果 → 只读详情（Phase 4 实现，此处占位）
    // 3. 已完成无成果 → "暂无成果 [补录成果]" 按钮
    if (!task.isCompleted()) {
        achievementLabel_->setText(QStringLiteral("\uff08\u5c1a\u672a\u5b8c\u6210\uff09\n\u5b8c\u6210\u540e\u53ef\u5728\u6b64\u5904\u67e5\u770b\u6210\u679c\u8bb0\u5f55"));  // （尚未完成）完成后可在此处查看成果记录
        achievementLabel_->setStyleSheet("font-size: 14px; color: #0F172A; border: none;");
        addAchievementBtn_->hide();
    } else {
        // 已完成：查询是否已有成果记录
        auto optAchievement = useCase.findAchievement(taskId_);
        if (optAchievement.has_value()) {
            // 有成果 → 只读详情
            const Achievement &ach = optAchievement.value();
            QStringList tags;
            if (ach.isAnnual) tags << QStringLiteral("\u7eb3\u5165\u5e74\u5ea6\u6210\u679c");  // 纳入年度成果
            if (ach.isKeyAchievement) tags << QStringLiteral("\u91cd\u70b9\u6210\u679c");  // 重点成果
            QString tagText = tags.isEmpty() ? QStringLiteral("-")
                                             : tags.join(QStringLiteral(" \u00b7 "));  // ·

            QString detail = QStringLiteral(
                "\u5de5\u4f5c\u5185\u5bb9\uff1a%1\n"   // 工作内容：
                "\u5de5\u4f5c\u5bf9\u8c61\uff1a%2\n"   // 工作对象：
                "\u6210\u679c\u6570\u91cf\uff1a%3\n"   // 成果数量：
                "\u6210\u679c\u91d1\u989d\uff1a%4 \u5143\n"  // 成果金额： 元
                "\u6210\u679c\u8bf4\u660e\uff1a%5\n"   // 成果说明：
                "\u6807\u8bb0\uff1a%6")                 // 标记：
                .arg(ach.workContent.isEmpty() ? QStringLiteral("-") : ach.workContent,
                     ach.workObject.isEmpty() ? QStringLiteral("-") : ach.workObject,
                     QString::number(ach.resultCount),
                     AmountUtil::centsToYuan(ach.resultAmount),
                     ach.description.isEmpty() ? QStringLiteral("-") : ach.description,
                     tagText);
            achievementLabel_->setText(detail);
            achievementLabel_->setStyleSheet("font-size: 14px; color: #0F172A; border: none;");
            addAchievementBtn_->hide();
        } else {
            // 无成果 → "暂无成果" + 补录按钮
            achievementLabel_->setText(QStringLiteral("\u6682\u65e0\u6210\u679c"));  // 暂无成果
            achievementLabel_->setStyleSheet("font-size: 14px; color: #0F172A; border: none;");
            addAchievementBtn_->show();
        }
    }
}

void TaskDetailView::updateCompleteButton(const QString &status)
{
    if (status == QStringLiteral("\u4ed6\u4eba\u529e\u7406")) {  // 他人办理
        // 隐藏完成按钮
        completeBtn_->hide();
    } else if (status == QStringLiteral("\u5df2\u5b8c\u6210")) {  // 已完成
        // 红色"取消完成"
        completeBtn_->setText(QStringLiteral("\u53d6\u6d88\u5b8c\u6210"));  // 取消完成
        completeBtn_->setStyleSheet(
            "QPushButton { background: #DC2626; color: #FFFFFF; border: none; "
            "font-size: 14px; font-weight: 500; padding: 6px 14px; border-radius: 8px; }"
            "QPushButton:hover { background: #B91C1C; }");
        completeBtn_->show();
    } else {
        // 进行中：绿色"标记任务完成"
        completeBtn_->setText(QStringLiteral("\u6807\u8bb0\u4efb\u52a1\u5b8c\u6210"));  // 标记任务完成
        completeBtn_->setStyleSheet(
            "QPushButton { background: #16A34A; color: #FFFFFF; border: none; "
            "font-size: 14px; font-weight: 500; padding: 6px 14px; border-radius: 8px; }"
            "QPushButton:hover { background: #15803D; }");
        completeBtn_->show();
    }
}

// ============================================================================
// 槽函数
// ============================================================================

void TaskDetailView::onCompleteClicked()
{
    TaskDetailUseCase useCase;
    auto optTask = useCase.findTask(taskId_);
    if (!optTask.has_value()) {
        StyledMessageBox::warning(this, QStringLiteral("提示"),
                             QStringLiteral("任务不存在"));
        return;
    }

    const Task &task = optTask.value();
    TaskResult result;

    if (task.isCompleted()) {
        // 已完成 → 取消完成
        result = useCase.cancelComplete(taskId_);
        if (result.ok) {
            Logger::instance().info(QStringLiteral("任务已取消完成: %1").arg(task.taskNo));
        }
    } else if (task.isActive()) {
        // 进行中 → 标记完成
        result = useCase.completeTask(taskId_);
        if (result.ok) {
            Logger::instance().info(QStringLiteral("任务已标记完成: %1").arg(task.taskNo));
            // T4.4: 完成后弹出成果填写弹窗（跳过则留待日后补录）
            if (!useCase.findAchievement(taskId_).has_value()) {
                AchievementDialog dialog(taskId_, task.category, task.title, this);
                dialog.exec();  // 保存则入库，跳过则任务仍为已完成
            }
        }
    } else {
        // 其他状态（如他人办理）不显示完成按钮，不应到达此分支
        return;
    }

    if (!result.ok) {
        StyledMessageBox::warning(this, QStringLiteral("操作失败"),
                             result.error.isEmpty() ? QStringLiteral("操作失败，请重试")
                                                   : result.error);
        return;
    }

    // 刷新数据并通知父窗口
    emit taskUpdated();

    // 标记完成（非取消完成）后自动关闭弹窗
    if (task.isActive()) {
        accept();
        return;
    }
    // 取消完成则刷新数据保持弹窗打开
    refreshData();
}

void TaskDetailView::onEditClicked()
{
    hide();  // 隐藏详情弹窗，避免与编辑弹窗堆叠
    emit editRequested(taskId_);
    accept();
}

void TaskDetailView::onDeleteClicked()
{
    // 确认删除（软件UI风格）
    auto ret = StyledMessageBox::question(this,
                                     QStringLiteral("确认删除"),
                                     QStringLiteral("确定要删除此任务吗？删除后可在回收站恢复。"),
                                     QStringLiteral("确认删除"), QStringLiteral("取消"));
    if (ret != StyledMessageBox::Yes) {
        return;
    }

    TaskResult result = TaskDetailUseCase().deleteTask(taskId_);
    if (!result.ok) {
        StyledMessageBox::warning(this, QStringLiteral("删除失败"),
                             result.error.isEmpty() ? QStringLiteral("删除失败，请重试")
                                                   : result.error);
        return;
    }

    Logger::instance().info(QStringLiteral("任务已删除: taskId=%1").arg(taskId_));
    emit taskDeleted(taskId_);
    emit taskUpdated();
    accept();  // 关闭弹窗
}

void TaskDetailView::onAddRemark()
{
    QString content = remarkInput_->text().trimmed();
    if (content.isEmpty()) {
        remarkInput_->setFocus();
        return;
    }

    TaskResult result = TaskDetailUseCase().addRemark(taskId_, content);
    if (!result.ok) {
        StyledMessageBox::warning(this, QStringLiteral("\u6dfb\u52a0\u5931\u8d25"),  // 添加失败
                             result.error.isEmpty() ? QStringLiteral("\u5907\u6ce8\u6dfb\u52a0\u5931\u8d25\uff0c\u8bf7\u91cd\u8bd5")  // 备注添加失败，请重试
                                                   : result.error);
        return;
    }

    remarkInput_->clear();
    loadRemarks();  // 刷新备注列表
    loadHistory();  // 操作历史也会更新
    Logger::instance().info(QStringLiteral("备注已添加: taskId=%1").arg(taskId_));
}

void TaskDetailView::onRemarkDeleted(int remarkId)
{
    if (remarkId <= 0) {
        return;
    }

    TaskResult result = TaskDetailUseCase().deleteRemark(remarkId);
    if (!result.ok) {
        StyledMessageBox::warning(this, QStringLiteral("\u5220\u9664\u5931\u8d25"),  // 删除失败
                             result.error.isEmpty() ? QStringLiteral("\u5907\u6ce8\u5220\u9664\u5931\u8d25\uff0c\u8bf7\u91cd\u8bd5")  // 备注删除失败，请重试
                                                   : result.error);
        return;
    }

    loadRemarks();  // 刷新备注列表
    loadHistory();  // 操作历史也会更新
    Logger::instance().info(QStringLiteral("备注已删除: remarkId=%1").arg(remarkId));
}

void TaskDetailView::onOpenFolderClicked()
{
    TaskDetailUseCase useCase;
    auto optTask = useCase.findTask(taskId_);
    if (!optTask.has_value()) {
        return;
    }
    const Task &task = optTask.value();

    if (task.folderPath.isEmpty() || !QDir(task.folderPath).exists()) {
        StyledMessageBox::information(this, QStringLiteral("\u63d0\u793a"),
            QStringLiteral("\u9644\u4ef6\u6587\u4ef6\u5939\u4e0d\u5b58\u5728\uff0c\u53ef\u80fd\u672a\u968f\u8f6f\u4ef6\u8fc1\u79fb\u3002\n\n\u8def\u5f84\uff1a%1").arg(
                task.folderPath.isEmpty() ? QStringLiteral("\uff08\u7a7a\uff09") : task.folderPath));  // 附件文件夹不存在，可能未随软件迁移。\n\n路径：xxx
        return;
    }

    QDesktopServices::openUrl(QUrl::fromLocalFile(task.folderPath));
    Logger::instance().info(QStringLiteral("TaskDetailView: 打开文件夹 (taskId=%1, path=%2)")
                                .arg(taskId_).arg(task.folderPath));
}

void TaskDetailView::refreshData()
{
    loadTaskData();
    loadRemarks();
    loadHistory();
    loadAchievement();
}

// ============================================================================
// 工具方法
// ============================================================================

QLabel* TaskDetailView::createInfoLabel(const QString &text)
{
    QLabel *label = new QLabel(text);
    label->setStyleSheet("font-size: 12px; color: #0F172A; border: none;");
    return label;
}

QLabel* TaskDetailView::createInfoValue(const QString &text)
{
    QLabel *label = new QLabel(text);
    label->setStyleSheet("font-size: 14px; color: #0F172A; border: none; font-weight: 400;");
    label->setWordWrap(true);
    return label;
}

QLabel* TaskDetailView::createTagLabel(const QString &text, const QString &bgColor, const QString &textColor)
{
    QLabel *label = new QLabel(text);
    label->setStyleSheet(QString(
        "QLabel { background: %1; color: %2; font-size: 12px; font-weight: 500; "
        "padding: 2px 8px; border-radius: 4px; }").arg(bgColor, textColor));
    label->setFixedSize(label->sizeHint());
    return label;
}

QString TaskDetailView::actionToChinese(const QString &action) const
{
    // 操作日志 action 字段 → 中文描述映射
    static const QHash<QString, QString> mapping = {
        {"create",          QStringLiteral("\u65b0\u5efa")},        // 新建
        {"update",          QStringLiteral("\u66f4\u65b0")},        // 更新
        {"complete",        QStringLiteral("\u5b8c\u6210")},        // 完成
        {"cancel_complete", QStringLiteral("\u53d6\u6d88\u5b8c\u6210")},  // 取消完成
        {"delete",          QStringLiteral("\u5220\u9664")},        // 删除
        {"restore",         QStringLiteral("\u6062\u590d")},        // 恢复
        {"permanent_delete",QStringLiteral("\u5f7b\u5e95\u5220\u9664")},  // 彻底删除
        {"add_remark",      QStringLiteral("\u6dfb\u52a0\u5907\u6ce8")},  // 添加备注
        {"update_remark",   QStringLiteral("\u66f4\u65b0\u5907\u6ce8")},  // 更新备注
        {"delete_remark",   QStringLiteral("\u5220\u9664\u5907\u6ce8")},  // 删除备注
        {"add_file",        QStringLiteral("\u6dfb\u52a0\u6587\u4ef6")},  // 添加文件
        {"remove_file",     QStringLiteral("\u5220\u9664\u6587\u4ef6")},  // 删除文件
        {"update_key_task", QStringLiteral("\u66f4\u65b0\u91cd\u70b9\u6807\u8bb0")},  // 更新重点标记
        {"change_handler",  QStringLiteral("\u53d8\u66f4\u529e\u7406\u4eba")},  // 变更办理人
    };

    return mapping.value(action, action);
}

QString TaskDetailView::formatDateTime(const QDateTime &dt) const
{
    if (!dt.isValid()) {
        return QStringLiteral("-");
    }
    return dt.toString("yyyy-MM-dd HH:mm");
}
