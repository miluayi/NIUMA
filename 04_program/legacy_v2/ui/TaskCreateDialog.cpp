/**
 * @file TaskCreateDialog.cpp
 * @brief 任务创建弹窗实现（T1.8）
 *
 * 实现：setupUI 搭建表单 / loadSources/loadTags 加载字典 /
 *       validateForm 表单验证 / onCreateClicked 创建任务入库
 *
 * 依赖：ui/TaskCreateDialog.h、ui/widgets/DragDropArea.h、ui/ColorPalette.h、ui/Spacing.h、
 *       application/task、utils/Logger
 * 文档：PRD 5.3、qt_implementation_guide.md 3.7、coding.md 第十一节、task.md T1.8
 */

#include "TaskCreateDialog.h"

#include <QApplication>
#include <QButtonGroup>
#include <QCalendarWidget>
#include <QClipboard>
#include <QComboBox>
#include <QCompleter>
#include <QDateEdit>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QShortcut>
#include <QLabel>
#include <QLineEdit>
#include <QMimeData>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QTextEdit>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

#include "ColorPalette.h"
#include "Spacing.h"
#include "application/task/TaskFormUseCase.h"
#include "application/task/TaskCommandUseCase.h"
#include "application/task/TaskQueryUseCase.h"
#include "models/Task.h"
#include "utils/Logger.h"
#include "widgets/DragDropArea.h"
#include "widgets/WindowDragger.h"
#include "widgets/StyledMessageBox.h"
#include "widgets/RoundedCornerFilter.h"

// ============================================================================
// 构造
// ============================================================================

TaskCreateDialog::TaskCreateDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUI();
    loadFormOptions();
}

// ============================================================================
// UI 搭建
// ============================================================================

void TaskCreateDialog::setupUI()
{
    // 窗口属性：无边框、模态、可调整大小
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setModal(true);
    auto *saveShortcut = new QShortcut(QKeySequence::Save, this);
    saveShortcut->setObjectName(QStringLiteral("shortcutSaveTask"));
    connect(saveShortcut, &QShortcut::activated, this, &TaskCreateDialog::onCreateClicked);
    setMinimumWidth(720);
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

    QLabel *titleLabel = new QLabel(QStringLiteral("新建任务"), headerFrame);
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

    connect(closeBtn, &QPushButton::clicked, this, &TaskCreateDialog::onCancelClicked);

    mainLayout->addWidget(headerFrame);

    // ---- 表单区域 ----
    QFrame *bodyFrame = new QFrame(this);
    bodyFrame->setObjectName("dialogBody");
    bodyFrame->setStyleSheet(
        "#dialogBody { background: #FFFFFF; }"
        "#dialogBody QLabel { color: #0F172A; font-size: 14px; font-weight: 400; }");
    QVBoxLayout *bodyLayout = new QVBoxLayout(bodyFrame);
    bodyLayout->setContentsMargins(Spacing::XXL, Spacing::XL, Spacing::XXL, Spacing::XL);
    bodyLayout->setSpacing(Spacing::MD);

    // 前三行用 QGridLayout（2 列）
    QGridLayout *formGrid = new QGridLayout();
    formGrid->setSpacing(Spacing::SM);
    formGrid->setColumnStretch(0, 1);
    formGrid->setColumnStretch(1, 1);

    // ---- 第一行：任务名称 + 分类 ----
    QLabel *titleLabelField = new QLabel(QStringLiteral("任务名称 <span style='color:#DC2626;'>*</span>"), bodyFrame);
    titleLabelField->setTextFormat(Qt::RichText);
    titleEdit_ = new QLineEdit(bodyFrame);
    titleEdit_->setObjectName("taskTitleEdit");
    titleEdit_->setPlaceholderText(QStringLiteral("如：股权转让专项核查"));

    QLabel *categoryLabel = new QLabel(QStringLiteral("分类 <span style='color:#DC2626;'>*</span>"), bodyFrame);
    categoryLabel->setTextFormat(Qt::RichText);
    categoryCombo_ = new QComboBox(bodyFrame);
    categoryCombo_->addItem(QStringLiteral("催"));
    categoryCombo_->addItem(QStringLiteral("查"));
    categoryCombo_->addItem(QStringLiteral("报"));
    categoryCombo_->addItem(QStringLiteral("办"));
    categoryCombo_->setCurrentText(QStringLiteral("查"));  // 默认"查"

    formGrid->addWidget(titleLabelField, 0, 0);
    formGrid->addWidget(titleEdit_, 1, 0);
    formGrid->addWidget(categoryLabel, 0, 1);
    formGrid->addWidget(categoryCombo_, 1, 1);

    // ---- 第二行：来源 + 事项标签 ----
    QLabel *sourceLabel = new QLabel(QStringLiteral("来源"), bodyFrame);
    sourceCombo_ = new QComboBox(bodyFrame);
    sourceCombo_->setObjectName(QStringLiteral("taskSourceCombo"));
    sourceCombo_->setEditable(true);
    sourceCombo_->setInsertPolicy(QComboBox::NoInsert);
    sourceCombo_->lineEdit()->setPlaceholderText(QStringLiteral("输入或选择来源"));

    QLabel *tagLabel = new QLabel(QStringLiteral("事项标签"), bodyFrame);
    tagCombo_ = new QComboBox(bodyFrame);
    tagCombo_->setObjectName(QStringLiteral("taskTagCombo"));
    tagCombo_->setEditable(true);
    tagCombo_->setInsertPolicy(QComboBox::NoInsert);
    tagCombo_->lineEdit()->setPlaceholderText(QStringLiteral("输入或选择标签"));

    formGrid->addWidget(sourceLabel, 2, 0);
    formGrid->addWidget(sourceCombo_, 3, 0);
    formGrid->addWidget(tagLabel, 2, 1);
    formGrid->addWidget(tagCombo_, 3, 1);

    // ---- 第三行：办理人 + 截止日期 ----
    QLabel *handlerLabel = new QLabel(QStringLiteral("办理人"), bodyFrame);
    handlerCombo_ = new QComboBox(bodyFrame);
    handlerCombo_->setObjectName(QStringLiteral("taskHandlerCombo"));
    handlerCombo_->setEditable(true);
    handlerCombo_->setInsertPolicy(QComboBox::NoInsert);
    handlerCombo_->lineEdit()->setPlaceholderText(QStringLiteral("输入或选择办理人"));
    {
        QCompleter *completer = new QCompleter(handlerCombo_->model(), handlerCombo_);
        completer->setFilterMode(Qt::MatchContains);
        completer->setCaseSensitivity(Qt::CaseInsensitive);
        handlerCombo_->setCompleter(completer);
    }
    handlerCombo_->setCurrentText(QStringLiteral("本人"));  // 默认"本人"

    QLabel *deadlineLabel = new QLabel(QStringLiteral("截止日期"), bodyFrame);
    deadlineEdit_ = new QDateEdit(bodyFrame);
    deadlineEdit_->setCalendarPopup(true);
    deadlineEdit_->setDisplayFormat("yyyy-MM-dd");
    deadlineEdit_->setSpecialValueText(QStringLiteral(" "));  // 空格实现空白显示（Qt 判断 size>0 才生效）
    deadlineEdit_->setMinimumDate(QDate(2000, 1, 1));
    deadlineEdit_->setMaximumDate(QDate(2099, 12, 31));
    // 默认无截止日期（设为 minimumDate 触发 specialValueText 空显示）
    deadlineEdit_->setDate(deadlineEdit_->minimumDate());
    // 点击下拉箭头时，日历弹窗默认选中当天（而非 minimumDate 2000-01-01）
    deadlineEdit_->installEventFilter(this);

    // 清除截止日期按钮（×）：点击后重置为未指定状态
    QPushButton *deadlineClearBtn = new QPushButton(QStringLiteral("\u2715"), bodyFrame);
    deadlineClearBtn->setObjectName("deadlineClearBtn");
    deadlineClearBtn->setFixedSize(28, 28);
    deadlineClearBtn->setCursor(Qt::PointingHandCursor);
    deadlineClearBtn->setFocusPolicy(Qt::NoFocus);
    deadlineClearBtn->setToolTip(QStringLiteral("清除截止日期"));
    deadlineClearBtn->setStyleSheet(
        "QPushButton#deadlineClearBtn {"
        "  border:1px solid #E2E8F0;"
        "  border-radius:6px;"
        "  background:#FFFFFF;"
        "  color:#DC2626;"
        "  font-size:12px;"
        "}"
        "QPushButton#deadlineClearBtn:hover {"
        "  background:#FEF2F2;"
        "  color:#DC2626;"
        "  border-color:#FCA5A5;"
        "}");
    connect(deadlineClearBtn, &QPushButton::clicked, this, [this]() {
        deadlineEdit_->setDate(deadlineEdit_->minimumDate());
    });

    // 将 QDateEdit 和清除按钮放在同一行
    QHBoxLayout *deadlineLayout = new QHBoxLayout();
    deadlineLayout->setSpacing(6);
    deadlineLayout->setContentsMargins(0, 0, 0, 0);
    deadlineLayout->addWidget(deadlineEdit_, 1);
    deadlineLayout->addWidget(deadlineClearBtn);

    formGrid->addWidget(handlerLabel, 4, 0);
    formGrid->addWidget(handlerCombo_, 5, 0);
    formGrid->addWidget(deadlineLabel, 4, 1);
    formGrid->addLayout(deadlineLayout, 5, 1);

    bodyLayout->addLayout(formGrid);

    // ---- 第四行：任务标记 + 目录结构（并排）----
    QHBoxLayout *radioRowLayout = new QHBoxLayout();
    radioRowLayout->setSpacing(Spacing::XXXL);

    // 任务标记组（用 QButtonGroup 分组，避免与其他同父 radio 互斥）
    QVBoxLayout *taskMarkLayout = new QVBoxLayout();
    taskMarkLayout->setSpacing(Spacing::XS);
    QLabel *taskMarkLabel = new QLabel(QStringLiteral("任务标记"), bodyFrame);
    QHBoxLayout *taskMarkRadios = new QHBoxLayout();
    taskMarkRadios->setSpacing(Spacing::MD);
    normalTaskRadio_ = new QRadioButton(QStringLiteral("常规任务"), bodyFrame);
    normalTaskRadio_->setAutoExclusive(false);
    normalTaskRadio_->setChecked(true);
    keyTaskRadio_ = new QRadioButton(QStringLiteral("重点任务"), bodyFrame);
    keyTaskRadio_->setAutoExclusive(false);
    QButtonGroup *taskMarkGroup = new QButtonGroup(this);
    taskMarkGroup->addButton(normalTaskRadio_);
    taskMarkGroup->addButton(keyTaskRadio_);
    taskMarkRadios->addWidget(normalTaskRadio_);
    taskMarkRadios->addWidget(keyTaskRadio_);
    taskMarkRadios->addStretch();
    taskMarkLayout->addWidget(taskMarkLabel);
    taskMarkLayout->addLayout(taskMarkRadios);

    // 目录结构组
    QVBoxLayout *dirStructLayout = new QVBoxLayout();
    dirStructLayout->setSpacing(Spacing::XS);
    QLabel *dirStructLabel = new QLabel(QStringLiteral("目录结构"), bodyFrame);
    QHBoxLayout *dirStructRadios = new QHBoxLayout();
    dirStructRadios->setSpacing(Spacing::MD);
    flatDirRadio_ = new QRadioButton(QStringLiteral("扁平目录"), bodyFrame);
    flatDirRadio_->setAutoExclusive(false);
    flatDirRadio_->setChecked(true);
    subdirRadio_ = new QRadioButton(QStringLiteral("创建子目录"), bodyFrame);
    subdirRadio_->setAutoExclusive(false);
    QButtonGroup *dirStructGroup = new QButtonGroup(this);
    dirStructGroup->addButton(flatDirRadio_);
    dirStructGroup->addButton(subdirRadio_);
    dirStructRadios->addWidget(flatDirRadio_);
    dirStructRadios->addWidget(subdirRadio_);
    dirStructRadios->addStretch();
    dirStructLayout->addWidget(dirStructLabel);
    dirStructLayout->addLayout(dirStructRadios);

    radioRowLayout->addLayout(taskMarkLayout);
    radioRowLayout->addLayout(dirStructLayout);
    radioRowLayout->addStretch();

    bodyLayout->addLayout(radioRowLayout);
    bodyLayout->addSpacing(Spacing::SM);

    // ---- 任务要求 ----
    QLabel *descLabel = new QLabel(QStringLiteral("任务要求"), bodyFrame);
    bodyLayout->addWidget(descLabel);

    descEdit_ = new QTextEdit(bodyFrame);
    descEdit_->setObjectName("taskDescriptionEdit");
    descEdit_->setPlaceholderText(QStringLiteral("粘贴通知原文或录入任务要求..."));
    descEdit_->setFixedHeight(256);

    bodyLayout->addWidget(descEdit_);

    // ---- 任务资料 ----
    QLabel *filesLabel = new QLabel(QStringLiteral("任务资料"), bodyFrame);
    bodyLayout->addWidget(filesLabel);

    dropArea_ = new DragDropArea(bodyFrame);
    dropArea_->setObjectName("taskAttachmentArea");
    dropArea_->setFixedHeight(128);
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

    QPushButton *createBtn = new QPushButton(QStringLiteral("创建任务"), footerFrame);
    createBtn->setObjectName("btnPrimary");
    createBtn->setCursor(Qt::PointingHandCursor);
    createBtn->setFocusPolicy(Qt::NoFocus);
    createBtn->setMinimumWidth(96);
    createBtn->setDefault(true);

    footerLayout->addWidget(cancelBtn);
    footerLayout->addWidget(createBtn);

    connect(cancelBtn, &QPushButton::clicked, this, &TaskCreateDialog::onCancelClicked);
    connect(createBtn, &QPushButton::clicked, this, &TaskCreateDialog::onCreateClicked);

    mainLayout->addWidget(footerFrame);

    // 来源和标签默认为空
    sourceCombo_->setCurrentText(QString());
    tagCombo_->setCurrentText(QString());
}

// ============================================================================
// 数据加载
// ============================================================================

void TaskCreateDialog::loadFormOptions()
{
    const TaskFormOptions options = TaskQueryUseCase().loadFormOptions();
    sourceCombo_->addItems(options.sources);
    tagCombo_->addItems(options.tags);
    handlerCombo_->addItems(options.handlers);
    sourceCombo_->setCurrentText(QString());
    tagCombo_->setCurrentText(QString());
    handlerCombo_->setCurrentText(QStringLiteral("本人"));
}

// ============================================================================
// 表单验证
// ============================================================================

bool TaskCreateDialog::validateForm()
{
    if (titleEdit_->text().trimmed().isEmpty()) {
        StyledMessageBox::warning(this, QStringLiteral("提示"),
                             QStringLiteral("请输入任务名称"));
        titleEdit_->setFocus();
        return false;
    }
    return true;
}

// ============================================================================
// 将手动录入的新来源/标签/办理人写入字典表
// ============================================================================

void TaskCreateDialog::ensureDictEntry()
{
    TaskFormUseCase().ensureDictionaryEntries(source(), tag(), handler());
}

// ============================================================================
// 槽：创建 / 取消
// ============================================================================

void TaskCreateDialog::onCreateClicked()
{
    if (!validateForm()) {
        return;
    }

    // 将手动录入的新来源/标签/办理人写入字典表
    ensureDictEntry();

    // 构造 Task 结构体
    Task task;
    task.title = title();
    task.category = category();
    task.source = source();
    task.tag = tag();
    task.handler = handler();
    if (task.handler.isEmpty()) {
        task.handler = QStringLiteral("本人");
    }
    task.deadline = deadline();
    task.isKeyTask = isKeyTask();
    task.hasSubdirs = hasSubdirs();
    task.description = description();

    // 通过 application 命令创建任务（含编号生成、状态设置、操作日志）
    TaskCommandUseCase taskCommand;
    TaskResult result = taskCommand.createTask(task, sourceFiles());
    if (!result.ok) {
        StyledMessageBox::warning(this, QStringLiteral("创建失败"), result.error);
        return;
    }

    auto created = taskCommand.findTask(result.taskId);
    QString actualTaskNo = created ? created->taskNo : QString();

    Logger::instance().info(QStringLiteral("任务已创建: %1").arg(actualTaskNo));

    emit taskCreated(actualTaskNo);
    QDialog::accept();  // 调用基类，避免递归调用自身的 accept()
}

void TaskCreateDialog::onCancelClicked()
{
    reject();
}

// ============================================================================
// 事件重写
// ============================================================================

void TaskCreateDialog::keyPressEvent(QKeyEvent *event)
{
    // Enter / Return → 创建任务
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        onCreateClicked();
        return;
    }
    // Ctrl+V 粘贴文件到附件区域（无文本框聚焦时）
    if (event->key() == Qt::Key_V && event->modifiers() == Qt::ControlModifier) {
        QWidget *focusWidget = QApplication::focusWidget();
        bool isTextInput = qobject_cast<QLineEdit *>(focusWidget)
                         || qobject_cast<QTextEdit *>(focusWidget)
                         || qobject_cast<QSpinBox *>(focusWidget)
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
    // Esc 由 QDialog 默认处理（调用 reject）
    QDialog::keyPressEvent(event);
}

bool TaskCreateDialog::eventFilter(QObject *watched, QEvent *event)
{
    // 截止日期为空时，点击下拉箭头后日历默认选中当天
    if (watched == deadlineEdit_ && event->type() == QEvent::MouseButtonPress) {
        if (deadlineEdit_->date() == deadlineEdit_->minimumDate()) {
            QTimer::singleShot(0, this, [this]() {
                QCalendarWidget *cal = deadlineEdit_->calendarWidget();
                if (cal) {
                    cal->setSelectedDate(QDate::currentDate());
                    cal->showSelectedDate();
                }
            });
        }
    }
    return QDialog::eventFilter(watched, event);
}

void TaskCreateDialog::accept()
{
    // 重写 accept：任何外部 accept 调用都走 onCreateClicked 验证流程
    onCreateClicked();
}

// ============================================================================
// 表单数据获取
// ============================================================================

QString TaskCreateDialog::title() const
{
    return titleEdit_->text().trimmed();
}

QString TaskCreateDialog::category() const
{
    return categoryCombo_->currentText();
}

QString TaskCreateDialog::source() const
{
    return sourceCombo_->currentText().trimmed();
}

QString TaskCreateDialog::tag() const
{
    return tagCombo_->currentText().trimmed();
}

QString TaskCreateDialog::handler() const
{
    return handlerCombo_->currentText().trimmed();
}

QDate TaskCreateDialog::deadline() const
{
    // 如果日期等于 minimumDate，视为未设置截止日期
    if (deadlineEdit_->date() == deadlineEdit_->minimumDate()) {
        return QDate();  // 无效日期
    }
    return deadlineEdit_->date();
}

bool TaskCreateDialog::isKeyTask() const
{
    return keyTaskRadio_->isChecked();
}

bool TaskCreateDialog::hasSubdirs() const
{
    return subdirRadio_->isChecked();
}

QString TaskCreateDialog::description() const
{
    return descEdit_->toPlainText();
}

QStringList TaskCreateDialog::sourceFiles() const
{
    return dropArea_->filePaths();
}
