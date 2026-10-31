/**
 * @file ReferenceCreateDialog.cpp
 * @brief 参考资料创建弹窗实现（T4.9）
 *
 * 实现：setupUI 搭建表单 / loadSources/loadTags 加载字典 /
 *       validateForm 表单验证 / onCreateClicked 创建资料入库 + 创建文件夹 + 复制文件
 *
 * 依赖：ui/ReferenceCreateDialog.h、ui/widgets/DragDropArea.h、ui/Spacing.h、
 *       application/reference、models/ReferenceMaterial、
 *       service/FileService、utils/TaskNumberGenerator、utils/Logger
 * 文档：task.md T4.9、qt_implementation_guide.md 3.7
 */

#include "ReferenceCreateDialog.h"

#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QCompleter>
#include <QDateTime>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QShortcut>
#include <QLabel>
#include <QLineEdit>
#include <QMimeData>
#include <QPushButton>
#include <QTextEdit>
#include <QUrl>
#include <QVBoxLayout>

#include "ColorPalette.h"
#include "Spacing.h"
#include "application/reference/ReferenceCommandUseCase.h"
#include "application/reference/ReferenceQueryUseCase.h"
#include "models/ReferenceMaterial.h"
#include "service/FileService.h"
#include "utils/Logger.h"
#include "utils/TaskNumberGenerator.h"
#include "widgets/DragDropArea.h"
#include "widgets/WindowDragger.h"
#include "widgets/RoundedCornerFilter.h"
#include "widgets/StyledMessageBox.h"

// ============================================================================
// 构造
// ============================================================================

ReferenceCreateDialog::ReferenceCreateDialog(QWidget *parent)
    : QDialog(parent)
    , nameEdit_(nullptr)
    , refNoEdit_(nullptr)
    , sourceCombo_(nullptr)
    , tagCombo_(nullptr)
    , descEdit_(nullptr)
    , dropArea_(nullptr)
{
    setupUI();
}

// ============================================================================
// UI 搭建
// ============================================================================

void ReferenceCreateDialog::setupUI()
{
    // 窗口属性：无边框、模态、可调整大小
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setModal(true);
    auto *saveShortcut = new QShortcut(QKeySequence::Save, this);
    saveShortcut->setObjectName(QStringLiteral("shortcutSaveReference"));
    connect(saveShortcut, &QShortcut::activated, this, &ReferenceCreateDialog::onCreateClicked);
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

    QLabel *titleLabel = new QLabel(QStringLiteral("新建资料"), headerFrame);
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

    connect(closeBtn, &QPushButton::clicked, this, &ReferenceCreateDialog::onCancelClicked);

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

    // 前两行用 QGridLayout（2 列）
    QGridLayout *formGrid = new QGridLayout();
    formGrid->setSpacing(Spacing::SM);
    formGrid->setColumnStretch(0, 1);
    formGrid->setColumnStretch(1, 1);

    // ---- 第一行：资料名称 + 编号 ----
    QLabel *nameLabel = new QLabel(QStringLiteral("资料名称 *"), bodyFrame);
    nameEdit_ = new QLineEdit(bodyFrame);
    nameEdit_->setObjectName("referenceNameEdit");
    nameEdit_->setPlaceholderText(QStringLiteral("如：股权转让政策汇编"));

    QLabel *refNoLabel = new QLabel(QStringLiteral("编号"), bodyFrame);
    refNoEdit_ = new QLineEdit(bodyFrame);
    refNoEdit_->setObjectName("referenceNoEdit");
    refNoEdit_->setReadOnly(true);
    refNoEdit_->setPlaceholderText(QStringLiteral("自动生成"));
    refNoEdit_->setStyleSheet(
        "QLineEdit { background: #F8FAFC; color: #94A3B8; "
        "border: 1px solid #E2E8F0; border-radius: 8px; padding: 8px 12px; font-size: 14px; }");

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
    descEdit_->setPlaceholderText(QStringLiteral("录入资料说明..."));
    descEdit_->setFixedHeight(240);
    bodyLayout->addWidget(descEdit_);

    // ---- 资料文件 ----
    QLabel *filesLabel = new QLabel(QStringLiteral("资料文件"), bodyFrame);
    bodyLayout->addWidget(filesLabel);

    dropArea_ = new DragDropArea(bodyFrame);
    dropArea_->setObjectName("referenceAttachmentArea");
    dropArea_->setFixedHeight(120);
    bodyLayout->addWidget(dropArea_);

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

    QPushButton *createBtn = new QPushButton(QStringLiteral("创建"), footerFrame);
    createBtn->setObjectName("btnPrimary");
    createBtn->setCursor(Qt::PointingHandCursor);
    createBtn->setFocusPolicy(Qt::NoFocus);
    createBtn->setMinimumWidth(96);
    createBtn->setDefault(true);
    createBtn->setStyleSheet(
        "#btnPrimary { background: #2463EB; color: #FFFFFF; "
        "border: 1px solid #2463EB; border-radius: 8px; padding: 9px 18px; "
        "font-size: 14px; font-weight: 400; }"
        "#btnPrimary:hover { background: #1D4ED8; }");

    footerLayout->addWidget(cancelBtn);
    footerLayout->addWidget(createBtn);

    connect(cancelBtn, &QPushButton::clicked, this, &ReferenceCreateDialog::onCancelClicked);
    connect(createBtn, &QPushButton::clicked, this, &ReferenceCreateDialog::onCreateClicked);

    mainLayout->addWidget(footerFrame);

    // 加载字典数据
    loadSources();
    loadTags();

    // 来源和标签默认为空（与新建任务弹窗一致）
    sourceCombo_->setCurrentText(QString());
    tagCombo_->setCurrentText(QString());
}

// ============================================================================
// 数据加载
// ============================================================================

void ReferenceCreateDialog::loadSources()
{
    sourceCombo_->addItems(ReferenceQueryUseCase().formSources());
}

void ReferenceCreateDialog::loadTags()
{
    tagCombo_->addItems(ReferenceQueryUseCase().formTags());
}

// ============================================================================
// 将手动录入的新来源/标签写入字典表
// ============================================================================

void ReferenceCreateDialog::ensureDictEntry()
{
    ReferenceCommandUseCase().ensureDictionaryEntries(source(), tag());
}

// ============================================================================
// 表单验证
// ============================================================================

bool ReferenceCreateDialog::validateForm()
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
// 槽：创建 / 取消
// ============================================================================

void ReferenceCreateDialog::onCreateClicked()
{
    if (!validateForm()) {
        return;
    }

    // 将手动录入的新来源/标签写入字典表
    ReferenceCommandUseCase command;
    command.ensureDictionaryEntries(source(), tag());

    // 构造 ReferenceMaterial
    ReferenceMaterial ref;
    ref.refNo = TaskNumberGenerator::nextRefNo();
    ref.name = name();
    ref.source = source();
    ref.tag = tag();
    ref.description = description();
    ref.createTime = QDateTime::currentDateTime();

    // 入库
    int refId = command.create(ref);
    if (refId <= 0) {
        StyledMessageBox::warning(this, QStringLiteral("创建失败"),
                              QStringLiteral("资料创建失败，请重试"));
        return;
    }
    ref.id = refId;

    // 构建文件夹名并创建文件夹
    FileService fileService;
    ref.folderName = fileService.buildRefFolderName(ref);
    QString workDir = fileService.getWorkDir();
    if (!workDir.isEmpty()) {
        ref.folderPath = fileService.buildRefFolderPath(ref, workDir);
        if (!fileService.createRefFolder(ref)) {
            Logger::instance().warn(QStringLiteral("ReferenceCreateDialog: 资料文件夹创建失败: %1")
                                        .arg(ref.folderPath));
        }

        // 复制文件（异步）
        QStringList files = sourceFiles();
        if (!files.isEmpty()) {
            FileService::copyFilesToTaskDirAsync(files, ref.folderPath);
        }
    } else {
        Logger::instance().warn(QStringLiteral("ReferenceCreateDialog: 工作目录未设置，跳过文件夹创建"));
    }

    // 更新 folder_name/folder_path
    if (!command.update(ref)) {
        Logger::instance().warn(QStringLiteral("ReferenceCreateDialog: 更新文件夹路径失败 (refId=%1)")
                                    .arg(refId));
    }

    Logger::instance().info(QStringLiteral("参考资料已创建: %1 (id=%2)").arg(ref.refNo).arg(refId));

    emit referenceCreated(refId);
    QDialog::accept();
}

void ReferenceCreateDialog::onCancelClicked()
{
    reject();
}

// ============================================================================
// 事件重写
// ============================================================================

void ReferenceCreateDialog::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        onCreateClicked();
        return;
    }
    // Ctrl+V 粘贴文件到附件区域（无文本框聚焦时）
    if (event->key() == Qt::Key_V && event->modifiers() == Qt::ControlModifier) {
        QWidget *focusWidget = QApplication::focusWidget();
        bool isTextInput = qobject_cast<QLineEdit *>(focusWidget)
                         || qobject_cast<QTextEdit *>(focusWidget)
                         || (focusWidget && focusWidget->inherits("QAbstractSpinBox"));
        if (!isTextInput && dropArea_) {
            QClipboard *clipboard = QApplication::clipboard();
            const QMimeData *mimeData = clipboard->mimeData();
            if (mimeData && mimeData->hasUrls()) {
                QStringList newFiles;
                for (const QUrl &url : mimeData->urls()) {
                    if (url.isLocalFile()) {
                        newFiles << url.toLocalFile();
                    }
                }
                if (!newFiles.isEmpty() && dropArea_->pasteFiles(newFiles)) {
                    return;
                }
            }
        }
    }
    QDialog::keyPressEvent(event);
}

void ReferenceCreateDialog::accept()
{
    onCreateClicked();
}

// ============================================================================
// 表单数据获取
// ============================================================================

QString ReferenceCreateDialog::name() const
{
    return nameEdit_->text().trimmed();
}

QString ReferenceCreateDialog::source() const
{
    return sourceCombo_->currentText().trimmed();
}

QString ReferenceCreateDialog::tag() const
{
    return tagCombo_->currentText().trimmed();
}

QString ReferenceCreateDialog::description() const
{
    return descEdit_->toPlainText();
}

QStringList ReferenceCreateDialog::sourceFiles() const
{
    return dropArea_->filePaths();
}
