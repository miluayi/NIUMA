/**
 * @file ReferenceDetailDialog.cpp
 * @brief 参考资料详情弹窗实现（T4.10）
 *
 * 实现：setupUI 搭建弹窗布局 / loadData 加载资料数据 /
 *       onOpenFolderClicked 打开文件夹 / onEditClicked 编辑 / onDeleteClicked 删除
 *
 * 依赖：ui/ReferenceDetailDialog.h、ui/Spacing.h、application/reference、models/ReferenceMaterial、service/FileService、
 *       utils/FileUtil、utils/Logger
 * 文档：task.md T4.10、qt_implementation_guide.md 3.4
 */

#include "ReferenceDetailDialog.h"

#include <QDesktopServices>
#include <QFont>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QUrl>
#include <QVBoxLayout>

#include "ColorPalette.h"
#include "Spacing.h"
#include "application/reference/ReferenceQueryUseCase.h"
#include "application/reference/ReferenceCommandUseCase.h"
#include "models/ReferenceMaterial.h"
#include "utils/FileUtil.h"
#include "utils/Logger.h"
#include "widgets/WindowDragger.h"
#include "widgets/RoundedCornerFilter.h"
#include "widgets/StyledMessageBox.h"

// ============================================================================
// 构造
// ============================================================================

ReferenceDetailDialog::ReferenceDetailDialog(int refId, QWidget *parent)
    : QDialog(parent)
    , refId_(refId)
    , titleLabel_(nullptr)
    , closeBtn_(nullptr)
    , refNoValue_(nullptr)
    , sourceValue_(nullptr)
    , tagValue_(nullptr)
    , createTimeValue_(nullptr)
    , descEdit_(nullptr)
    , openFolderBtn_(nullptr)
{
    setupUI();
    loadData();
}

// ============================================================================
// UI 搭建
// ============================================================================

void ReferenceDetailDialog::setupUI()
{
    // 窗口属性：无边框、模态、可调整大小
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setModal(true);
    setMinimumWidth(600);
    setSizeGripEnabled(true);
    installEventFilter(new RoundedCornerFilter(12, this));

    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    mainLayout->addWidget(buildHeader());
    mainLayout->addWidget(buildBasicInfoSection());
    mainLayout->addWidget(buildDescriptionSection());
    mainLayout->addStretch();
    mainLayout->addWidget(buildFooter());
}

QFrame* ReferenceDetailDialog::buildHeader()
{
    QFrame *header = new QFrame(this);
    header->setObjectName("detailHeader");
    header->setStyleSheet(
        "#detailHeader { background: #FFFFFF; border-bottom: none; }");
    header->installEventFilter(new WindowDragger(this));
    QHBoxLayout *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(Spacing::XXL, Spacing::XL, Spacing::XL, Spacing::XL);
    headerLayout->setSpacing(Spacing::SM);

    titleLabel_ = new QLabel(header);
    titleLabel_->setObjectName("appUiReferenceDetailTitle");
    QFont titleFont(QStringLiteral("Segoe UI"), 18, QFont::Normal);
    titleLabel_->setFont(titleFont);
    titleLabel_->setStyleSheet("color: #0F172A; border: none;");
    titleLabel_->setWordWrap(false);

    closeBtn_ = new QPushButton(QStringLiteral("\u2715"), header);
    closeBtn_->setObjectName("headerCloseBtn");
    closeBtn_->setFixedSize(32, 32);
    closeBtn_->setCursor(Qt::PointingHandCursor);
    closeBtn_->setFocusPolicy(Qt::NoFocus);
    closeBtn_->setStyleSheet(
        "#headerCloseBtn { border: none; background: transparent; "
        "font-size: 18px; color: #94A3B8; border-radius: 6px; }"
        "#headerCloseBtn:hover { color: #0F172A; background: #F5F7FA; }");

    headerLayout->addWidget(titleLabel_);
    headerLayout->addStretch();
    headerLayout->addWidget(closeBtn_);

    connect(closeBtn_, &QPushButton::clicked, this, &ReferenceDetailDialog::onCloseClicked);

    return header;
}

QFrame* ReferenceDetailDialog::buildBasicInfoSection()
{
    QFrame *section = new QFrame(this);
    section->setObjectName("basicInfoSection");
    section->setStyleSheet(
        "#basicInfoSection { background: #FFFFFF; border: 1px solid #DCE3EF; border-radius: 8px; }");
    QVBoxLayout *layout = new QVBoxLayout(section);
    layout->setContentsMargins(Spacing::XXL, Spacing::LG, Spacing::XXL, Spacing::LG);
    layout->setSpacing(Spacing::MD);

    QLabel *title = new QLabel(QStringLiteral("基本信息"), section);
    title->setStyleSheet("font-size: 14px; font-weight: 700; color: #94A3B8; border: none;");
    layout->addWidget(title);

    // 2×2 网格
    QGridLayout *grid = new QGridLayout();
    grid->setSpacing(Spacing::MD);
    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(1, 1);

    // 编号
    grid->addWidget(createInfoLabel(QStringLiteral("编号")), 0, 0);
    refNoValue_ = createInfoValue(QStringLiteral("-"));
    refNoValue_->setStyleSheet("font-family: Segoe UI; font-size: 14px; color: #0F172A; border: none;");
    grid->addWidget(refNoValue_, 1, 0);

    // 来源
    grid->addWidget(createInfoLabel(QStringLiteral("来源")), 0, 1);
    sourceValue_ = createInfoValue(QStringLiteral("-"));
    grid->addWidget(sourceValue_, 1, 1);

    // 事项标签
    grid->addWidget(createInfoLabel(QStringLiteral("事项标签")), 2, 0);
    tagValue_ = createInfoValue(QStringLiteral("-"));
    grid->addWidget(tagValue_, 3, 0);

    // 创建时间
    grid->addWidget(createInfoLabel(QStringLiteral("创建时间")), 2, 1);
    createTimeValue_ = createInfoValue(QStringLiteral("-"));
    grid->addWidget(createTimeValue_, 3, 1);

    layout->addLayout(grid);
    return section;
}

QFrame* ReferenceDetailDialog::buildDescriptionSection()
{
    QFrame *section = new QFrame(this);
    section->setObjectName("descSection");
    section->setStyleSheet(
        "#descSection { background: #FFFFFF; border: none; }");
    QVBoxLayout *layout = new QVBoxLayout(section);
    layout->setContentsMargins(Spacing::XXL, Spacing::LG, Spacing::XXL, Spacing::XL);
    layout->setSpacing(Spacing::SM);

    // 标题行 + 打开文件夹按钮
    QHBoxLayout *titleRow = new QHBoxLayout();
    QLabel *title = new QLabel(QStringLiteral("资料说明"), section);
    title->setStyleSheet("font-size: 14px; font-weight: 700; color: #94A3B8; border: none;");

    openFolderBtn_ = new QPushButton(QStringLiteral("打开文件夹"), section);
    openFolderBtn_->setObjectName("openFolderBtn");
    openFolderBtn_->setCursor(Qt::PointingHandCursor);
    openFolderBtn_->setFocusPolicy(Qt::NoFocus);
    openFolderBtn_->setStyleSheet(
        "#openFolderBtn { background: #2463EB; color: #FFFFFF; border: none; "
        "font-size: 14px; font-weight: 700; padding: 8px 18px; border-radius: 6px; }"
        "#openFolderBtn:hover { background: #1D4ED8; }");

    titleRow->addWidget(title);
    titleRow->addStretch();
    titleRow->addWidget(openFolderBtn_);
    layout->addLayout(titleRow);

    // 只读说明卡片
    descEdit_ = new QTextEdit(section);
    descEdit_->setReadOnly(true);
    descEdit_->setMinimumHeight(80);
    descEdit_->setMaximumHeight(200);
    descEdit_->setStyleSheet(
        "QTextEdit { background: #F8FAFC; border: 1px solid #F1F5F9; "
        "border-radius: 8px; padding: 14px 16px; font-size: 15px; "
        "color: #475569; line-height: 1.7; }");
    layout->addWidget(descEdit_);

    connect(openFolderBtn_, &QPushButton::clicked, this, &ReferenceDetailDialog::onOpenFolderClicked);

    return section;
}

QFrame* ReferenceDetailDialog::buildFooter()
{
    QFrame *footer = new QFrame(this);
    footer->setObjectName("dialogFooter");
    footer->setStyleSheet(
        "#dialogFooter { background: #FFFFFF; border-top: 1px solid #E2E8F0; }");
    QHBoxLayout *footerLayout = new QHBoxLayout(footer);
    footerLayout->setContentsMargins(Spacing::XXL, Spacing::LG, Spacing::XXL, Spacing::LG);
    footerLayout->setSpacing(Spacing::SM);

    // 左侧：编辑 + 删除
    QPushButton *editBtn = new QPushButton(QStringLiteral("编辑"), footer);
    editBtn->setObjectName("headerEditBtn");
    editBtn->setCursor(Qt::PointingHandCursor);
    editBtn->setFocusPolicy(Qt::NoFocus);
    editBtn->setStyleSheet(
        "#headerEditBtn { background: #2463EB; color: #FFFFFF; border: none; "
        "font-size: 14px; font-weight: 700; padding: 6px 16px; border-radius: 6px; }"
        "#headerEditBtn:hover { background: #1D4ED8; }");

    QPushButton *deleteBtn = new QPushButton(QStringLiteral("删除"), footer);
    deleteBtn->setObjectName("headerDeleteBtn");
    deleteBtn->setCursor(Qt::PointingHandCursor);
    deleteBtn->setFocusPolicy(Qt::NoFocus);
    deleteBtn->setStyleSheet(
        "#headerDeleteBtn { background: #DC2626; color: #FFFFFF; border: none; "
        "font-size: 14px; font-weight: 700; padding: 6px 16px; border-radius: 6px; }"
        "#headerDeleteBtn:hover { background: #B91C1C; }");

    footerLayout->addWidget(editBtn);
    footerLayout->addWidget(deleteBtn);
    footerLayout->addStretch();

    // 右侧：关闭
    QPushButton *closeFooterBtn = new QPushButton(QStringLiteral("关闭"), footer);
    closeFooterBtn->setObjectName("btnSecondary");
    closeFooterBtn->setCursor(Qt::PointingHandCursor);
    closeFooterBtn->setFocusPolicy(Qt::NoFocus);
    closeFooterBtn->setMinimumWidth(96);
    closeFooterBtn->setStyleSheet(
        "#btnSecondary { background: #FFFFFF; color: #475569; "
        "border: 1px solid #E2E8F0; border-radius: 8px; padding: 9px 18px; font-size: 14px; }"
        "#btnSecondary:hover { background: #F5F7FA; border-color: #CBD5E1; }");
    footerLayout->addWidget(closeFooterBtn);

    connect(editBtn, &QPushButton::clicked, this, &ReferenceDetailDialog::onEditClicked);
    connect(deleteBtn, &QPushButton::clicked, this, &ReferenceDetailDialog::onDeleteClicked);
    connect(closeFooterBtn, &QPushButton::clicked, this, &ReferenceDetailDialog::onCloseClicked);

    return footer;
}

// ============================================================================
// 数据加载
// ============================================================================

void ReferenceDetailDialog::loadData()
{
    auto optRef = ReferenceQueryUseCase().find(refId_);
    if (!optRef.has_value()) {
        StyledMessageBox::warning(this, QStringLiteral("提示"),
                             QStringLiteral("资料不存在或已被删除"));
        reject();
        return;
    }

    const ReferenceMaterial &ref = optRef.value();
    refName_ = ref.name;
    folderPath_ = ref.folderPath;

    titleLabel_->setText(ref.name);
    refNoValue_->setText(ref.refNo.isEmpty() ? QStringLiteral("-") : ref.refNo);
    sourceValue_->setText(ref.source.isEmpty() ? QStringLiteral("-") : ref.source);
    tagValue_->setText(ref.tag.isEmpty() ? QStringLiteral("-") : ref.tag);
    createTimeValue_->setText(ref.createTime.isValid()
                                  ? ref.createTime.toString("yyyy-MM-dd")
                                  : QStringLiteral("-"));

    descEdit_->setPlainText(ref.description.isEmpty()
                                ? QStringLiteral("（暂无说明）")
                                : ref.description);
}

// ============================================================================
// 槽函数
// ============================================================================

void ReferenceDetailDialog::onOpenFolderClicked()
{
    if (folderPath_.isEmpty()) {
        StyledMessageBox::information(this, QStringLiteral("提示"),
                                 QStringLiteral("该资料未关联文件夹"));
        return;
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(folderPath_));
}

void ReferenceDetailDialog::onEditClicked()
{
    hide();  // 隐藏详情弹窗，避免与编辑弹窗堆叠
    emit editRequested(refId_);
    accept();
}

void ReferenceDetailDialog::onDeleteClicked()
{
    auto ret = StyledMessageBox::question(this,
                                     QStringLiteral("确认删除"),
                                     QStringLiteral("确定要删除此参考资料吗？删除后无法恢复。"),
                                     QStringLiteral("确认"),
                                     QStringLiteral("取消"));
    if (ret != StyledMessageBox::Yes) {
        return;
    }

    if (!ReferenceCommandUseCase().remove(refId_)) {
        StyledMessageBox::warning(this, QStringLiteral("删除失败"),
                             QStringLiteral("资料删除失败，请重试"));
        return;
    }

    // 删除文件夹
    if (!folderPath_.isEmpty()) {
        if (!FileUtil::removeDir(folderPath_)) {
            Logger::instance().warn(QStringLiteral("ReferenceDetailDialog: 文件夹删除失败: %1")
                                        .arg(folderPath_));
        }
    }

    Logger::instance().info(QStringLiteral("参考资料已删除: refId=%1").arg(refId_));
    emit referenceDeleted(refId_);
    accept();
}

void ReferenceDetailDialog::onCloseClicked()
{
    reject();
}

// ============================================================================
// 工具方法
// ============================================================================

QLabel* ReferenceDetailDialog::createInfoLabel(const QString &text)
{
    QLabel *label = new QLabel(text);
    label->setStyleSheet("font-size: 12px; color: #94A3B8; border: none;");
    return label;
}

QLabel* ReferenceDetailDialog::createInfoValue(const QString &text)
{
    QLabel *label = new QLabel(text);
    label->setStyleSheet("font-size: 14px; color: #0F172A; border: none; font-weight: 400;");
    label->setWordWrap(true);
    return label;
}
