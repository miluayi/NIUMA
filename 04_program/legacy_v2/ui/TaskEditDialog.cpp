/**
 * @file TaskEditDialog.cpp
 * @brief 任务编辑弹窗实现（T2.5）
 *
 * 实现：setupUI 搭建表单 / loadTaskData 预填任务数据 /
 *       loadSources/loadTags 加载字典 / validateForm 表单验证 / onSaveClicked 更新任务入库
 *
 * 依赖：ui/TaskEditDialog.h、ui/ColorPalette.h、ui/Spacing.h、ui/Radius.h、
 *       application/task、models/Task、utils/Logger
 * 文档：PRD 5.3、qt_implementation_guide.md 3.7、coding.md 第十一节、task.md T2.5
 */

#include "TaskEditDialog.h"

#include <QButtonGroup>
#include <QCalendarWidget>
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
#include <QPushButton>
#include <QRadioButton>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>

#include <optional>

#include "ColorPalette.h"
#include "Radius.h"
#include "Spacing.h"
#include "application/task/TaskFormUseCase.h"
#include "application/task/TaskCommandUseCase.h"
#include "application/task/TaskQueryUseCase.h"
#include "models/Task.h"
#include "utils/Logger.h"
#include "widgets/WindowDragger.h"
#include "widgets/RoundedCornerFilter.h"
#include "widgets/StyledMessageBox.h"

// ============================================================================
// 构造
// ============================================================================

TaskEditDialog::TaskEditDialog(int taskId, QWidget *parent)
    : QDialog(parent)
    , taskId_(taskId)
{
    setupUI();
    loadFormOptions();
    loadTaskData();
}

// ============================================================================
// UI 搭建
// ============================================================================

void TaskEditDialog::setupUI()
{
    // 窗口属性：无边框、模态、可调整大小
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setModal(true);
    auto *saveShortcut = new QShortcut(QKeySequence::Save, this);
    saveShortcut->setObjectName(QStringLiteral("shortcutSaveTask"));
    connect(saveShortcut, &QShortcut::activated, this, &TaskEditDialog::onSaveClicked);
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

    QLabel *titleLabel = new QLabel(QStringLiteral("编辑任务"), headerFrame);
    QFont titleFont(QStringLiteral("MiSans"), 18, QFont::Normal);
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

    connect(closeBtn, &QPushButton::clicked, this, &TaskEditDialog::onCancelClicked);

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

    // 表单网格（2 列）
    QGridLayout *formGrid = new QGridLayout();
    formGrid->setSpacing(Spacing::SM);
    formGrid->setColumnStretch(0, 1);
    formGrid->setColumnStretch(1, 1);

    // ---- 第一行：任务编号（只读，跨两列）----
    QLabel *taskNoLabel = new QLabel(QStringLiteral("任务编号"), bodyFrame);
    taskNoEdit_ = new QLineEdit(bodyFrame);
    taskNoEdit_->setReadOnly(true);
    taskNoEdit_->setStyleSheet(
        "QLineEdit { background: #F1F5F9; color: #64748B; "
        "border: 1px solid #E2E8F0; border-radius: 6px; padding: 6px 8px; }");

    formGrid->addWidget(taskNoLabel, 0, 0, 1, 2);
    formGrid->addWidget(taskNoEdit_, 1, 0, 1, 2);

    // ---- 第二行：任务名称 + 分类 ----
    QLabel *titleLabelField = new QLabel(QStringLiteral("任务名称 *"), bodyFrame);
    titleEdit_ = new QLineEdit(bodyFrame);
    titleEdit_->setObjectName("taskTitleEdit");
    titleEdit_->setPlaceholderText(QStringLiteral("如：股权转让专项核查"));

    QLabel *categoryLabel = new QLabel(QStringLiteral("分类 *"), bodyFrame);
    categoryCombo_ = new QComboBox(bodyFrame);
    categoryCombo_->addItem(QStringLiteral("催"));
    categoryCombo_->addItem(QStringLiteral("查"));
    categoryCombo_->addItem(QStringLiteral("报"));
    categoryCombo_->addItem(QStringLiteral("办"));

    formGrid->addWidget(titleLabelField, 2, 0);
    formGrid->addWidget(titleEdit_, 3, 0);
    formGrid->addWidget(categoryLabel, 2, 1);
    formGrid->addWidget(categoryCombo_, 3, 1);

    // ---- 第三行：来源 + 事项标签 ----
    QLabel *sourceLabel = new QLabel(QStringLiteral("来源"), bodyFrame);
    sourceCombo_ = new QComboBox(bodyFrame);
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
    tagCombo_->setEditable(true);
    tagCombo_->setInsertPolicy(QComboBox::NoInsert);
    tagCombo_->lineEdit()->setPlaceholderText(QStringLiteral("输入或选择标签"));
    {
        QCompleter *completer = new QCompleter(tagCombo_->model(), tagCombo_);
        completer->setFilterMode(Qt::MatchContains);
        completer->setCaseSensitivity(Qt::CaseInsensitive);
        tagCombo_->setCompleter(completer);
    }

    formGrid->addWidget(sourceLabel, 4, 0);
    formGrid->addWidget(sourceCombo_, 5, 0);
    formGrid->addWidget(tagLabel, 4, 1);
    formGrid->addWidget(tagCombo_, 5, 1);

    // ---- 第四行：办理人 + 截止日期 ----
    QLabel *handlerLabel = new QLabel(QStringLiteral("办理人"), bodyFrame);
    handlerCombo_ = new QComboBox(bodyFrame);
    handlerCombo_->setEditable(true);
    handlerCombo_->setInsertPolicy(QComboBox::NoInsert);
    handlerCombo_->lineEdit()->setPlaceholderText(QStringLiteral("输入或选择办理人"));
    {
        QCompleter *completer = new QCompleter(handlerCombo_->model(), handlerCombo_);
        completer->setFilterMode(Qt::MatchContains);
        completer->setCaseSensitivity(Qt::CaseInsensitive);
        handlerCombo_->setCompleter(completer);
    }

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

    formGrid->addWidget(handlerLabel, 6, 0);
    formGrid->addWidget(handlerCombo_, 7, 0);
    formGrid->addWidget(deadlineLabel, 6, 1);
    formGrid->addLayout(deadlineLayout, 7, 1);

    bodyLayout->addLayout(formGrid);

    // ---- 第五行：任务标记（不显示目录结构）----
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

    bodyLayout->addLayout(taskMarkLayout);
    bodyLayout->addSpacing(Spacing::SM);

    // ---- 任务要求 ----
    QLabel *descLabel = new QLabel(QStringLiteral("任务要求"), bodyFrame);
    bodyLayout->addWidget(descLabel);

    descEdit_ = new QTextEdit(bodyFrame);
    descEdit_->setObjectName("taskDescriptionEdit");
    descEdit_->setPlaceholderText(QStringLiteral("粘贴通知原文或录入任务要求..."));
    descEdit_->setFixedHeight(200);

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

    QPushButton *saveBtn = new QPushButton(QStringLiteral("保存修改"), footerFrame);
    saveBtn->setObjectName("btnPrimary");
    saveBtn->setCursor(Qt::PointingHandCursor);
    saveBtn->setFocusPolicy(Qt::NoFocus);
    saveBtn->setMinimumWidth(96);
    saveBtn->setDefault(true);

    footerLayout->addWidget(cancelBtn);
    footerLayout->addWidget(saveBtn);

    connect(cancelBtn, &QPushButton::clicked, this, &TaskEditDialog::onCancelClicked);
    connect(saveBtn, &QPushButton::clicked, this, &TaskEditDialog::onSaveClicked);

    mainLayout->addWidget(footerFrame);
}

// ============================================================================
// 数据加载
// ============================================================================

void TaskEditDialog::loadTaskData()
{
    // 通过 application 查询任务数据
    std::optional<Task> task = TaskCommandUseCase().findTask(taskId_);
    if (!task.has_value()) {
        Logger::instance().error(
            QStringLiteral("TaskEditDialog::loadTaskData 任务不存在: %1").arg(taskId_));
        StyledMessageBox::warning(this, QStringLiteral("提示"),
                             QStringLiteral("任务不存在或已被删除"));
        return;
    }

    originalTask_ = task.value();

    // 预填所有可编辑字段
    taskNoEdit_->setText(originalTask_.taskNo);
    titleEdit_->setText(originalTask_.title);
    categoryCombo_->setCurrentText(originalTask_.category);
    sourceCombo_->setCurrentText(originalTask_.source);
    tagCombo_->setCurrentText(originalTask_.tag);

    // 办理人：为空时回退为"本人"
    QString handler = originalTask_.handler;
    handlerCombo_->setCurrentText(handler.isEmpty() ? QStringLiteral("本人") : handler);

    // 截止日期：有效则预填，否则保持空（minimumDate 触发 specialValueText 空显示）
    if (originalTask_.deadline.isValid()) {
        deadlineEdit_->setDate(originalTask_.deadline);
    } else {
        deadlineEdit_->setDate(deadlineEdit_->minimumDate());
    }

    // 任务标记
    if (originalTask_.isKeyTask) {
        keyTaskRadio_->setChecked(true);
    } else {
        normalTaskRadio_->setChecked(true);
    }

    // 任务说明
    descEdit_->setPlainText(originalTask_.description);
}

void TaskEditDialog::loadFormOptions()
{
    const TaskFormOptions options = TaskQueryUseCase().loadFormOptions();
    sourceCombo_->addItems(options.sources);
    tagCombo_->addItems(options.tags);
    handlerCombo_->addItems(options.handlers);
}

// ============================================================================
// 将手动录入的新来源/标签/办理人写入字典表
// ============================================================================

void TaskEditDialog::ensureDictEntry()
{
    TaskFormUseCase().ensureDictionaryEntries(sourceCombo_->currentText(),
                                              tagCombo_->currentText(),
                                              handlerCombo_->currentText());
}

// ============================================================================
// 表单验证
// ============================================================================

bool TaskEditDialog::validateForm()
{
    if (titleEdit_->text().trimmed().isEmpty()) {
        StyledMessageBox::warning(this, QStringLiteral("提示"),
                             QStringLiteral("请输入任务名称"));
        titleEdit_->setFocus();
        return false;
    }
    if (categoryCombo_->currentText().trimmed().isEmpty()) {
        StyledMessageBox::warning(this, QStringLiteral("提示"),
                             QStringLiteral("请选择分类"));
        categoryCombo_->setFocus();
        return false;
    }
    return true;
}

// ============================================================================
// 槽：保存 / 取消
// ============================================================================

void TaskEditDialog::onSaveClicked()
{
    if (!validateForm()) {
        return;
    }

    // 将手动录入的新来源/标签/办理人写入字典表
    ensureDictEntry();

    // 构造 Task 对象：以原始任务为基底，仅更新可编辑字段
    // 保留 id/taskNo/status/hasSubdirs/folderPath/createTime 等不可变字段
    Task task = originalTask_;
    task.title = titleEdit_->text().trimmed();
    task.category = categoryCombo_->currentText();
    task.source = sourceCombo_->currentText().trimmed();
    task.tag = tagCombo_->currentText().trimmed();

    // 办理人为空时回退为"本人"
    QString handler = handlerCombo_->currentText().trimmed();
    task.handler = handler.isEmpty() ? QStringLiteral("本人") : handler;

    // 截止日期：如果等于 minimumDate，视为未设置截止日期
    if (deadlineEdit_->date() == deadlineEdit_->minimumDate()) {
        task.deadline = QDate();  // 无效日期，表示无截止日期
    } else {
        task.deadline = deadlineEdit_->date();
    }
    task.isKeyTask = keyTaskRadio_->isChecked();
    task.description = descEdit_->toPlainText();

    // application 命令保留办理人变更联动
    TaskResult result = TaskCommandUseCase().updateTask(task);
    if (!result.ok) {
        StyledMessageBox::warning(this, QStringLiteral("保存失败"), result.error);
        return;
    }

    Logger::instance().info(QStringLiteral("任务已更新: %1").arg(task.taskNo));

    emit taskUpdated();
    QDialog::accept();  // 调用基类，避免递归调用自身的 accept()
}

void TaskEditDialog::onCancelClicked()
{
    reject();
}

// ============================================================================
// 事件重写
// ============================================================================

void TaskEditDialog::keyPressEvent(QKeyEvent *event)
{
    // Enter / Return → 保存修改
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        onSaveClicked();
        return;
    }
    // Esc 由 QDialog 默认处理（调用 reject）
    QDialog::keyPressEvent(event);
}

bool TaskEditDialog::eventFilter(QObject *watched, QEvent *event)
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

void TaskEditDialog::accept()
{
    // 重写 accept：任何外部 accept 调用都走 onSaveClicked 验证流程
    onSaveClicked();
}
