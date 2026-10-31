/**
 * @file SettingsView.cpp
 * @brief 设置中心页面实现（T4.6）
 *
 * 实现：setupUI 搭建三卡片布局 / loadConfig 从 settings 表读取导出配置 /
 *       onSaveClicked 保存配置 / onResetDefaultsClicked 恢复默认 /
 *       loadWorkDir 读取工作目录 / loadLastBackupTime 读取最近备份时间
 *
 * 依赖：ui/SettingsView.h、ui/Spacing.h、application/settings/SettingsUseCase、
 *       models/ExportConfig、utils/Logger
 * 文档：task.md T4.6、PRD 13.2
 */

#include "SettingsView.h"
#include "src/AppVersion.h"

#include <QCheckBox>
#include <QButtonGroup>
#include <QComboBox>
#include <QDesktopServices>
#include <QFrame>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QRadioButton>
#include <QSignalBlocker>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

#include "Spacing.h"
#include "application/settings/SettingsUseCase.h"
#include "utils/Logger.h"
#include "widgets/StyledMessageBox.h"

namespace {
    // 次要按钮 QSS（白底灰字带边框）
    const char *kSecondaryBtnQss =
        "QPushButton {"
        "  background:#FFFFFF;"
        "  color:#475569;"
        "  border:1px solid #E2E8F0;"
        "  border-radius:6px;"
        "  padding:6px 18px;"
        "  font-size:14px;"
        "}"
        "QPushButton:hover {"
        "  background:#F5F7FA;"
        "  border-color:#CBD5E1;"
        "}";

    // 主要按钮 QSS（蓝底白字）
    const char *kPrimaryBtnQss =
        "QPushButton {"
        "  background:#2463EB;"
        "  color:#FFFFFF;"
        "  border:1px solid #2463EB;"
        "  border-radius:6px;"
        "  padding:6px 20px;"
        "  font-size:14px;"
        "  font-weight:700;"
        "}"
        "QPushButton:hover {"
        "  background:#1D4ED8;"
        "}";

    // 输入框 QSS
    const char *kLineEditQss =
        "QLineEdit {"
        "  padding:10px 12px;"
        "  border:1px solid #E2E8F0;"
        "  border-radius:8px;"
        "  font-size:14px;"
        "  background:#FFFFFF;"
        "  color:#0F172A;"
        "}"
        "QLineEdit:focus {"
        "  border:2px solid #2463EB;"
        "  padding:9px 11px;"
        "}";

    // 下拉框 QSS
    const char *kComboQss =
        "QComboBox {"
        "  padding:10px 12px;"
        "  border:1px solid #E2E8F0;"
        "  border-radius:8px;"
        "  font-size:14px;"
        "  background:#FFFFFF;"
        "  color:#0F172A;"
        "  min-width:160px;"
        "}"
        "QComboBox::drop-down {"
        "  border:none;"
        "  width:24px;"
        "}"
        "QComboBox QAbstractItemView {"
        "  border:1px solid #E2E8F0;"
        "  border-radius:8px;"
        "  background:#FFFFFF;"
        "  selection-background-color:#F5F7FA;"
        "  selection-color:#2463EB;"
        "  outline:none;"
        "  padding:4px;"
        "}";

    // 复选框 QSS
    const char *kCheckQss =
        "QCheckBox {"
        "  color:#0F172A;"
        "  font-size:14px;"
        "  spacing:8px;"
        "  padding:8px 0px;"
        "}"
        "QCheckBox::indicator {"
        "  width:18px;"
        "  height:18px;"
        "  border:1px solid #CBD5E1;"
        "  border-radius:4px;"
        "  background:#FFFFFF;"
        "}"
        "QCheckBox::indicator:checked {"
        "  background:#2463EB;"
        "  border:1px solid #2463EB;"
        "}";

    // 卡片标题 QSS
    const char *kCardTitleQss =
        "font-size:18px; font-weight:700; color:#0F172A; border:none;";
}

// ============================================================================
// 构造
// ============================================================================

SettingsView::SettingsView(QWidget *parent)
    : QWidget(parent)
    , workDirLabel_(nullptr)
    , changeDirBtn_(nullptr)
    , openDirBtn_(nullptr)
    , reportTitleEdit_(nullptr)
    , includeAchievementsCheck_(nullptr)
    , includeCategoryStatsCheck_(nullptr)
    , includeSourceStatsCheck_(nullptr)
    , includeAmountSummaryCheck_(nullptr)
    , includeKeyAchievementsCheck_(nullptr)
    , sortByCombo_(nullptr)
    , dateFormatCombo_(nullptr)
    , amountUnitCombo_(nullptr)
    , saveBtn_(nullptr)
    , resetBtn_(nullptr)
    , statusLabel_(nullptr)
    , fontSizeCombo_(nullptr)
    , fontWeightGroup_(nullptr)
    , lastBackupLabel_(nullptr)
    , backupBtn_(nullptr)
    , restoreBtn_(nullptr)
    , sourceList_(nullptr)
    , sourceInput_(nullptr)
    , tagList_(nullptr)
    , tagInput_(nullptr)
    , handlerList_(nullptr)
    , handlerInput_(nullptr)
{
    setupUI();
    loadConfig();
    const UiFontPreferences fontPreferences = SettingsUseCase().uiFontPreferences();
    const QSignalBlocker sizeBlocker(fontSizeCombo_);
    const QSignalBlocker weightBlocker(fontWeightGroup_);
    fontSizeCombo_->setCurrentIndex(fontSizeCombo_->findData(fontPreferences.pixelSize));
    if (auto *weightButton = fontWeightGroup_->button(static_cast<int>(fontPreferences.weight))) {
        weightButton->setChecked(true);
    }
    for (QLabel *sample : findChildren<QLabel *>(QStringLiteral("fontWeightSample"))) {
        QFont font = sample->font();
        font.setPixelSize(fontPreferences.pixelSize);
        sample->setFont(font);
    }
    loadWorkDir();
    loadLastBackupTime();
    loadSourceList();
    loadTagList();
    loadHandlerList();
}

// ============================================================================
// UI 构建
// ============================================================================

void SettingsView::setupUI()
{
    setObjectName("settingsView");
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(Spacing::XXL, Spacing::XL, Spacing::XXL, Spacing::XL);
    mainLayout->setSpacing(Spacing::LG);

    mainLayout->addWidget(buildHeader());
    mainLayout->addWidget(buildWorkDirCard());
    mainLayout->addWidget(buildDictManagementCard());
    mainLayout->addWidget(buildExportCard());
    mainLayout->addWidget(buildFontCard());
    mainLayout->addWidget(buildDataMgmtCard());
    mainLayout->addWidget(buildAboutCard());
    mainLayout->addStretch();
}

QFrame* SettingsView::buildHeader()
{
    QFrame *header = new QFrame(this);
    header->setObjectName("settingsHeader");
    QVBoxLayout *layout = new QVBoxLayout(header);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(Spacing::XS);

    QLabel *titleLabel = new QLabel(QStringLiteral("设置中心"), header);
    titleLabel->setObjectName("settingsTitle");
    titleLabel->setFont(QFont(QStringLiteral("Segoe UI"), 22, QFont::Normal));
    titleLabel->setStyleSheet("color:#0F172A; border:none;");
    layout->addWidget(titleLabel);

    QLabel *subtitleLabel = new QLabel(QStringLiteral("管理应用配置和导出选项"), header);
    subtitleLabel->setObjectName("settingsSubtitle");
    subtitleLabel->setFont(QFont(QStringLiteral("Segoe UI"), 14));
    subtitleLabel->setStyleSheet("color:#0F172A; border:none;");
    layout->addWidget(subtitleLabel);

    return header;
}

QFrame* SettingsView::buildWorkDirCard()
{
    QFrame *card = new QFrame(this);
    card->setObjectName("workDirCard");
    card->setStyleSheet(
        "QFrame#workDirCard { background:#FFFFFF; border:1px solid #DCE3EF; border-radius:12px; }");
    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(Spacing::XL, Spacing::XL, Spacing::XL, Spacing::XL);
    layout->setSpacing(Spacing::LG);

    QLabel *title = new QLabel(QStringLiteral("工作目录"), card);
    title->setStyleSheet(kCardTitleQss);
    layout->addWidget(title);

    // 当前路径显示（只读灰底）
    workDirLabel_ = new QLabel(QStringLiteral("未设置"), card);
    workDirLabel_->setObjectName("settingsWorkDirLabel");
    workDirLabel_->setWordWrap(true);
    workDirLabel_->setStyleSheet(
        "font-size:14px; color:#0F172A; border:none; "
        "background:#F8FAFC; padding:10px 12px; border-radius:8px;");
    layout->addWidget(workDirLabel_);

    // 按钮行
    QHBoxLayout *btnRow = new QHBoxLayout();
    btnRow->setSpacing(Spacing::SM);

    changeDirBtn_ = new QPushButton(QStringLiteral("更改目录"), card);
    changeDirBtn_->setObjectName("settingsChangeDir");
    changeDirBtn_->setCursor(Qt::PointingHandCursor);
    changeDirBtn_->setFocusPolicy(Qt::NoFocus);
    changeDirBtn_->setStyleSheet(kSecondaryBtnQss);

    openDirBtn_ = new QPushButton(QStringLiteral("打开目录"), card);
    openDirBtn_->setObjectName("settingsOpenDir");
    openDirBtn_->setCursor(Qt::PointingHandCursor);
    openDirBtn_->setFocusPolicy(Qt::NoFocus);
    openDirBtn_->setStyleSheet(kSecondaryBtnQss);

    btnRow->addWidget(changeDirBtn_);
    btnRow->addWidget(openDirBtn_);
    btnRow->addStretch();
    layout->addLayout(btnRow);

    connect(changeDirBtn_, &QPushButton::clicked, this, &SettingsView::onChangeDirClicked);
    connect(openDirBtn_, &QPushButton::clicked, this, &SettingsView::onOpenDirClicked);

    return card;
}

QFrame* SettingsView::buildDictManagementCard()
{
    QFrame *card = new QFrame(this);
    card->setObjectName("dictMgmtCard");
    card->setStyleSheet(
        "QFrame#dictMgmtCard { background:#FFFFFF; border:1px solid #DCE3EF; border-radius:12px; }");
    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(Spacing::XL, Spacing::XL, Spacing::XL, Spacing::XL);
    layout->setSpacing(Spacing::LG);

    QLabel *title = new QLabel(QStringLiteral("来源、标签与办理人管理"), card);
    title->setStyleSheet(kCardTitleQss);
    layout->addWidget(title);

    // 三列：来源管理 + 标签管理 + 办理人管理
    QHBoxLayout *cols = new QHBoxLayout();
    cols->setSpacing(Spacing::XL);

    // ---- 来源管理列 ----
    QFrame *sourceCol = new QFrame(card);
    sourceCol->setStyleSheet("border:none;");
    QVBoxLayout *sourceLayout = new QVBoxLayout(sourceCol);
    sourceLayout->setContentsMargins(0, 0, 0, 0);
    sourceLayout->setSpacing(Spacing::SM);

    QLabel *sourceTitle = new QLabel(QStringLiteral("来源管理"), sourceCol);
    sourceTitle->setStyleSheet("font-size:15px; font-weight:600; color:#0F172A; border:none;");
    sourceLayout->addWidget(sourceTitle);

    sourceList_ = new QListWidget(sourceCol);
    sourceList_->setObjectName("settingsSourceList");
    sourceList_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    sourceList_->setStyleSheet(
        "QListWidget { background:#F8FAFC; border:1px solid #E2E8F0; border-radius:8px; }"
        "QListWidget::item { border-bottom:1px solid #E2E8F0; }");
    sourceList_->setMinimumHeight(200);
    sourceList_->setFocusPolicy(Qt::NoFocus);
    sourceList_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    sourceLayout->addWidget(sourceList_);

    QHBoxLayout *sourceInputRow = new QHBoxLayout();
    sourceInputRow->setSpacing(Spacing::SM);
    sourceInput_ = new QLineEdit(sourceCol);
    sourceInput_->setObjectName("settingsSourceInput");
    sourceInput_->setPlaceholderText(QStringLiteral("输入来源名称"));
    sourceInput_->setStyleSheet(kLineEditQss);
    QPushButton *addSourceBtn = new QPushButton(QStringLiteral("添加来源"), sourceCol);
    addSourceBtn->setObjectName("settingsAddSource");
    addSourceBtn->setCursor(Qt::PointingHandCursor);
    addSourceBtn->setFocusPolicy(Qt::NoFocus);
    addSourceBtn->setStyleSheet(kPrimaryBtnQss);
    sourceInputRow->addWidget(sourceInput_, 1);
    sourceInputRow->addWidget(addSourceBtn);
    sourceLayout->addLayout(sourceInputRow);

    connect(addSourceBtn, &QPushButton::clicked, this, &SettingsView::onAddSource);

    // ---- 标签管理列 ----
    QFrame *tagCol = new QFrame(card);
    tagCol->setStyleSheet("border:none;");
    QVBoxLayout *tagLayout = new QVBoxLayout(tagCol);
    tagLayout->setContentsMargins(0, 0, 0, 0);
    tagLayout->setSpacing(Spacing::SM);

    QLabel *tagTitle = new QLabel(QStringLiteral("标签管理"), tagCol);
    tagTitle->setStyleSheet("font-size:15px; font-weight:600; color:#0F172A; border:none;");
    tagLayout->addWidget(tagTitle);

    tagList_ = new QListWidget(tagCol);
    tagList_->setObjectName("settingsTagList");
    tagList_->setStyleSheet(
        "QListWidget { background:#F8FAFC; border:1px solid #E2E8F0; border-radius:8px; }"
        "QListWidget::item { border-bottom:1px solid #E2E8F0; }");
    tagList_->setMinimumHeight(200);
    tagList_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    tagList_->setFocusPolicy(Qt::NoFocus);
    tagList_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    tagLayout->addWidget(tagList_);

    QHBoxLayout *tagInputRow = new QHBoxLayout();
    tagInputRow->setSpacing(Spacing::SM);
    tagInput_ = new QLineEdit(tagCol);
    tagInput_->setObjectName("settingsTagInput");
    tagInput_->setPlaceholderText(QStringLiteral("输入标签名称"));
    tagInput_->setStyleSheet(kLineEditQss);
    QPushButton *addTagBtn = new QPushButton(QStringLiteral("添加标签"), tagCol);
    addTagBtn->setObjectName("settingsAddTag");
    addTagBtn->setCursor(Qt::PointingHandCursor);
    addTagBtn->setFocusPolicy(Qt::NoFocus);
    addTagBtn->setStyleSheet(kPrimaryBtnQss);
    tagInputRow->addWidget(tagInput_, 1);
    tagInputRow->addWidget(addTagBtn);
    tagLayout->addLayout(tagInputRow);

    connect(addTagBtn, &QPushButton::clicked, this, &SettingsView::onAddTag);

    // ---- 办理人管理列 ----
    QFrame *handlerCol = new QFrame(card);
    handlerCol->setStyleSheet("border:none;");
    QVBoxLayout *handlerLayout = new QVBoxLayout(handlerCol);
    handlerLayout->setContentsMargins(0, 0, 0, 0);
    handlerLayout->setSpacing(Spacing::SM);

    QLabel *handlerTitle = new QLabel(QStringLiteral("办理人管理"), handlerCol);
    handlerTitle->setStyleSheet("font-size:15px; font-weight:600; color:#0F172A; border:none;");
    handlerLayout->addWidget(handlerTitle);

    handlerList_ = new QListWidget(handlerCol);
    handlerList_->setObjectName("settingsHandlerList");
    handlerList_->setStyleSheet(
        "QListWidget { background:#F8FAFC; border:1px solid #E2E8F0; border-radius:8px; }"
        "QListWidget::item { border-bottom:1px solid #E2E8F0; }");
    handlerList_->setMinimumHeight(200);
    handlerList_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    handlerList_->setFocusPolicy(Qt::NoFocus);
    handlerList_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    handlerLayout->addWidget(handlerList_);

    QHBoxLayout *handlerInputRow = new QHBoxLayout();
    handlerInputRow->setSpacing(Spacing::SM);
    handlerInput_ = new QLineEdit(handlerCol);
    handlerInput_->setObjectName("settingsHandlerInput");
    handlerInput_->setPlaceholderText(QStringLiteral("输入办理人名称"));
    handlerInput_->setStyleSheet(kLineEditQss);
    QPushButton *addHandlerBtn = new QPushButton(QStringLiteral("添加办理人"), handlerCol);
    addHandlerBtn->setObjectName("settingsAddHandler");
    addHandlerBtn->setCursor(Qt::PointingHandCursor);
    addHandlerBtn->setFocusPolicy(Qt::NoFocus);
    addHandlerBtn->setStyleSheet(kPrimaryBtnQss);
    handlerInputRow->addWidget(handlerInput_, 1);
    handlerInputRow->addWidget(addHandlerBtn);
    handlerLayout->addLayout(handlerInputRow);

    connect(addHandlerBtn, &QPushButton::clicked, this, &SettingsView::onAddHandler);

    cols->addWidget(sourceCol, 1);
    cols->addWidget(tagCol, 1);
    cols->addWidget(handlerCol, 1);
    layout->addLayout(cols);

    return card;
}

QFrame* SettingsView::buildExportCard()
{
    QFrame *card = new QFrame(this);
    card->setObjectName("exportCard");
    card->setStyleSheet(
        "QFrame#exportCard { background:#FFFFFF; border:1px solid #DCE3EF; border-radius:12px; }");
    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(Spacing::XL, Spacing::XL, Spacing::XL, Spacing::XL);
    layout->setSpacing(Spacing::XL);

    QLabel *title = new QLabel(QStringLiteral("导出设置"), card);
    title->setStyleSheet(kCardTitleQss);
    layout->addWidget(title);

    // ---- 报告标题 ----
    QHBoxLayout *titleRow = new QHBoxLayout();
    titleRow->setSpacing(Spacing::SM);
    QLabel *reportTitleLabel = new QLabel(QStringLiteral("报告标题"), card);
    reportTitleLabel->setFixedWidth(90);
    reportTitleLabel->setStyleSheet("font-size:14px; color:#0F172A; border:none;");
    reportTitleEdit_ = new QLineEdit(card);
    reportTitleEdit_->setObjectName("settingsReportTitle");
    reportTitleEdit_->setText(QStringLiteral("年度工作总结"));
    reportTitleEdit_->setStyleSheet(kLineEditQss);
    titleRow->addWidget(reportTitleLabel);
    titleRow->addWidget(reportTitleEdit_, 1);
    layout->addLayout(titleRow);

    // ---- 复选框组 ----
    includeAchievementsCheck_ = new QCheckBox(QStringLiteral("包含成果列表"), card);
    includeAchievementsCheck_->setObjectName("settingsIncludeAchievements");
    includeCategoryStatsCheck_ = new QCheckBox(QStringLiteral("包含分类统计"), card);
    includeSourceStatsCheck_ = new QCheckBox(QStringLiteral("包含来源统计"), card);
    includeAmountSummaryCheck_ = new QCheckBox(QStringLiteral("包含金额汇总"), card);
    includeKeyAchievementsCheck_ = new QCheckBox(QStringLiteral("包含重点成果"), card);

    for (QCheckBox *cb : {includeAchievementsCheck_, includeCategoryStatsCheck_,
                          includeSourceStatsCheck_, includeAmountSummaryCheck_,
                          includeKeyAchievementsCheck_}) {
        cb->setStyleSheet(kCheckQss);
        cb->setMinimumHeight(25);
        layout->addWidget(cb);
    }
    includeAchievementsCheck_->setChecked(true);  // 默认勾选

    // ---- 下拉框组 ----
    // 排序方式
    QHBoxLayout *sortRow = new QHBoxLayout();
    sortRow->setSpacing(Spacing::SM);
    QLabel *sortLabel = new QLabel(QStringLiteral("排序方式"), card);
    sortLabel->setFixedWidth(90);
    sortLabel->setStyleSheet("font-size:14px; color:#0F172A; border:none;");
    sortByCombo_ = new QComboBox(card);
    sortByCombo_->setObjectName("settingsSortBy");
    sortByCombo_->setStyleSheet(kComboQss);
    sortByCombo_->setMinimumHeight(25);
    sortByCombo_->addItem(QStringLiteral("创建时间"), QStringLiteral("create_time"));
    sortByCombo_->addItem(QStringLiteral("分类"), QStringLiteral("category"));
    sortByCombo_->addItem(QStringLiteral("来源"), QStringLiteral("source"));
    sortRow->addWidget(sortLabel);
    sortRow->addWidget(sortByCombo_);
    sortRow->addStretch();
    layout->addLayout(sortRow);

    // 日期格式
    QHBoxLayout *dateRow = new QHBoxLayout();
    dateRow->setSpacing(Spacing::SM);
    QLabel *dateLabel = new QLabel(QStringLiteral("日期格式"), card);
    dateLabel->setFixedWidth(90);
    dateLabel->setStyleSheet("font-size:14px; color:#0F172A; border:none;");
    dateFormatCombo_ = new QComboBox(card);
    dateFormatCombo_->setObjectName("settingsDateFormat");
    dateFormatCombo_->setStyleSheet(kComboQss);
    dateFormatCombo_->setMinimumHeight(25);
    dateFormatCombo_->addItem(QStringLiteral("yyyy-MM-dd"), QStringLiteral("yyyy-MM-dd"));
    dateFormatCombo_->addItem(QStringLiteral("yyyy年MM月dd日"), QStringLiteral("yyyy年MM月dd日"));
    dateRow->addWidget(dateLabel);
    dateRow->addWidget(dateFormatCombo_);
    dateRow->addStretch();
    layout->addLayout(dateRow);

    // 金额单位
    QHBoxLayout *unitRow = new QHBoxLayout();
    unitRow->setSpacing(Spacing::SM);
    QLabel *unitLabel = new QLabel(QStringLiteral("金额单位"), card);
    unitLabel->setFixedWidth(90);
    unitLabel->setStyleSheet("font-size:14px; color:#0F172A; border:none;");
    amountUnitCombo_ = new QComboBox(card);
    amountUnitCombo_->setObjectName("settingsAmountUnit");
    amountUnitCombo_->setStyleSheet(kComboQss);
    amountUnitCombo_->setMinimumHeight(25);
    amountUnitCombo_->addItem(QStringLiteral("元"), QStringLiteral("yuan"));
    amountUnitCombo_->addItem(QStringLiteral("万元"), QStringLiteral("wanyuan"));
    unitRow->addWidget(unitLabel);
    unitRow->addWidget(amountUnitCombo_);
    unitRow->addStretch();
    layout->addLayout(unitRow);

    // ---- 状态提示（保存后显示）----
    statusLabel_ = new QLabel(card);
    statusLabel_->setObjectName("settingsStatus");
    statusLabel_->setStyleSheet("font-size:13px; color:#16A34A; border:none;");
    statusLabel_->hide();
    layout->addWidget(statusLabel_);

    // ---- 按钮行 ----
    QHBoxLayout *btnRow = new QHBoxLayout();
    btnRow->setSpacing(Spacing::SM);

    saveBtn_ = new QPushButton(QStringLiteral("保存设置"), card);
    saveBtn_->setObjectName("settingsSave");
    saveBtn_->setCursor(Qt::PointingHandCursor);
    saveBtn_->setFocusPolicy(Qt::NoFocus);
    saveBtn_->setStyleSheet(kPrimaryBtnQss);

    resetBtn_ = new QPushButton(QStringLiteral("恢复默认"), card);
    resetBtn_->setObjectName("settingsResetDefaults");
    resetBtn_->setCursor(Qt::PointingHandCursor);
    resetBtn_->setFocusPolicy(Qt::NoFocus);
    resetBtn_->setStyleSheet(kSecondaryBtnQss);

    btnRow->addWidget(saveBtn_);
    btnRow->addWidget(resetBtn_);
    btnRow->addStretch();
    layout->addLayout(btnRow);

    connect(saveBtn_, &QPushButton::clicked, this, &SettingsView::onSaveClicked);
    connect(resetBtn_, &QPushButton::clicked, this, &SettingsView::onResetDefaultsClicked);

    return card;
}

QFrame* SettingsView::buildDataMgmtCard()
{
    QFrame *card = new QFrame(this);
    card->setObjectName("dataMgmtCard");
    card->setStyleSheet(
        "QFrame#dataMgmtCard { background:#FFFFFF; border:1px solid #DCE3EF; border-radius:12px; }");
    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(Spacing::XL, Spacing::XL, Spacing::XL, Spacing::XL);
    layout->setSpacing(Spacing::LG);

    QLabel *title = new QLabel(QStringLiteral("数据管理"), card);
    title->setStyleSheet(kCardTitleQss);
    layout->addWidget(title);

    // 最近备份时间（只读）
    lastBackupLabel_ = new QLabel(QStringLiteral("最近备份：暂无"), card);
    lastBackupLabel_->setObjectName("settingsLastBackup");
    lastBackupLabel_->setStyleSheet("font-size:14px; color:#0F172A; border:none;");
    layout->addWidget(lastBackupLabel_);

    // 按钮行
    QHBoxLayout *btnRow = new QHBoxLayout();
    btnRow->setSpacing(Spacing::SM);

    backupBtn_ = new QPushButton(QStringLiteral("立即备份"), card);
    backupBtn_->setObjectName("settingsBackup");
    backupBtn_->setCursor(Qt::PointingHandCursor);
    backupBtn_->setFocusPolicy(Qt::NoFocus);
    backupBtn_->setStyleSheet(kSecondaryBtnQss);

    restoreBtn_ = new QPushButton(QStringLiteral("恢复备份"), card);
    restoreBtn_->setObjectName("settingsRestore");
    restoreBtn_->setCursor(Qt::PointingHandCursor);
    restoreBtn_->setFocusPolicy(Qt::NoFocus);
    restoreBtn_->setStyleSheet(kSecondaryBtnQss);

    scanImportBtn_ = new QPushButton(QStringLiteral("扫描导入"), card);
    scanImportBtn_->setObjectName("settingsScanImport");
    scanImportBtn_->setCursor(Qt::PointingHandCursor);
    scanImportBtn_->setFocusPolicy(Qt::NoFocus);
    scanImportBtn_->setStyleSheet(kSecondaryBtnQss);

    btnRow->addWidget(backupBtn_);
    btnRow->addWidget(restoreBtn_);
    btnRow->addWidget(scanImportBtn_);
    btnRow->addStretch();
    layout->addLayout(btnRow);

    // ---- 分隔线 ----
    QFrame *separator = new QFrame(card);
    separator->setFrameShape(QFrame::HLine);
    separator->setStyleSheet("color:#F1F5F9; background:#F1F5F9; max-height:1px; border:none;");
    layout->addWidget(separator);

    // ---- 软件初始化 ----
    QLabel *initTitle = new QLabel(QStringLiteral("软件初始化"), card);
    initTitle->setStyleSheet("font-size:15px; font-weight:700; color:#DC2626; border:none;");
    layout->addWidget(initTitle);

    QLabel *initDesc = new QLabel(
        QStringLiteral("初始化将清空所有任务、资料、成果等数据并恢复出厂设置，此操作不可撤销。"),
        card);
    initDesc->setWordWrap(true);
    initDesc->setStyleSheet("font-size:14px; color:#0F172A; border:none;");
    layout->addWidget(initDesc);

    QPushButton *initBtn = new QPushButton(QStringLiteral("软件初始化"), card);
    initBtn->setObjectName("settingsInitialize");
    initBtn->setCursor(Qt::PointingHandCursor);
    initBtn->setFocusPolicy(Qt::NoFocus);
    initBtn->setStyleSheet(
        "QPushButton {"
        "  background:#DC2626;"
        "  color:#FFFFFF;"
        "  border:1px solid #DC2626;"
        "  border-radius:6px;"
        "  padding:8px 20px;"
        "  font-size:14px;"
        "  font-weight:700;"
        "}"
        "QPushButton:hover {"
        "  background:#B91C1C;"
        "  border-color:#B91C1C;"
        "}");
    connect(initBtn, &QPushButton::clicked, this, &SettingsView::onInitClicked);
    layout->addWidget(initBtn);

    connect(backupBtn_, &QPushButton::clicked, this, &SettingsView::onBackupClicked);
    connect(restoreBtn_, &QPushButton::clicked, this, &SettingsView::onRestoreClicked);
    connect(scanImportBtn_, &QPushButton::clicked, this, &SettingsView::onScanImportClicked);

    return card;
}

QFrame* SettingsView::buildFontCard()
{
    auto *card = new QFrame(this);
    card->setObjectName(QStringLiteral("fontSettingsCard"));
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(Spacing::XL, Spacing::XL, Spacing::XL, Spacing::XL);
    layout->setSpacing(Spacing::MD);

    auto *title = new QLabel(QStringLiteral("字体设置"), card);
    title->setStyleSheet(kCardTitleQss);
    layout->addWidget(title);

    auto *sizeRow = new QHBoxLayout();
    auto *sizeLabel = new QLabel(QStringLiteral("字号"), card);
    sizeLabel->setFixedWidth(90);
    fontSizeCombo_ = new QComboBox(card);
    fontSizeCombo_->setObjectName(QStringLiteral("settingsFontSize"));
    fontSizeCombo_->addItem(QStringLiteral("较小 · 13 px"), 13);
    fontSizeCombo_->addItem(QStringLiteral("标准 · 14 px"), 14);
    fontSizeCombo_->addItem(QStringLiteral("较大 · 15 px"), 15);
    fontSizeCombo_->setCurrentIndex(1);
    sizeRow->addWidget(sizeLabel);
    sizeRow->addWidget(fontSizeCombo_);
    sizeRow->addStretch();
    layout->addLayout(sizeRow);

    auto *weightLabel = new QLabel(QStringLiteral("字重预览"), card);
    layout->addWidget(weightLabel);
    auto *weightRow = new QHBoxLayout();
    weightRow->setSpacing(Spacing::SM);
    fontWeightGroup_ = new QButtonGroup(this);
    fontWeightGroup_->setExclusive(true);
    const QList<QPair<QString, QPair<QString, UiFontWeight>>> weights = {
        {QStringLiteral("常规"), {QStringLiteral("Normal"), UiFontWeight::Regular}},
        {QStringLiteral("中等"), {QStringLiteral("Medium"), UiFontWeight::Medium}},
        {QStringLiteral("半粗"), {QStringLiteral("Semibold"), UiFontWeight::Semibold}}
    };
    const QStringList objectNames = {
        QStringLiteral("settingsFontWeightRegular"),
        QStringLiteral("settingsFontWeightMedium"),
        QStringLiteral("settingsFontWeightSemibold")
    };
    for (int index = 0; index < weights.size(); ++index) {
        auto *option = new QFrame(card);
        option->setObjectName(QStringLiteral("fontWeightOption"));
        auto *optionLayout = new QVBoxLayout(option);
        optionLayout->setContentsMargins(Spacing::SM, Spacing::SM, Spacing::SM, Spacing::SM);
        auto *radio = new QRadioButton(weights.at(index).first, option);
        radio->setObjectName(objectNames.at(index));
        fontWeightGroup_->addButton(radio, static_cast<int>(weights.at(index).second.second));
        auto *preview = new QLabel(QStringLiteral("NIUMA 中文预览 123"), option);
        preview->setObjectName(QStringLiteral("fontWeightSample"));
        QFont sample = QFontDatabase().font(QStringLiteral("Segoe UI"), weights.at(index).second.first, 14);
        if (sample.family().isEmpty()) {
            sample = QFont(QStringLiteral("Segoe UI"), 14);
            sample.setWeight(index == 0 ? QFont::Normal : index == 1 ? QFont::Medium : QFont::DemiBold);
        }
        preview->setFont(sample);
        preview->setAlignment(Qt::AlignCenter);
        optionLayout->addWidget(radio);
        optionLayout->addWidget(preview);
        weightRow->addWidget(option, 1);
    }
    layout->addLayout(weightRow);
    connect(fontSizeCombo_, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &SettingsView::onFontPreferencesChanged);
    connect(fontWeightGroup_, qOverload<QAbstractButton *, bool>(&QButtonGroup::buttonToggled),
            this, [this](QAbstractButton *, bool checked) {
                if (checked) onFontPreferencesChanged();
            });
    return card;
}

void SettingsView::onFontPreferencesChanged()
{
    const int pixelSize = fontSizeCombo_->currentData().toInt();
    const int weight = fontWeightGroup_->checkedId();
    for (QLabel *sample : findChildren<QLabel *>(QStringLiteral("fontWeightSample"))) {
        QFont font = sample->font();
        font.setPixelSize(pixelSize);
        sample->setFont(font);
    }
    const UiFontPreferences preferences{pixelSize, static_cast<UiFontWeight>(weight)};
    if (weight < 0 || !SettingsUseCase().saveUiFontPreferences(preferences)) {
        showStatus(QStringLiteral("字体设置保存失败"));
        return;
    }
    emit fontPreferencesChanged(pixelSize, weight);
}

// ============================================================================
// 来源与标签管理
// ============================================================================

void SettingsView::loadSourceList()
{
    sourceList_->clear();
    for (const SourceDict &source : SettingsUseCase().sources()) {
        const int id = source.id;
        const QString &name = source.name;

        QListWidgetItem *item = new QListWidgetItem(sourceList_);
        item->setData(Qt::UserRole, id);

        QWidget *row = new QWidget(sourceList_);
        QHBoxLayout *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(12, 6, 12, 6);
        rowLayout->setSpacing(Spacing::SM);

        QLabel *nameLbl = new QLabel(name, row);
        nameLbl->setStyleSheet("font-size:14px; color:#0F172A; border:none; background:transparent;");
        rowLayout->addWidget(nameLbl, 1);

        QPushButton *delBtn = new QPushButton(QStringLiteral("删除"), row);
        delBtn->setObjectName(QStringLiteral("settingsDeleteSource_%1").arg(id));
        delBtn->setCursor(Qt::PointingHandCursor);
        delBtn->setFocusPolicy(Qt::NoFocus);
        delBtn->setStyleSheet(kSecondaryBtnQss);
        rowLayout->addWidget(delBtn);

        connect(delBtn, &QPushButton::clicked, this, [this, id]() { onDeleteSource(id); });

        item->setSizeHint(row->sizeHint());
        sourceList_->setItemWidget(item, row);
    }
}

void SettingsView::loadTagList()
{
    tagList_->clear();
    for (const TagDict &tag : SettingsUseCase().tags()) {
        const int id = tag.id;
        const QString &name = tag.name;

        QListWidgetItem *item = new QListWidgetItem(tagList_);
        item->setData(Qt::UserRole, id);

        QWidget *row = new QWidget(tagList_);
        QHBoxLayout *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(12, 6, 12, 6);
        rowLayout->setSpacing(Spacing::SM);

        QLabel *nameLbl = new QLabel(name, row);
        nameLbl->setStyleSheet("font-size:14px; color:#0F172A; border:none; background:transparent;");
        rowLayout->addWidget(nameLbl, 1);

        QPushButton *delBtn = new QPushButton(QStringLiteral("删除"), row);
        delBtn->setObjectName(QStringLiteral("settingsDeleteTag_%1").arg(id));
        delBtn->setCursor(Qt::PointingHandCursor);
        delBtn->setFocusPolicy(Qt::NoFocus);
        delBtn->setStyleSheet(kSecondaryBtnQss);
        rowLayout->addWidget(delBtn);

        connect(delBtn, &QPushButton::clicked, this, [this, id]() { onDeleteTag(id); });

        item->setSizeHint(row->sizeHint());
        tagList_->setItemWidget(item, row);
    }
}

void SettingsView::onAddSource()
{
    QString name = sourceInput_->text().trimmed();
    if (name.isEmpty()) {
        return;
    }
    if (SettingsUseCase().addSource(name) <= 0) {
        Logger::instance().error(QStringLiteral("SettingsView: 添加来源失败: %1").arg(name));
        return;
    }
    sourceInput_->clear();
    loadSourceList();
    Logger::instance().info(QStringLiteral("SettingsView: 已添加来源: %1").arg(name));
}

void SettingsView::onDeleteSource(int dictId)
{
    if (!SettingsUseCase().deleteSource(dictId)) {
        Logger::instance().error(QStringLiteral("SettingsView: 删除来源失败: %1").arg(dictId));
        return;
    }
    loadSourceList();
    Logger::instance().info(QStringLiteral("SettingsView: 已删除来源 id=%1").arg(dictId));
}

void SettingsView::onAddTag()
{
    QString name = tagInput_->text().trimmed();
    if (name.isEmpty()) {
        return;
    }
    if (SettingsUseCase().addTag(name) <= 0) {
        Logger::instance().error(QStringLiteral("SettingsView: 添加标签失败: %1").arg(name));
        return;
    }
    tagInput_->clear();
    loadTagList();
    Logger::instance().info(QStringLiteral("SettingsView: 已添加标签: %1").arg(name));
}

void SettingsView::onDeleteTag(int dictId)
{
    if (!SettingsUseCase().deleteTag(dictId)) {
        Logger::instance().error(QStringLiteral("SettingsView: 删除标签失败: %1").arg(dictId));
        return;
    }
    loadTagList();
    Logger::instance().info(QStringLiteral("SettingsView: 已删除标签 id=%1").arg(dictId));
}

void SettingsView::loadHandlerList()
{
    handlerList_->clear();
    for (const Handler &handler : SettingsUseCase().handlers()) {
        const int id = handler.id;
        const QString &name = handler.name;

        QListWidgetItem *item = new QListWidgetItem(handlerList_);
        item->setData(Qt::UserRole, id);

        QWidget *row = new QWidget(handlerList_);
        QHBoxLayout *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(12, 6, 12, 6);
        rowLayout->setSpacing(Spacing::SM);

        QLabel *nameLbl = new QLabel(name, row);
        nameLbl->setStyleSheet("font-size:14px; color:#0F172A; border:none; background:transparent;");
        rowLayout->addWidget(nameLbl, 1);

        QPushButton *delBtn = new QPushButton(QStringLiteral("删除"), row);
        delBtn->setObjectName(QStringLiteral("settingsDeleteHandler_%1").arg(id));
        delBtn->setCursor(Qt::PointingHandCursor);
        delBtn->setFocusPolicy(Qt::NoFocus);
        delBtn->setStyleSheet(kSecondaryBtnQss);
        rowLayout->addWidget(delBtn);

        connect(delBtn, &QPushButton::clicked, this, [this, id]() { onDeleteHandler(id); });

        item->setSizeHint(row->sizeHint());
        handlerList_->setItemWidget(item, row);
    }
}

void SettingsView::onAddHandler()
{
    QString name = handlerInput_->text().trimmed();
    if (name.isEmpty()) {
        return;
    }
    if (SettingsUseCase().addHandler(name) <= 0) {
        Logger::instance().error(QStringLiteral("SettingsView: 添加办理人失败: %1").arg(name));
        return;
    }
    handlerInput_->clear();
    loadHandlerList();
    Logger::instance().info(QStringLiteral("SettingsView: 已添加办理人: %1").arg(name));
}

void SettingsView::onDeleteHandler(int dictId)
{
    if (!SettingsUseCase().deleteHandler(dictId)) {
        Logger::instance().error(QStringLiteral("SettingsView: 删除办理人失败: %1").arg(dictId));
        return;
    }
    loadHandlerList();
    Logger::instance().info(QStringLiteral("SettingsView: 已删除办理人 id=%1").arg(dictId));
}

// ============================================================================
// 配置加载与保存
// ============================================================================

void SettingsView::loadConfig()
{
    applyConfigToUi(SettingsUseCase().exportConfig());
}

void SettingsView::applyConfigToUi(const ExportConfig &config)
{
    reportTitleEdit_->setText(config.reportTitle);
    includeAchievementsCheck_->setChecked(config.includeAchievements);
    includeCategoryStatsCheck_->setChecked(config.includeCategoryStats);
    includeSourceStatsCheck_->setChecked(config.includeSourceStats);
    includeAmountSummaryCheck_->setChecked(config.includeAmountSummary);
    includeKeyAchievementsCheck_->setChecked(config.includeKeyAchievements);

    sortByCombo_->setCurrentIndex(sortByCombo_->findData(config.sortBy));
    dateFormatCombo_->setCurrentIndex(dateFormatCombo_->findData(config.dateFormat));
    amountUnitCombo_->setCurrentIndex(amountUnitCombo_->findData(config.amountUnit));
}

ExportConfig SettingsView::collectConfigFromUi() const
{
    ExportConfig config;
    config.reportTitle = reportTitleEdit_->text().trimmed();
    if (config.reportTitle.isEmpty()) {
        config.reportTitle = QStringLiteral("年度工作总结");
    }
    config.includeAchievements = includeAchievementsCheck_->isChecked();
    config.includeCategoryStats = includeCategoryStatsCheck_->isChecked();
    config.includeSourceStats = includeSourceStatsCheck_->isChecked();
    config.includeAmountSummary = includeAmountSummaryCheck_->isChecked();
    config.includeKeyAchievements = includeKeyAchievementsCheck_->isChecked();
    config.sortBy = sortByCombo_->currentData().toString();
    config.dateFormat = dateFormatCombo_->currentData().toString();
    config.amountUnit = amountUnitCombo_->currentData().toString();
    return config;
}

bool SettingsView::saveConfig(const ExportConfig &config)
{
    return SettingsUseCase().saveExportConfig(config);
}

void SettingsView::loadWorkDir()
{
    setWorkDir(SettingsUseCase().workDir());
}

void SettingsView::loadLastBackupTime()
{
    const QString time = SettingsUseCase().lastBackupTime();
    if (time.isEmpty()) {
        lastBackupLabel_->setText(QStringLiteral("最近备份：暂无"));
    } else {
        lastBackupLabel_->setText(QStringLiteral("最近备份：%1").arg(time));
    }
}

// ============================================================================
// 公共方法
// ============================================================================

void SettingsView::refresh()
{
    loadConfig();
    loadWorkDir();
    loadLastBackupTime();
    loadSourceList();
    loadTagList();
    loadHandlerList();
}

void SettingsView::setWorkDir(const QString &path)
{
    if (path.isEmpty()) {
        workDirLabel_->setText(QStringLiteral("未设置"));
        workDirLabel_->setStyleSheet(
            "font-size:14px; color:#0F172A; border:none; "
            "background:#F8FAFC; padding:10px 12px; border-radius:8px;");
    } else {
        workDirLabel_->setText(path);
        workDirLabel_->setStyleSheet(
            "font-size:14px; color:#0F172A; border:none; "
            "background:#F8FAFC; padding:10px 12px; border-radius:8px;");
    }
}

// ============================================================================
// 槽函数
// ============================================================================

void SettingsView::onChangeDirClicked()
{
    emit workDirChangeRequested();
}

void SettingsView::onOpenDirClicked()
{
    QString dir = workDirLabel_->text();
    if (dir.isEmpty() || dir == QStringLiteral("未设置")) {
        return;
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
}

void SettingsView::onSaveClicked()
{
    ExportConfig config = collectConfigFromUi();
    if (saveConfig(config)) {
        showStatus(QStringLiteral("✓ 设置已保存"));
        Logger::instance().info(QStringLiteral("SettingsView: 导出设置已保存"));
    } else {
        Logger::instance().error(QStringLiteral("SettingsView: 导出设置保存失败"));
        showStatus(QStringLiteral("保存失败，请重试"));
    }
}

void SettingsView::onResetDefaultsClicked()
{
    ExportConfig config = ExportConfig::defaultValue();
    applyConfigToUi(config);
    if (saveConfig(config)) {
        showStatus(QStringLiteral("✓ 已恢复默认设置"));
        Logger::instance().info(QStringLiteral("SettingsView: 导出设置已恢复默认"));
    }
}

void SettingsView::onBackupClicked()
{
    emit backupRequested();
}

void SettingsView::onRestoreClicked()
{
    emit restoreRequested();
}

void SettingsView::onScanImportClicked()
{
    emit scanImportRequested();
}

void SettingsView::onInitClicked()
{
    // 二次确认
    auto ret = StyledMessageBox::question(this,
        QStringLiteral("软件初始化"),
        QStringLiteral("初始化将清空所有任务、资料、成果、字典库、设置等数据并恢复出厂设置。\n\n此操作不可撤销，是否继续？"),
        QStringLiteral("确认初始化"), QStringLiteral("取消"));
    if (ret != StyledMessageBox::Yes) {
        return;
    }

    SettingsUseCase().resetAllData();

    Logger::instance().info(QStringLiteral("SettingsView: 软件已初始化，所有数据已清空（含字典表、settings、自增ID）"));
    showStatus(QStringLiteral("软件已初始化"));

    // 通知 MainWindow 引导重新设置工作目录
    emit initCompleted();
}

void SettingsView::showStatus(const QString &text)
{
    statusLabel_->setText(text);
    statusLabel_->show();
    QTimer::singleShot(3000, statusLabel_, &QLabel::hide);
}

QFrame* SettingsView::buildAboutCard()
{
    QFrame *card = new QFrame(this);
    card->setObjectName("aboutCard");
    card->setStyleSheet(
        "QFrame#aboutCard { background:#FFFFFF; border:1px solid #DCE3EF; border-radius:12px; }");
    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(Spacing::XL, Spacing::XL, Spacing::XL, Spacing::XL);
    layout->setSpacing(Spacing::SM);

    QLabel *title = new QLabel(QStringLiteral("\u5173\u4E8E"), card);  // 关于
    title->setStyleSheet(kCardTitleQss);
    layout->addWidget(title);

    // 软件名 + 副标题
    QLabel *nameLabel = new QLabel(
        QStringLiteral("\u8F6F\u4EF6\u540D\u79F0\uFF1ANIUMA\uFF08\u94C1\u519B\u725B\u9A6C\u4EFB\u52A1\u65E5\u5FD7\uFF09"),
        card);  // 软件名称：NIUMA（铁军牛马任务日志）
    nameLabel->setStyleSheet("font-size:15px; color:#0F172A; border:none; background:transparent;");
    layout->addWidget(nameLabel);

    QLabel *codeLabel = new QLabel(QStringLiteral("内部代号：铁军贰号"), card);
    codeLabel->setObjectName("settingsInternalCode");
    codeLabel->setStyleSheet("font-size:15px; color:#0F172A; border:none; background:transparent;");
    layout->addWidget(codeLabel);

    // 版本号
    QLabel *versionLabel = new QLabel(QStringLiteral("版本号：V%1").arg(NIUMA_APP_VERSION), card);
    versionLabel->setObjectName("settingsVersion");
    versionLabel->setStyleSheet("font-size:15px; color:#0F172A; border:none; background:transparent;");
    layout->addWidget(versionLabel);

    // 作者信息
    QLabel *authorLabel = new QLabel(
        QStringLiteral("\u4F5C\u8005\u4FE1\u606F\uFF1A\u957F\u767D\u671D\u9C9C\u65CF\u81EA\u6CBB\u53BF\u7A0E\u52A1\u5C40\u516B\u9053\u6C9F\u7A0E\u52A1\u5206\u5C40 \u97E9\u708E\u8D64"),
        card);  // 作者信息：长白朝鲜族自治县税务局八道沟税务分局 韩炎赤
    authorLabel->setStyleSheet("font-size:15px; color:#0F172A; border:none; background:transparent;");
    layout->addWidget(authorLabel);

    return card;
}
