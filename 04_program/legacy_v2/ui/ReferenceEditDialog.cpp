/**
 * @file ReferenceEditDialog.cpp
 * @brief 参考资料编辑弹窗实现
 *
 * 实现：setupUI 搭建表单 / loadData 加载现有数据 / onSaveClicked 更新入库
 *
 * 依赖：ui/ReferenceEditDialog.h、ui/Spacing.h、application/reference、models/ReferenceMaterial、utils/Logger
 * 文档：qt_implementation_guide.md 3.7
 */

#include "ReferenceEditDialog.h"

#include <QComboBox>
#include <QCompleter>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QShortcut>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

#include "ColorPalette.h"
#include "Spacing.h"
#include "application/reference/ReferenceCommandUseCase.h"
#include "application/reference/ReferenceQueryUseCase.h"
#include "models/ReferenceMaterial.h"
#include "service/FileService.h"
#include "utils/Logger.h"
#include "widgets/WindowDragger.h"
#include "widgets/RoundedCornerFilter.h"
#include "widgets/StyledMessageBox.h"

// ============================================================================
// 构造
// ============================================================================

ReferenceEditDialog::ReferenceEditDialog(int refId, QWidget *parent)
    : QDialog(parent)
    , refId_(refId)
    , nameEdit_(nullptr)
    , refNoEdit_(nullptr)
    , sourceCombo_(nullptr)
    , tagCombo_(nullptr)
    , descEdit_(nullptr)
{
    setupUI();
    loadSources();
    loadTags();
    loadData();
}

// ============================================================================
// UI 搭建
// ============================================================================

void ReferenceEditDialog::setupUI()
{
    // 窗口属性：无边框、模态、可调整大小
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setModal(true);
    auto *saveShortcut = new QShortcut(QKeySequence::Save, this);
    saveShortcut->setObjectName(QStringLiteral("shortcutSaveReference"));
    connect(saveShortcut, &QShortcut::activated, this, &ReferenceEditDialog::onSaveClicked);
    setMinimumWidth(560);
    setSizeGripEnabled(true);
    installEventFilter(new RoundedCornerFilter(12, this));

    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ---- 自定义标题栏 ----
    QFrame *headerFrame = new QFrame(this);
    headerFrame->setObjectName("dialogHeader");
    headerFrame->setStyleSheet(
        "#dialogHeader { background: #FFFFFF; border-bottom: none; }");
    headerFrame->installEventFilter(new WindowDragger(this));
    QHBoxLayout *headerLayout = new QHBoxLayout(headerFrame);
    headerLayout->setContentsMargins(Spacing::XXL, Spacing::XL, Spacing::XL, Spacing::XL);
    headerLayout->setSpacing(Spacing::SM);

    QLabel *titleLabel = new QLabel(QStringLiteral("编辑资料"), headerFrame);
    QFont titleFont(QStringLiteral("Segoe UI"), 18, QFont::Normal);
    titleLabel->setFont(titleFont);
    titleLabel->setStyleSheet("color: #0F172A; border: none;");

    QPushButton *closeBtn = new QPushButton(QStringLiteral("\u2715"), headerFrame);
    closeBtn->setObjectName("dialogCloseBtn");
    closeBtn->setFixedSize(32, 32);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setFocusPolicy(Qt::NoFocus);
    closeBtn->setStyleSheet(
        "#dialogCloseBtn { border: none; background: transparent; "
        "font-size: 16px; color: #94A3B8; border-radius: 6px; }"
        "#dialogCloseBtn:hover { color: #0F172A; background: #F5F7FA; }");

    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(closeBtn);

    connect(closeBtn, &QPushButton::clicked, this, &ReferenceEditDialog::onCancelClicked);

    mainLayout->addWidget(headerFrame);

    // ---- 表单区域 ----
    QFrame *bodyFrame = new QFrame(this);
    bodyFrame->setObjectName("dialogBody");
    bodyFrame->setStyleSheet(
        "#dialogBody { background: #FFFFFF; }"
        "#dialogBody QLabel { color: #475569; font-size: 13px; }");
    QVBoxLayout *bodyLayout = new QVBoxLayout(bodyFrame);
    bodyLayout->setContentsMargins(Spacing::XXL, Spacing::XL, Spacing::XXL, Spacing::XL);
    bodyLayout->setSpacing(Spacing::MD);

    QGridLayout *formGrid = new QGridLayout();
    formGrid->setSpacing(Spacing::SM);
    formGrid->setColumnStretch(0, 1);
    formGrid->setColumnStretch(1, 1);

    // ---- 第一行：资料名称 + 编号 ----
    QLabel *nameLabel = new QLabel(QStringLiteral("资料名称 *"), bodyFrame);
    nameEdit_ = new QLineEdit(bodyFrame);
    nameEdit_->setObjectName("referenceNameEdit");

    QLabel *refNoLabel = new QLabel(QStringLiteral("编号"), bodyFrame);
    refNoEdit_ = new QLineEdit(bodyFrame);
    refNoEdit_->setObjectName("referenceNoEdit");
    refNoEdit_->setReadOnly(true);
    refNoEdit_->setStyleSheet(
        "QLineEdit { background: #F8FAFC; color: #94A3B8; "
        "border: 1px solid #E2E8F0; border-radius: 8px; padding: 8px 12px; "
        "font-size: 14px; font-family: Segoe UI; }");

    formGrid->addWidget(nameLabel, 0, 0);
    formGrid->addWidget(nameEdit_, 1, 0);
    formGrid->addWidget(refNoLabel, 0, 1);
    formGrid->addWidget(refNoEdit_, 1, 1);

    // ---- 第二行：来源 + 事项标签 ----
    QLabel *sourceLabel = new QLabel(QStringLiteral("来源"), bodyFrame);
    sourceCombo_ = new QComboBox(bodyFrame);
    sourceCombo_->setObjectName("referenceSourceCombo");
    sourceCombo_->setEditable(true);
    sourceCombo_->setInsertPolicy(QComboBox::NoInsert);
    sourceCombo_->lineEdit()->setPlaceholderText(QStringLiteral("输入或选择来源"));
    {
        QCompleter *completer = new QCompleter(sourceCombo_->model(), sourceCombo_);
        completer->setFilterMode(Qt::MatchContains);
        completer->setCaseSensitivity(Qt::CaseInsensitive);
        sourceCombo_->setCompleter(completer);
    }

    QLabel *tagLabel = new QLabel(QStringLiteral("事项标签"), bodyFrame);
    tagCombo_ = new QComboBox(bodyFrame);
    tagCombo_->setObjectName("referenceTagCombo");
    tagCombo_->setEditable(true);
    tagCombo_->setInsertPolicy(QComboBox::NoInsert);
    tagCombo_->lineEdit()->setPlaceholderText(QStringLiteral("输入或选择标签"));
    {
        QCompleter *completer = new QCompleter(tagCombo_->model(), tagCombo_);
        completer->setFilterMode(Qt::MatchContains);
        completer->setCaseSensitivity(Qt::CaseInsensitive);
        tagCombo_->setCompleter(completer);
    }

    formGrid->addWidget(sourceLabel, 2, 0);
    formGrid->addWidget(sourceCombo_, 3, 0);
    formGrid->addWidget(tagLabel, 2, 1);
    formGrid->addWidget(tagCombo_, 3, 1);

    bodyLayout->addLayout(formGrid);

    // ---- 资料说明 ----
    QLabel *descLabel = new QLabel(QStringLiteral("资料说明"), bodyFrame);
    bodyLayout->addWidget(descLabel);

    descEdit_ = new QTextEdit(bodyFrame);
    descEdit_->setObjectName("referenceDescriptionEdit");
    descEdit_->setFixedHeight(96);  // 约 4 行高度
    bodyLayout->addWidget(descEdit_);

    mainLayout->addWidget(bodyFrame);

    // ---- 底部按钮栏 ----
    QFrame *footerFrame = new QFrame(this);
    footerFrame->setObjectName("dialogFooter");
    footerFrame->setStyleSheet(
        "#dialogFooter { background: #FFFFFF; border-top: 1px solid #E2E8F0; }");
    QHBoxLayout *footerLayout = new QHBoxLayout(footerFrame);
    footerLayout->setContentsMargins(Spacing::XXL, Spacing::LG, Spacing::XXL, Spacing::LG);
    footerLayout->addStretch();

    QPushButton *cancelBtn = new QPushButton(QStringLiteral("取消"), footerFrame);
    cancelBtn->setObjectName("btnSecondary");
    cancelBtn->setCursor(Qt::PointingHandCursor);
    cancelBtn->setFocusPolicy(Qt::NoFocus);
    cancelBtn->setMinimumWidth(96);
    cancelBtn->setStyleSheet(
        "#btnSecondary { background: #FFFFFF; color: #475569; "
        "border: 1px solid #E2E8F0; border-radius: 8px; padding: 9px 18px; font-size: 14px; }"
        "#btnSecondary:hover { background: #F5F7FA; border-color: #CBD5E1; }");

    QPushButton *saveBtn = new QPushButton(QStringLiteral("保存修改"), footerFrame);
    saveBtn->setObjectName("btnPrimary");
    saveBtn->setCursor(Qt::PointingHandCursor);
    saveBtn->setFocusPolicy(Qt::NoFocus);
    saveBtn->setMinimumWidth(96);
    saveBtn->setDefault(true);
    saveBtn->setStyleSheet(
        "#btnPrimary { background: #2463EB; color: #FFFFFF; "
        "border: 1px solid #2463EB; border-radius: 8px; padding: 9px 18px; "
        "font-size: 14px; font-weight: 700; }"
        "#btnPrimary:hover { background: #1D4ED8; }");

    footerLayout->addWidget(cancelBtn);
    footerLayout->addWidget(saveBtn);

    connect(cancelBtn, &QPushButton::clicked, this, &ReferenceEditDialog::onCancelClicked);
    connect(saveBtn, &QPushButton::clicked, this, &ReferenceEditDialog::onSaveClicked);

    mainLayout->addWidget(footerFrame);
}

// ============================================================================
// 数据加载
// ============================================================================

void ReferenceEditDialog::loadSources()
{
    sourceCombo_->addItems(ReferenceQueryUseCase().formSources());
}

void ReferenceEditDialog::loadTags()
{
    tagCombo_->addItems(ReferenceQueryUseCase().formTags());
}

void ReferenceEditDialog::ensureDictEntry()
{
    ReferenceCommandUseCase().ensureDictionaryEntries(sourceCombo_->currentText(), tagCombo_->currentText());
}

void ReferenceEditDialog::loadData()
{
    auto optRef = ReferenceQueryUseCase().find(refId_);
    if (!optRef.has_value()) {
        StyledMessageBox::warning(this, QStringLiteral("提示"),
                             QStringLiteral("资料不存在或已被删除"));
        reject();
        return;
    }

    const ReferenceMaterial &ref = optRef.value();

    // 保存原始数据（保存时保持不变）
    originalRefNo_ = ref.refNo;
    originalFolderName_ = ref.folderName;
    originalFolderPath_ = ref.folderPath;
    originalCreateTime_ = ref.createTime;

    // 填充表单
    nameEdit_->setText(ref.name);
    refNoEdit_->setText(ref.refNo);
    sourceCombo_->setCurrentText(ref.source);
    tagCombo_->setCurrentText(ref.tag);
    descEdit_->setPlainText(ref.description);
}

// ============================================================================
// 表单验证
// ============================================================================

bool ReferenceEditDialog::validateForm()
{
    if (nameEdit_->text().trimmed().isEmpty()) {
        StyledMessageBox::warning(this, QStringLiteral("提示"),
                             QStringLiteral("请输入资料名称"));
        nameEdit_->setFocus();
        return false;
    }
    return true;
}

// ============================================================================
// 槽：保存 / 取消
// ============================================================================

void ReferenceEditDialog::onSaveClicked()
{
    if (!validateForm()) {
        return;
    }

    // 将手动录入的新来源/标签写入字典表
    ReferenceCommandUseCase command;
    command.ensureDictionaryEntries(sourceCombo_->currentText(), tagCombo_->currentText());

    ReferenceMaterial ref;
    ref.id = refId_;
    ref.refNo = originalRefNo_;            // 不变
    ref.name = nameEdit_->text().trimmed();
    ref.source = sourceCombo_->currentText().trimmed();
    ref.tag = tagCombo_->currentText().trimmed();
    ref.description = descEdit_->toPlainText();
    ref.folderName = originalFolderName_;
    ref.folderPath = originalFolderPath_;
    ref.createTime = originalCreateTime_;  // 不变

    const QString newFolderName = FileService().buildRefFolderName(ref);
    const QString newFolderPath = originalFolderPath_.isEmpty() ? QString()
        : QDir(QFileInfo(originalFolderPath_).absolutePath()).filePath(newFolderName);
    const bool renameFolder = !originalFolderPath_.isEmpty()
        && QDir::cleanPath(newFolderPath) != QDir::cleanPath(originalFolderPath_);
    if (renameFolder) {
        if (QFileInfo::exists(newFolderPath)
            || !QDir().rename(originalFolderPath_, newFolderPath)) {
            StyledMessageBox::warning(this, QStringLiteral("保存失败"),
                QStringLiteral("资料目录更名失败，请关闭占用文件后重试"));
            return;
        }
        ref.folderName = newFolderName;
        ref.folderPath = newFolderPath;
    }

    if (!command.update(ref)) {
        if (renameFolder) QDir().rename(newFolderPath, originalFolderPath_);
        StyledMessageBox::warning(this, QStringLiteral("保存失败"),
                              QStringLiteral("资料保存失败，请重试"));
        return;
    }

    Logger::instance().info(QStringLiteral("参考资料已更新: refId=%1").arg(refId_));

    emit referenceUpdated(refId_);
    QDialog::accept();
}

void ReferenceEditDialog::onCancelClicked()
{
    reject();
}

// ============================================================================
// 事件重写
// ============================================================================

void ReferenceEditDialog::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        onSaveClicked();
        return;
    }
    QDialog::keyPressEvent(event);
}

void ReferenceEditDialog::accept()
{
    onSaveClicked();
}
