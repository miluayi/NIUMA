/**
 * @file AchievementDialog.cpp
 * @brief 成果填写弹窗实现（T4.2）
 *
 * 实现：setupUI 搭建表单 / validateForm 表单验证 / onSaveClicked 成果入库 /
 *       onSkipClicked 跳过（reject）。金额元→分转换使用 AmountUtil::yuanToCents。
 *
 * 依赖：ui/AchievementDialog.h、ui/ColorPalette.h、ui/Spacing.h、ui/Radius.h、
 *       application/achievement/AchievementUseCase、models/Achievement、
 *       utils/AmountUtil、utils/Logger
 * 文档：task.md T4.2、qt_implementation_guide.md 3.7、coding.md 第十一章
 */

#include "AchievementDialog.h"

#include <QCheckBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QShortcut>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

#include "ColorPalette.h"
#include "Radius.h"
#include "Spacing.h"
#include "application/achievement/AchievementUseCase.h"
#include "models/Achievement.h"
#include "utils/AmountUtil.h"
#include "utils/Logger.h"
#include "widgets/WindowDragger.h"
#include "widgets/RoundedCornerFilter.h"
#include "widgets/StyledMessageBox.h"

// ============================================================================
// 构造
// ============================================================================

AchievementDialog::AchievementDialog(int taskId, const QString &taskCategory,
                                     const QString &taskTitle, QWidget *parent)
    : QDialog(parent)
    , taskId_(taskId)
    , taskCategory_(taskCategory)
    , taskTitle_(taskTitle)
    , guideLabel_(nullptr)
    , workContentEdit_(nullptr)
    , workObjectEdit_(nullptr)
    , resultCountEdit_(nullptr)
    , resultAmountEdit_(nullptr)
    , descEdit_(nullptr)
    , annualCheck_(nullptr)
    , keyCheck_(nullptr)
{
    setupUI();
}

// ============================================================================
// UI 搭建
// ============================================================================

void AchievementDialog::setupUI()
{
    // 窗口属性：无边框、模态、可调整大小
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setModal(true);
    auto *saveShortcut = new QShortcut(QKeySequence::Save, this);
    saveShortcut->setObjectName(QStringLiteral("shortcutSaveAchievement"));
    connect(saveShortcut, &QShortcut::activated, this, &AchievementDialog::onSaveClicked);
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

    QLabel *titleLabel = new QLabel(QStringLiteral("\u586B\u5199\u6210\u679C\u4FE1\u606F"), headerFrame);  // 填写成果信息
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

    connect(closeBtn, &QPushButton::clicked, this, &AchievementDialog::onSkipClicked);

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

    // ---- 分类引导提示（浅蓝底 #EFF6FF，圆角）----
    guideLabel_ = new QLabel(bodyFrame);
    guideLabel_->setWordWrap(true);
    guideLabel_->setText(guideTextForCategory(taskCategory_));
    guideLabel_->setStyleSheet(
        "QLabel {"
        "  background: #EFF6FF;"
        "  color: #1D4ED8;"
        "  border: none;"
        "  border-radius: 8px;"
        "  padding: 10px 14px;"
        "  font-size: 13px;"
        "}");
    bodyLayout->addWidget(guideLabel_);

    // ---- 工作内容 ----
    QLabel *workContentLabel = new QLabel(QStringLiteral("\u5DE5\u4F5C\u5185\u5BB9 *"), bodyFrame);  // 工作内容 *
    bodyLayout->addWidget(workContentLabel);

    workContentEdit_ = new QLineEdit(bodyFrame);
    workContentEdit_->setObjectName("achievementWorkContentEdit");
    workContentEdit_->setText(taskTitle_);  // 默认 = 任务标题
    workContentEdit_->setStyleSheet(
        "QLineEdit {"
        "  padding: 9px 12px;"
        "  border: 1px solid #E2E8F0;"
        "  border-radius: 8px;"
        "  font-size: 14px;"
        "  background: #FFFFFF;"
        "  color: #0F172A;"
        "}"
        "QLineEdit:focus {"
        "  border: 2px solid #2463EB;"
        "  padding: 8px 11px;"
        "}");
    bodyLayout->addWidget(workContentEdit_);

    // ---- 工作对象 ----
    QLabel *workObjectLabel = new QLabel(QStringLiteral("\u5DE5\u4F5C\u5BF9\u8C61"), bodyFrame);  // 工作对象
    bodyLayout->addWidget(workObjectLabel);

    workObjectEdit_ = new QLineEdit(bodyFrame);
    workObjectEdit_->setObjectName("achievementWorkObjectEdit");
    workObjectEdit_->setPlaceholderText(
        QStringLiteral("\u5982\uFF1A12\u6237\u4F01\u4E1A\u3001\u8F96\u533A\u91CD\u70B9\u7A0E\u6E90"));  // 如：12户企业、辖区重点税源
    workObjectEdit_->setStyleSheet(workContentEdit_->styleSheet());
    bodyLayout->addWidget(workObjectEdit_);

    // ---- 成果数量 + 成果金额（元）并排 ----
    QHBoxLayout *countRowLayout = new QHBoxLayout();
    countRowLayout->setSpacing(Spacing::MD);

    QVBoxLayout *countLayout = new QVBoxLayout();
    countLayout->setSpacing(Spacing::XS);
    QLabel *countLabel = new QLabel(QStringLiteral("\u6210\u679C\u6570\u91CF"), bodyFrame);  // 成果数量
    countLabel->setStyleSheet("color: #475569; font-size: 13px; border: none;");
    resultCountEdit_ = new QLineEdit(bodyFrame);
    resultCountEdit_->setObjectName("achievementResultCountEdit");
    resultCountEdit_->setPlaceholderText(QStringLiteral("\u5982\uFF1A3"));  // 如：3
    resultCountEdit_->setStyleSheet(workContentEdit_->styleSheet());
    countLayout->addWidget(countLabel);
    countLayout->addWidget(resultCountEdit_);
    countRowLayout->addLayout(countLayout);

    QVBoxLayout *amountLayout = new QVBoxLayout();
    amountLayout->setSpacing(Spacing::XS);
    QLabel *amountLabel = new QLabel(QStringLiteral("\u6210\u679C\u91D1\u989D\uFF08\u5143\uFF09"), bodyFrame);  // 成果金额（元）
    amountLabel->setStyleSheet("color: #475569; font-size: 13px; border: none;");
    resultAmountEdit_ = new QLineEdit(bodyFrame);
    resultAmountEdit_->setObjectName("achievementResultAmountEdit");
    resultAmountEdit_->setPlaceholderText(QStringLiteral("0.00"));
    resultAmountEdit_->setStyleSheet(workContentEdit_->styleSheet());
    amountLayout->addWidget(amountLabel);
    amountLayout->addWidget(resultAmountEdit_);
    countRowLayout->addLayout(amountLayout);

    bodyLayout->addLayout(countRowLayout);

    // ---- 成果说明 ----
    QLabel *descLabel = new QLabel(QStringLiteral("\u6210\u679C\u8BF4\u660E"), bodyFrame);  // 成果说明
    bodyLayout->addWidget(descLabel);

    descEdit_ = new QTextEdit(bodyFrame);
    descEdit_->setObjectName("achievementDescriptionEdit");
    descEdit_->setPlaceholderText(descPlaceholderForCategory(taskCategory_));
    descEdit_->setFixedHeight(72);  // 约 3 行高度
    descEdit_->setStyleSheet(
        "QTextEdit {"
        "  padding: 9px 12px;"
        "  border: 1px solid #E2E8F0;"
        "  border-radius: 8px;"
        "  font-size: 14px;"
        "  background: #FFFFFF;"
        "  color: #0F172A;"
        "}"
        "QTextEdit:focus {"
        "  border: 2px solid #2463EB;"
        "  padding: 8px 11px;"
        "}");
    bodyLayout->addWidget(descEdit_);

    // ---- 纳入年度成果 + 重点成果（并排复选框）----
    QHBoxLayout *checkRowLayout = new QHBoxLayout();
    checkRowLayout->setSpacing(Spacing::XL);

    annualCheck_ = new QCheckBox(QStringLiteral("\u7EB3\u5165\u5E74\u5EA6\u6210\u679C"), bodyFrame);  // 纳入年度成果
    annualCheck_->setObjectName("achievementAnnualCheck");
    annualCheck_->setStyleSheet(
        "QCheckBox {"
        "  color: #0F172A;"
        "  font-size: 14px;"
        "  spacing: 8px;"
        "}"
        "QCheckBox::indicator {"
        "  width: 16px;"
        "  height: 16px;"
        "  border: 1px solid #CBD5E1;"
        "  border-radius: 4px;"
        "  background: #FFFFFF;"
        "}"
        "QCheckBox::indicator:checked {"
        "  background: #2463EB;"
        "  border: 1px solid #2463EB;"
        "}");

    keyCheck_ = new QCheckBox(QStringLiteral("\u91CD\u70B9\u6210\u679C"), bodyFrame);  // 重点成果
    keyCheck_->setObjectName("achievementKeyCheck");
    keyCheck_->setStyleSheet(annualCheck_->styleSheet());

    checkRowLayout->addWidget(annualCheck_);
    checkRowLayout->addWidget(keyCheck_);
    checkRowLayout->addStretch();

    bodyLayout->addLayout(checkRowLayout);

    mainLayout->addWidget(bodyFrame);

    // ---- 底部按钮栏 ----
    QFrame *footerFrame = new QFrame(this);
    footerFrame->setObjectName("dialogFooter");
    footerFrame->setStyleSheet(
        "#dialogFooter { background: #FFFFFF; border-top: 1px solid #E2E8F0; }");
    QHBoxLayout *footerLayout = new QHBoxLayout(footerFrame);
    footerLayout->setContentsMargins(Spacing::XXL, Spacing::LG, Spacing::XXL, Spacing::LG);
    footerLayout->setSpacing(Spacing::MD);

    // "跳过" 按钮（ghost/secondary，左）
    QPushButton *skipBtn = new QPushButton(QStringLiteral("\u8DF3\u8FC7"), footerFrame);  // 跳过
    skipBtn->setCursor(Qt::PointingHandCursor);
    skipBtn->setFocusPolicy(Qt::NoFocus);
    skipBtn->setMinimumWidth(96);
    skipBtn->setStyleSheet(
        "QPushButton {"
        "  background: #FFFFFF;"
        "  color: #475569;"
        "  border: 1px solid #E2E8F0;"
        "  border-radius: 8px;"
        "  padding: 9px 18px;"
        "  font-size: 14px;"
        "}"
        "QPushButton:hover {"
        "  background: #F5F7FA;"
        "  border-color: #CBD5E1;"
        "}");

    // "保存并完成" 按钮（primary，右）
    QPushButton *saveBtn = new QPushButton(QStringLiteral("\u4FDD\u5B58\u5E76\u5B8C\u6210"), footerFrame);  // 保存并完成
    saveBtn->setObjectName("btnPrimary");
    saveBtn->setCursor(Qt::PointingHandCursor);
    saveBtn->setFocusPolicy(Qt::NoFocus);
    saveBtn->setMinimumWidth(120);
    saveBtn->setDefault(true);
    saveBtn->setStyleSheet(
        "QPushButton#btnPrimary {"
        "  background: #2463EB;"
        "  color: #FFFFFF;"
        "  border: 1px solid #2463EB;"
        "  border-radius: 8px;"
        "  padding: 9px 18px;"
        "  font-size: 14px;"
        "  font-weight: 700;"
        "}"
        "QPushButton#btnPrimary:hover {"
        "  background: #1D4ED8;"
        "}");

    footerLayout->addWidget(skipBtn);
    footerLayout->addStretch();
    footerLayout->addWidget(saveBtn);

    connect(skipBtn, &QPushButton::clicked, this, &AchievementDialog::onSkipClicked);
    connect(saveBtn, &QPushButton::clicked, this, &AchievementDialog::onSaveClicked);

    mainLayout->addWidget(footerFrame);
}

// ============================================================================
// 分类引导文案
// ============================================================================

QString AchievementDialog::guideTextForCategory(const QString &category) const
{
    if (category == QStringLiteral("\u50AC")) {  // 催
        return QStringLiteral(
            "\u3010\u5206\u7C7B\u5F15\u5BFC\u3011\u672C\u6B21\u50AC\u529E\u4E86\u54EA\u4E9B\u4E8B\u9879\uFF1F"
            "\u6D89\u53CA\u591A\u5C11\u6237\uFF1F\u50AC\u6536\u91D1\u989D\u591A\u5C11\uFF1F");  // 【分类引导】本次催办了哪些事项？涉及多少户？催收金额多少？
    }
    if (category == QStringLiteral("\u67E5")) {  // 查
        return QStringLiteral(
            "\u3010\u5206\u7C7B\u5F15\u5BFC\u3011\u672C\u6B21\u6838\u67E5\u4E86\u54EA\u4E9B\u5185\u5BB9\uFF1F"
            "\u53D1\u73B0\u591A\u5C11\u95EE\u9898/\u5F02\u5E38\uFF1F\u6D89\u53CA\u591A\u5C11\u6237\u4F01\u4E1A\uFF1F");  // 【分类引导】本次核查了哪些内容？发现多少问题/异常？涉及多少户企业？
    }
    if (category == QStringLiteral("\u62A5")) {  // 报
        return QStringLiteral(
            "\u3010\u5206\u7C7B\u5F15\u5BFC\u3011\u672C\u6B21\u62A5\u9001\u4E86\u4EC0\u4E48\u62A5\u544A\uFF1F"
            "\u62A5\u9001\u5BF9\u8C61\uFF1F\u6D89\u53CA\u6570\u636E\u91CF\uFF1F");  // 【分类引导】本次报送了什么报告？报送对象？涉及数据量？
    }
    if (category == QStringLiteral("\u529E")) {  // 办
        return QStringLiteral(
            "\u3010\u5206\u7C7B\u5F15\u5BFC\u3011\u672C\u6B21\u529E\u7406\u4E86\u4EC0\u4E48\u4E8B\u9879\uFF1F"
            "\u529E\u7406\u6570\u91CF\uFF1F\u6D89\u53CA\u91D1\u989D\uFF1F");  // 【分类引导】本次办理了什么事项？办理数量？涉及金额？
    }
    // 默认引导文案
    return QStringLiteral(
        "\u3010\u5206\u7C7B\u5F15\u5BFC\u3011\u8BF7\u586B\u5199\u672C\u6B21\u5DE5\u4F5C\u7684\u4E3B\u8981\u5185\u5BB9\u3001"
        "\u6D89\u53CA\u5BF9\u8C61\u4E0E\u6210\u679C\u3002");  // 【分类引导】请填写本次工作的主要内容、涉及对象与成果。
}

QString AchievementDialog::descPlaceholderForCategory(const QString &category) const
{
    if (category == QStringLiteral("\u50AC")) {  // 催
        return QStringLiteral(
            "\u672C\u6B21\u50AC\u529E\u4E86\u8F96\u533A15\u6237\u4F01\u4E1A\u7684\u6B20\u7A0E\u4E8B\u9879\uFF0C"
            "\u6D89\u53CA\u7A0E\u6B3E\u7EA612.5\u4E07\u5143\uFF0C\u5DF2\u7763\u4FC3\u6574\u6539\u3002");  // 本次催办了辖区15户企业的欠税事项，涉及税款约12.5万元，已督促整改。
    }
    if (category == QStringLiteral("\u67E5")) {  // 查
        return QStringLiteral(
            "\u672C\u6B21\u6838\u67E5\u4E86\u8F96\u533A15\u6237\u80A1\u6743\u8F6C\u8BA9\u4F01\u4E1A\uFF0C"
            "\u53D1\u73B03\u6237\u672A\u6309\u89C4\u5B9A\u5B8C\u7A0E\uFF0C\u6D89\u53CA\u7A0E\u6B3E\u7EA612.5\u4E07\u5143\uFF0C\u5DF2\u7763\u4FC3\u6574\u6539\u3002");  // 本次核查了辖区内15户股权转让企业，发现3户未按规定完税，涉及税款约12.5万元，已督促整改。
    }
    if (category == QStringLiteral("\u62A5")) {  // 报
        return QStringLiteral(
            "\u672C\u6B21\u62A5\u9001\u4E86\u6708\u5EA6\u7A0E\u6536\u5206\u6790\u62A5\u544A\uFF0C"
            "\u62A5\u9001\u5BF9\u8C61\u4E3A\u4E0A\u7EA7\u5C40\uFF0C\u6D89\u53CA\u5168\u5C40\u6570\u636E\u3002");  // 本次报送了月度税收分析报告，报送对象为上级局，涉及全局数据。
    }
    if (category == QStringLiteral("\u529E")) {  // 办
        return QStringLiteral(
            "\u672C\u6B21\u529E\u7406\u4E86\u7EB3\u7A0E\u4EBA\u7533\u8BF7\u4E8B\u98793\u4EF6\uFF0C"
            "\u6D89\u53CA\u51CF\u514D\u7A0E\u6B3E\u7EA68\u4E07\u5143\u3002");  // 本次办理了纳税人申请事项3件，涉及减免税款约8万元。
    }
    // 默认占位文案
    return QStringLiteral("\u8BF7\u586B\u5199\u6210\u679C\u8BF4\u660E...");  // 请填写成果说明...
}

// ============================================================================
// 表单验证
// ============================================================================

bool AchievementDialog::validateForm()
{
    // 工作内容必填
    if (workContentEdit_->text().trimmed().isEmpty()) {
        StyledMessageBox::warning(this, QStringLiteral("\u63D0\u793A"),  // 提示
                             QStringLiteral("\u8BF7\u8F93\u5165\u5DE5\u4F5C\u5185\u5BB9"));  // 请输入工作内容
        workContentEdit_->setFocus();
        return false;
    }

    // 成果数量：空串默认 0，否则必须为合法整数
    QString countText = resultCountEdit_->text().trimmed();
    if (!countText.isEmpty()) {
        bool ok = false;
        countText.toInt(&ok);
        if (!ok) {
            StyledMessageBox::warning(this, QStringLiteral("\u63D0\u793A"),
                                 QStringLiteral("\u6210\u679C\u6570\u91CF\u5FC5\u987B\u4E3A\u6574\u6570"));  // 成果数量必须为整数
            resultCountEdit_->setFocus();
            return false;
        }
    }

    // 成果金额：空串默认 0，否则必须为合法小数
    QString amountText = resultAmountEdit_->text().trimmed();
    if (!amountText.isEmpty()) {
        bool ok = false;
        amountText.toDouble(&ok);
        if (!ok) {
            StyledMessageBox::warning(this, QStringLiteral("\u63D0\u793A"),
                                 QStringLiteral("\u6210\u679C\u91D1\u989D\u5FC5\u987B\u4E3A\u5408\u6CD5\u6570\u5B57"));  // 成果金额必须为合法数字
            resultAmountEdit_->setFocus();
            return false;
        }
    }

    return true;
}

// ============================================================================
// 槽：保存 / 跳过
// ============================================================================

void AchievementDialog::onSaveClicked()
{
    if (!validateForm()) {
        return;
    }

    // 构造 Achievement
    Achievement achievement;
    achievement.taskId = taskId_;
    achievement.workContent = workContentEdit_->text().trimmed();
    achievement.workObject = workObjectEdit_->text().trimmed();

    // 成果数量：空串默认 0
    QString countText = resultCountEdit_->text().trimmed();
    if (countText.isEmpty()) {
        achievement.resultCount = 0;
    } else {
        achievement.resultCount = countText.toInt();
    }

    // 成果金额：元 → 分（AmountUtil::yuanToCents）
    achievement.resultAmount = AmountUtil::yuanToCents(resultAmountEdit_->text().trimmed());

    achievement.description = descEdit_->toPlainText();
    achievement.isAnnual = annualCheck_->isChecked();
    achievement.isKeyAchievement = keyCheck_->isChecked();
    achievement.createTime = QDateTime::currentDateTime();

    // 入库
    int newId = AchievementUseCase().create(achievement);
    if (newId <= 0) {
        StyledMessageBox::warning(this, QStringLiteral("\u4FDD\u5B58\u5931\u8D25"),  // 保存失败
                              QStringLiteral("\u6210\u679C\u4FDD\u5B58\u5931\u8D25\uFF0C\u8BF7\u91CD\u8BD5"));  // 成果保存失败，请重试
        return;
    }

    achievement.id = newId;
    result_ = achievement;

    Logger::instance().info(QStringLiteral("AchievementDialog: \u6210\u679C\u5DF2\u4FDD\u5B58, id=%1, task_id=%2")
                                .arg(newId)
                                .arg(taskId_));  // 成果已保存

    QDialog::accept();  // 调用基类，避免递归
}

void AchievementDialog::onSkipClicked()
{
    reject();
}

// ============================================================================
// 事件重写
// ============================================================================

void AchievementDialog::keyPressEvent(QKeyEvent *event)
{
    // Enter / Return → 保存并完成
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        onSaveClicked();
        return;
    }
    // Esc 由 QDialog 默认处理（调用 reject）
    QDialog::keyPressEvent(event);
}

void AchievementDialog::accept()
{
    // 重写 accept：任何外部 accept 调用都走 onSaveClicked 验证流程
    onSaveClicked();
}

// ============================================================================
// 结果获取
// ============================================================================

std::optional<Achievement> AchievementDialog::getAchievement() const
{
    return result_;
}
