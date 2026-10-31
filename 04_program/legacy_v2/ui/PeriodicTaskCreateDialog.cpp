/**
 * @file PeriodicTaskCreateDialog.cpp
 * @brief 周期任务模板创建/编辑弹窗实现（Phase 7）
 *
 * 实现：setupUI 搭建表单 / loadSources/loadTags 加载字典 /
 *       loadTemplateData 编辑模式预填 / validateForm 表单验证 /
 *       onSaveClicked 创建或更新模板
 *
 * 依赖：ui/PeriodicTaskCreateDialog.h、ui/ColorPalette.h、ui/Spacing.h、ui/Radius.h、
 *       application/periodic/PeriodicTaskUseCase、
 *       models/PeriodicTask、utils/Logger
 * 文档：Phase 7、PRD 9.3、qt_implementation_guide.md 3.7、coding.md 第十一节
 */

#include "PeriodicTaskCreateDialog.h"

#include <QCheckBox>
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
#include <QSpinBox>
#include <QTimeEdit>
#include <QVBoxLayout>

#include "ColorPalette.h"
#include "Radius.h"
#include "Spacing.h"
#include "application/periodic/PeriodicTaskUseCase.h"
#include "models/PeriodicTask.h"
#include "utils/Logger.h"
#include "widgets/WindowDragger.h"
#include "widgets/RoundedCornerFilter.h"
#include "widgets/StyledMessageBox.h"

// ============================================================================
// 周期类型与中文互转
// ============================================================================

QString PeriodicTaskCreateDialog::periodTypeToText(const QString &type)
{
    if (type == QStringLiteral("daily")) {
        return QStringLiteral("每日");
    }
    if (type == QStringLiteral("weekly")) {
        return QStringLiteral("每周");
    }
    if (type == QStringLiteral("monthly")) {
        return QStringLiteral("每月");
    }
    if (type == QStringLiteral("yearly")) {
        return QStringLiteral("每年");
    }
    return QStringLiteral("每日");
}

QString PeriodicTaskCreateDialog::textToPeriodType(const QString &text)
{
    if (text == QStringLiteral("每日")) {
        return QStringLiteral("daily");
    }
    if (text == QStringLiteral("每周")) {
        return QStringLiteral("weekly");
    }
    if (text == QStringLiteral("每月")) {
        return QStringLiteral("monthly");
    }
    if (text == QStringLiteral("每年")) {
        return QStringLiteral("yearly");
    }
    return QStringLiteral("daily");
}

// ============================================================================
// 构造
// ============================================================================

PeriodicTaskCreateDialog::PeriodicTaskCreateDialog(QWidget *parent)
    : QDialog(parent)
    , titleLabel_(nullptr)
    , nameEdit_(nullptr)
    , categoryCombo_(nullptr)
    , periodTypeCombo_(nullptr)
    , periodValueSpin_(nullptr)
    , sourceCombo_(nullptr)
    , tagCombo_(nullptr)
    , handlerEdit_(nullptr)
    , reminderTimeEdit_(nullptr)
    , enabledCheck_(nullptr)
    , saveBtn_(nullptr)
    , targetDayRow_(nullptr)
    , targetDayCombo_(nullptr)
    , targetDayDateEdit_(nullptr)
    , editingId_(0)
{
    setupUI();
    loadSources();
    loadTags();
}

PeriodicTaskCreateDialog::PeriodicTaskCreateDialog(int id, QWidget *parent)
    : QDialog(parent)
    , titleLabel_(nullptr)
    , nameEdit_(nullptr)
    , categoryCombo_(nullptr)
    , periodTypeCombo_(nullptr)
    , periodValueSpin_(nullptr)
    , sourceCombo_(nullptr)
    , tagCombo_(nullptr)
    , handlerEdit_(nullptr)
    , reminderTimeEdit_(nullptr)
    , enabledCheck_(nullptr)
    , saveBtn_(nullptr)
    , targetDayRow_(nullptr)
    , targetDayCombo_(nullptr)
    , targetDayDateEdit_(nullptr)
    , editingId_(id)
{
    setupUI();
    loadSources();
    loadTags();
    loadTemplateData();
}

// ============================================================================
// UI 搭建
// ============================================================================

void PeriodicTaskCreateDialog::setupUI()
{
    // 窗口属性：无边框、模态、可调整大小
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setModal(true);
    auto *saveShortcut = new QShortcut(QKeySequence::Save, this);
    saveShortcut->setObjectName(QStringLiteral("shortcutSavePeriodicTask"));
    connect(saveShortcut, &QShortcut::activated, this, &PeriodicTaskCreateDialog::onSaveClicked);
    setMinimumWidth(640);
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
    headerLayout->setContentsMargins(Spacing::XL, Spacing::LG, Spacing::XL, Spacing::LG);
    headerLayout->setSpacing(Spacing::SM);

    titleLabel_ = new QLabel(
        editingId_ > 0 ? QStringLiteral("编辑周期模板") : QStringLiteral("新建周期模板"),
        headerFrame);
    QFont titleFont(QStringLiteral("Segoe UI"), 16, QFont::Normal);
    titleLabel_->setFont(titleFont);
    titleLabel_->setStyleSheet("color: #0F172A; border: none;");

    QPushButton *closeBtn = new QPushButton(QStringLiteral("\u2715"), headerFrame);
    closeBtn->setObjectName("dialogCloseBtn");
    closeBtn->setFixedSize(28, 28);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setFocusPolicy(Qt::NoFocus);
    closeBtn->setStyleSheet(
        "#dialogCloseBtn { border: none; background: transparent; "
        "font-size: 14px; color: #94A3B8; border-radius: 6px; }"
        "#dialogCloseBtn:hover { color: #0F172A; background: #F5F7FA; }");

    headerLayout->addWidget(titleLabel_);
    headerLayout->addStretch();
    headerLayout->addWidget(closeBtn);

    connect(closeBtn, &QPushButton::clicked, this, &PeriodicTaskCreateDialog::onCancelClicked);

    mainLayout->addWidget(headerFrame);

    // ---- 表单区域 ----
    QFrame *bodyFrame = new QFrame(this);
    bodyFrame->setObjectName("dialogBody");
    bodyFrame->setStyleSheet(
        "#dialogBody { background: #FFFFFF; }"
        "#dialogBody QLabel { color: #475569; font-size: 13px; }");
    QVBoxLayout *bodyLayout = new QVBoxLayout(bodyFrame);
    bodyLayout->setContentsMargins(Spacing::XL, Spacing::LG, Spacing::XL, Spacing::LG);
    bodyLayout->setSpacing(Spacing::MD);

    // 通用标签样式
    auto makeLabel = [bodyFrame](const QString &text) {
        QLabel *lbl = new QLabel(text, bodyFrame);
        lbl->setStyleSheet("color:#475569; font-size:13px; font-weight:400;");
        return lbl;
    };

    // 通用输入控件样式
    const QString inputStyle =
        "QLineEdit, QComboBox, QSpinBox, QTimeEdit {"
        "  padding:7px 10px;"
        "  border:1px solid #E2E8F0;"
        "  border-radius:6px;"
        "  font-size:14px;"
        "  background:#FFFFFF;"
        "  color:#0F172A;"
        "  min-height:20px;"
        "}"
        "QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QTimeEdit:focus {"
        "  border:2px solid #2463EB;"
        "  padding:6px 9px;"
        "}"
        "QComboBox::drop-down {"
        "  border:none;"
        "  width:22px;"
        "}"
        "QComboBox QAbstractItemView {"
        "  border:1px solid #E2E8F0;"
        "  border-radius:6px;"
        "  background:#FFFFFF;"
        "  selection-background-color:#F5F7FA;"
        "  selection-color:#2463EB;"
        "  outline:none;"
        "  padding:4px;"
        "}";

    bodyFrame->setStyleSheet(bodyFrame->styleSheet() + inputStyle);

    // ---- 模板名称（必填） ----
    bodyLayout->addWidget(makeLabel(QStringLiteral("模板名称 *")));
    nameEdit_ = new QLineEdit(bodyFrame);
    nameEdit_->setObjectName("periodicTemplateNameEdit");
    nameEdit_->setPlaceholderText(QStringLiteral("如：月度工作总结"));
    bodyLayout->addWidget(nameEdit_);

    // ---- 分类 + 周期类型 ----
    QGridLayout *gridLayout = new QGridLayout();
    gridLayout->setSpacing(Spacing::SM);
    gridLayout->setColumnStretch(0, 1);
    gridLayout->setColumnStretch(1, 1);

    QLabel *categoryLabel = makeLabel(QStringLiteral("分类"));
    categoryCombo_ = new QComboBox(bodyFrame);
    categoryCombo_->setObjectName("periodicCategoryCombo");
    categoryCombo_->addItem(QStringLiteral("催"));
    categoryCombo_->addItem(QStringLiteral("查"));
    categoryCombo_->addItem(QStringLiteral("报"));
    categoryCombo_->addItem(QStringLiteral("办"));
    categoryCombo_->setCurrentText(QStringLiteral("报"));  // 默认"报"

    QLabel *periodTypeLabel = makeLabel(QStringLiteral("周期类型"));
    periodTypeCombo_ = new QComboBox(bodyFrame);
    periodTypeCombo_->setObjectName("periodicPeriodTypeCombo");
    periodTypeCombo_->addItem(QStringLiteral("每日"));
    periodTypeCombo_->addItem(QStringLiteral("每周"));
    periodTypeCombo_->addItem(QStringLiteral("每月"));
    periodTypeCombo_->addItem(QStringLiteral("每年"));
    periodTypeCombo_->setCurrentText(QStringLiteral("每月"));  // 默认"每月"

    gridLayout->addWidget(categoryLabel, 0, 0);
    gridLayout->addWidget(categoryCombo_, 1, 0);
    gridLayout->addWidget(periodTypeLabel, 0, 1);
    gridLayout->addWidget(periodTypeCombo_, 1, 1);
    bodyLayout->addLayout(gridLayout);

    // ---- 周期值 ----
    QHBoxLayout *periodValueLayout = new QHBoxLayout();
    periodValueLayout->setSpacing(Spacing::SM);
    QLabel *periodValueLabel = makeLabel(QStringLiteral("周期值"));
    periodValueSpin_ = new QSpinBox(bodyFrame);
    periodValueSpin_->setMinimum(1);
    periodValueSpin_->setMaximum(365);
    periodValueSpin_->setValue(1);
    periodValueSpin_->setFixedWidth(100);
    QLabel *periodValueHint = new QLabel(QStringLiteral("如：每 2 周 / 每 3 月"), bodyFrame);
    periodValueHint->setStyleSheet("color:#94A3B8; font-size:12px;");

    periodValueLayout->addWidget(periodValueLabel);
    periodValueLayout->addWidget(periodValueSpin_);
    periodValueLayout->addWidget(periodValueHint, 1);
    bodyLayout->addLayout(periodValueLayout);

    // ---- 执行日（根据周期类型动态切换） ----
    targetDayRow_ = new QFrame(bodyFrame);
    targetDayRow_->setObjectName("targetDayRow");
    targetDayRow_->setStyleSheet("#targetDayRow { border: none; background: transparent; }");
    QHBoxLayout *targetDayLayout = new QHBoxLayout(targetDayRow_);
    targetDayLayout->setContentsMargins(0, 0, 0, 0);
    targetDayLayout->setSpacing(Spacing::SM);

    QLabel *targetDayLabel = makeLabel(QStringLiteral("执行日"));
    targetDayLayout->addWidget(targetDayLabel);

    // 每周/每月用的下拉框
    targetDayCombo_ = new QComboBox(targetDayRow_);
    targetDayCombo_->setMinimumWidth(120);
    targetDayLayout->addWidget(targetDayCombo_);

    // 每年用的日期选择器（只显示 MM月dd日）
    targetDayDateEdit_ = new QDateEdit(targetDayRow_);
    targetDayDateEdit_->setCalendarPopup(true);
    targetDayDateEdit_->setDisplayFormat("MM月dd日");
    targetDayDateEdit_->setDate(QDate::currentDate());
    targetDayDateEdit_->setMinimumWidth(120);
    targetDayDateEdit_->hide();  // 默认隐藏，onPeriodTypeChanged 会按需显示
    targetDayLayout->addWidget(targetDayDateEdit_);

    targetDayLayout->addStretch();
    bodyLayout->addWidget(targetDayRow_);

    // 周期类型变化时切换执行日控件
    connect(periodTypeCombo_, &QComboBox::currentTextChanged,
            this, &PeriodicTaskCreateDialog::onPeriodTypeChanged);
    // 初始化执行日控件状态（默认"每月"）
    onPeriodTypeChanged();

    // ---- 来源 + 事项标签 ----
    QGridLayout *grid2 = new QGridLayout();
    grid2->setSpacing(Spacing::SM);
    grid2->setColumnStretch(0, 1);
    grid2->setColumnStretch(1, 1);

    QLabel *sourceLabel = makeLabel(QStringLiteral("来源"));
    sourceCombo_ = new QComboBox(bodyFrame);
    sourceCombo_->setObjectName(QStringLiteral("periodicSourceCombo"));
    sourceCombo_->setEditable(true);
    sourceCombo_->setInsertPolicy(QComboBox::NoInsert);
    sourceCombo_->lineEdit()->setPlaceholderText(QStringLiteral("输入或选择来源"));
    {
        QCompleter *completer = new QCompleter(sourceCombo_->model(), sourceCombo_);
        completer->setFilterMode(Qt::MatchContains);
        completer->setCaseSensitivity(Qt::CaseInsensitive);
        sourceCombo_->setCompleter(completer);
    }

    QLabel *tagLabel = makeLabel(QStringLiteral("事项标签"));
    tagCombo_ = new QComboBox(bodyFrame);
    tagCombo_->setObjectName(QStringLiteral("periodicTagCombo"));
    tagCombo_->setEditable(true);
    tagCombo_->setInsertPolicy(QComboBox::NoInsert);
    tagCombo_->lineEdit()->setPlaceholderText(QStringLiteral("输入或选择标签"));
    {
        QCompleter *completer = new QCompleter(tagCombo_->model(), tagCombo_);
        completer->setFilterMode(Qt::MatchContains);
        completer->setCaseSensitivity(Qt::CaseInsensitive);
        tagCombo_->setCompleter(completer);
    }

    grid2->addWidget(sourceLabel, 0, 0);
    grid2->addWidget(sourceCombo_, 1, 0);
    grid2->addWidget(tagLabel, 0, 1);
    grid2->addWidget(tagCombo_, 1, 1);
    bodyLayout->addLayout(grid2);

    // ---- 办理人 + 提醒时间 ----
    QGridLayout *grid3 = new QGridLayout();
    grid3->setSpacing(Spacing::SM);
    grid3->setColumnStretch(0, 1);
    grid3->setColumnStretch(1, 1);

    QLabel *handlerLabel = makeLabel(QStringLiteral("办理人"));
    handlerEdit_ = new QLineEdit(bodyFrame);
    handlerEdit_->setText(QStringLiteral("本人"));
    handlerEdit_->setPlaceholderText(QStringLiteral("默认 本人"));

    QLabel *reminderLabel = makeLabel(QStringLiteral("提醒时间"));
    reminderTimeEdit_ = new QTimeEdit(bodyFrame);
    reminderTimeEdit_->setDisplayFormat("HH:mm");
    reminderTimeEdit_->setTime(QTime(9, 0));  // 默认 09:00

    grid3->addWidget(handlerLabel, 0, 0);
    grid3->addWidget(handlerEdit_, 1, 0);
    grid3->addWidget(reminderLabel, 0, 1);
    grid3->addWidget(reminderTimeEdit_, 1, 1);
    bodyLayout->addLayout(grid3);

    // ---- 启用 ----
    enabledCheck_ = new QCheckBox(QStringLiteral("启用此模板"), bodyFrame);
    enabledCheck_->setObjectName("periodicTemplateEnabledCheck");
    enabledCheck_->setChecked(true);
    enabledCheck_->setStyleSheet(
        "QCheckBox { color:#0F172A; font-size:14px; spacing:8px; }"
        "QCheckBox::indicator { width:16px; height:16px; border-radius:3px; "
        "  border:1px solid #CBD5E1; background:#FFFFFF; }"
        "QCheckBox::indicator:checked { background:#2463EB; border-color:#2463EB; }");
    bodyLayout->addWidget(enabledCheck_);

    mainLayout->addWidget(bodyFrame);

    // ---- 底部按钮栏 ----
    QFrame *footerFrame = new QFrame(this);
    footerFrame->setObjectName("dialogFooter");
    footerFrame->setStyleSheet(
        "#dialogFooter { background: #FFFFFF; border-top: 1px solid #E2E8F0; }");
    QHBoxLayout *footerLayout = new QHBoxLayout(footerFrame);
    footerLayout->setContentsMargins(Spacing::XL, Spacing::MD, Spacing::XL, Spacing::MD);
    footerLayout->addStretch();

    QPushButton *cancelBtn = new QPushButton(QStringLiteral("取消"), footerFrame);
    cancelBtn->setObjectName("btnSecondary");
    cancelBtn->setCursor(Qt::PointingHandCursor);
    cancelBtn->setFocusPolicy(Qt::NoFocus);
    cancelBtn->setMinimumWidth(88);
    cancelBtn->setStyleSheet(
        "QPushButton#btnSecondary {"
        "  background:#FFFFFF;"
        "  color:#475569;"
        "  border:1px solid #E2E8F0;"
        "  border-radius:6px;"
        "  padding:8px 16px;"
        "  font-size:14px;"
        "}"
        "QPushButton#btnSecondary:hover {"
        "  background:#F5F7FA;"
        "  border-color:#CBD5E1;"
        "}");

    saveBtn_ = new QPushButton(
        editingId_ > 0 ? QStringLiteral("保存修改") : QStringLiteral("创建"),
        footerFrame);
    saveBtn_->setObjectName("btnPrimary");
    saveBtn_->setCursor(Qt::PointingHandCursor);
    saveBtn_->setFocusPolicy(Qt::NoFocus);
    saveBtn_->setMinimumWidth(88);
    saveBtn_->setDefault(true);
    saveBtn_->setStyleSheet(
        "QPushButton#btnPrimary {"
        "  background:#2463EB;"
        "  color:#FFFFFF;"
        "  border:1px solid #2463EB;"
        "  border-radius:6px;"
        "  padding:8px 16px;"
        "  font-size:14px;"
        "  font-weight:700;"
        "}"
        "QPushButton#btnPrimary:hover {"
        "  background:#1D4ED8;"
        "}");

    footerLayout->addWidget(cancelBtn);
    footerLayout->addWidget(saveBtn_);

    connect(cancelBtn, &QPushButton::clicked, this, &PeriodicTaskCreateDialog::onCancelClicked);
    connect(saveBtn_, &QPushButton::clicked, this, &PeriodicTaskCreateDialog::onSaveClicked);

    mainLayout->addWidget(footerFrame);
}

// ============================================================================
// 数据加载
// ============================================================================

void PeriodicTaskCreateDialog::loadSources()
{
    sourceCombo_->addItems(PeriodicTaskUseCase().loadFormOptions().sources);
    if (editingId_ == 0) sourceCombo_->setCurrentText(QString());
}

void PeriodicTaskCreateDialog::loadTags()
{
    tagCombo_->addItems(PeriodicTaskUseCase().loadFormOptions().tags);
    if (editingId_ == 0) tagCombo_->setCurrentText(QString());
}

void PeriodicTaskCreateDialog::loadTemplateData()
{
    auto tmpl = PeriodicTaskUseCase().find(editingId_);
    if (!tmpl.has_value()) {
        Logger::instance().error(
            QStringLiteral("PeriodicTaskCreateDialog::loadTemplateData 模板不存在: id=%1").arg(editingId_));
        StyledMessageBox::warning(this, QStringLiteral("提示"),
                             QStringLiteral("周期任务模板不存在"));
        return;
    }

    originalTask_ = tmpl.value();

    // 预填字段
    nameEdit_->setText(originalTask_.name);
    categoryCombo_->setCurrentText(originalTask_.category);
    periodTypeCombo_->setCurrentText(periodTypeToText(originalTask_.periodType));
    // setCurrentText 会触发 onPeriodTypeChanged()，自动填充执行日选项
    // 之后根据 originalTask_.targetDay 恢复具体选择
    periodValueSpin_->setValue(originalTask_.periodValue > 0 ? originalTask_.periodValue : 1);
    sourceCombo_->setCurrentText(originalTask_.source);
    tagCombo_->setCurrentText(originalTask_.tag);
    handlerEdit_->setText(originalTask_.handler.isEmpty() ? QStringLiteral("本人") : originalTask_.handler);

    // 恢复执行日选择
    if (originalTask_.targetDay > 0) {
        QString periodType = originalTask_.periodType;
        if (periodType == QStringLiteral("weekly") || periodType == QStringLiteral("monthly")) {
            int idx = targetDayCombo_->findData(originalTask_.targetDay);
            if (idx >= 0) {
                targetDayCombo_->setCurrentIndex(idx);
            }
        } else if (periodType == QStringLiteral("yearly")) {
            int month = originalTask_.targetDay / 100;
            int day = originalTask_.targetDay % 100;
            if (month >= 1 && month <= 12 && day >= 1 && day <= 31) {
                QDate d(QDate::currentDate().year(), month, 1);
                day = qMin(day, d.daysInMonth());
                QDate targetDate(QDate::currentDate().year(), month, day);
                if (targetDate.isValid()) {
                    targetDayDateEdit_->setDate(targetDate);
                }
            }
        }
    }

    // 提醒时间
    if (!originalTask_.reminderTime.isEmpty()) {
        QTime t = QTime::fromString(originalTask_.reminderTime, QStringLiteral("HH:mm"));
        if (t.isValid()) {
            reminderTimeEdit_->setTime(t);
        }
    }

    enabledCheck_->setChecked(originalTask_.isEnabled);
}

// ============================================================================
// 表单验证
// ============================================================================

bool PeriodicTaskCreateDialog::validateForm()
{
    if (nameEdit_->text().trimmed().isEmpty()) {
        StyledMessageBox::warning(this, QStringLiteral("提示"),
                             QStringLiteral("请输入模板名称"));
        nameEdit_->setFocus();
        return false;
    }
    return true;
}

// ============================================================================
// 槽：保存 / 取消
// ============================================================================

void PeriodicTaskCreateDialog::onSaveClicked()
{
    if (!validateForm()) {
        return;
    }

    // 构造 PeriodicTask
    PeriodicTask tmpl = getPeriodicTask();

    TaskResult result;
    if (editingId_ > 0) {
        // 编辑模式：保留 id，调用 update
        tmpl.id = editingId_;
        tmpl.createTime = originalTask_.createTime;
        result = PeriodicTaskUseCase().update(tmpl);
    } else {
        // 新建模式
        result = PeriodicTaskUseCase().create(tmpl);
    }

    if (!result.ok) {
        StyledMessageBox::warning(this, QStringLiteral("保存失败"), result.error);
        return;
    }

    Logger::instance().info(
        QStringLiteral("PeriodicTaskCreateDialog 已保存: id=%1 name=%2")
            .arg(result.taskId).arg(tmpl.name));

    emit templateSaved();
    QDialog::accept();
}

void PeriodicTaskCreateDialog::onCancelClicked()
{
    reject();
}

// ============================================================================
// 事件重写
// ============================================================================

void PeriodicTaskCreateDialog::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        onSaveClicked();
        return;
    }
    QDialog::keyPressEvent(event);
}

void PeriodicTaskCreateDialog::accept()
{
    // 任何外部 accept 调用都走 onSaveClicked 验证流程
    onSaveClicked();
}

// ============================================================================
// 槽：周期类型变化时切换执行日控件
// ============================================================================

void PeriodicTaskCreateDialog::onPeriodTypeChanged()
{
    if (!periodTypeCombo_ || !targetDayRow_ || !targetDayCombo_ || !targetDayDateEdit_) {
        return;
    }

    QString typeText = periodTypeCombo_->currentText();
    QString periodType = textToPeriodType(typeText);

    // 保存当前选择值（切换前），以便切换后尽量恢复
    int prevValue = targetDayCombo_->currentData().toInt();

    // 先清空下拉框
    targetDayCombo_->blockSignals(true);
    targetDayCombo_->clear();

    if (periodType == QStringLiteral("daily")) {
        // 每日：不需要执行日，隐藏整行
        targetDayRow_->hide();
    } else if (periodType == QStringLiteral("weekly")) {
        // 每周：周一~周日（1-7）
        targetDayRow_->show();
        targetDayCombo_->show();
        targetDayDateEdit_->hide();
        const QStringList weekDays = {
            QStringLiteral("周一"), QStringLiteral("周二"), QStringLiteral("周三"),
            QStringLiteral("周四"), QStringLiteral("周五"), QStringLiteral("周六"),
            QStringLiteral("周日")
        };
        for (int i = 0; i < weekDays.size(); ++i) {
            targetDayCombo_->addItem(weekDays[i], i + 1);  // data = 1~7
        }
        // 恢复或默认选中
        int idx = targetDayCombo_->findData(prevValue);
        if (idx < 0 || prevValue < 1 || prevValue > 7) {
            idx = targetDayCombo_->findData(QDate::currentDate().dayOfWeek());
        }
        targetDayCombo_->setCurrentIndex(idx >= 0 ? idx : 0);
    } else if (periodType == QStringLiteral("monthly")) {
        // 每月：1号~31号（1-31）
        targetDayRow_->show();
        targetDayCombo_->show();
        targetDayDateEdit_->hide();
        for (int i = 1; i <= 31; ++i) {
            targetDayCombo_->addItem(QStringLiteral("%1号").arg(i), i);
        }
        // 恢复或默认选中当天
        int idx = targetDayCombo_->findData(prevValue);
        if (idx < 0 || prevValue < 1 || prevValue > 31) {
            idx = targetDayCombo_->findData(QDate::currentDate().day());
        }
        targetDayCombo_->setCurrentIndex(idx >= 0 ? idx : 0);
    } else if (periodType == QStringLiteral("yearly")) {
        // 每年：MM月dd日
        targetDayRow_->show();
        targetDayCombo_->hide();
        targetDayDateEdit_->show();
        // 默认选中当天（年份不影响，只取月日）
        if (prevValue > 0) {
            int month = prevValue / 100;
            int day = prevValue % 100;
            if (month >= 1 && month <= 12 && day >= 1 && day <= 31) {
                QDate d(QDate::currentDate().year(), month, qMin(day, QDate(QDate::currentDate().year(), month, 1).daysInMonth()));
                if (d.isValid()) {
                    targetDayDateEdit_->setDate(d);
                }
            }
        }
    }

    targetDayCombo_->blockSignals(false);
}

// ============================================================================
// 表单数据获取
// ============================================================================

PeriodicTask PeriodicTaskCreateDialog::getPeriodicTask() const
{
    PeriodicTask tmpl;
    // 编辑模式保留原 id 与 createTime
    if (editingId_ > 0) {
        tmpl.id = editingId_;
        tmpl.createTime = originalTask_.createTime;
        // 保留 lastRemindedDate，不在表单中编辑
        tmpl.lastRemindedDate = originalTask_.lastRemindedDate;
    }

    tmpl.name = nameEdit_->text().trimmed();
    tmpl.category = categoryCombo_->currentText();
    tmpl.periodType = textToPeriodType(periodTypeCombo_->currentText());
    tmpl.periodValue = periodValueSpin_->value();
    tmpl.source = sourceCombo_->currentText().trimmed();
    tmpl.tag = tagCombo_->currentText().trimmed();
    tmpl.handler = handlerEdit_->text().trimmed();
    if (tmpl.handler.isEmpty()) {
        tmpl.handler = QStringLiteral("本人");
    }
    tmpl.reminderTime = reminderTimeEdit_->time().toString(QStringLiteral("HH:mm"));
    tmpl.isEnabled = enabledCheck_->isChecked();

    // 计算执行日 targetDay
    // 0=不指定(每日)，weekly:1-7，monthly:1-31，yearly:MMDD(如315=3月15日)
    if (tmpl.periodType == QStringLiteral("daily")) {
        tmpl.targetDay = 0;
    } else if (tmpl.periodType == QStringLiteral("weekly") || tmpl.periodType == QStringLiteral("monthly")) {
        tmpl.targetDay = targetDayCombo_->currentData().toInt();
    } else if (tmpl.periodType == QStringLiteral("yearly")) {
        QDate d = targetDayDateEdit_->date();
        tmpl.targetDay = d.month() * 100 + d.day();
    } else {
        tmpl.targetDay = 0;
    }

    return tmpl;
}
