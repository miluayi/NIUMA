/**
 * @file AchievementView.cpp
 * @brief 年度成果页面实现（T4.3）
 *
 * 实现：setupUI 搭建页面 / loadData 加载当年成果与统计 / onYearChanged 切换年份 /
 *       onExportClicked 发射 exportRequested 信号。
 * 表格使用 QStandardItemModel + 自定义委托 AchievementTagDelegate 渲染标记列。
 *
 * 依赖：ui/AchievementView.h、ui/ColorPalette.h、ui/Spacing.h、ui/Radius.h、
 *       application/achievement/AchievementUseCase、
 *       ui/widgets/PaginationWidget、utils/AmountUtil、utils/Logger
 * 文档：task.md T4.3、qt_implementation_guide.md 3.4、coding.md 第七章
 */

#include "AchievementView.h"
#include "widgets/RoundedCardBorder.h"

#include <QComboBox>
#include <QDate>
#include <QFont>
#include <QFontMetrics>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMargins>
#include <QPainter>
#include <QPushButton>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>
#include <QTableView>
#include <QVBoxLayout>
#include <QVariant>

#include "ColorPalette.h"
#include "AchievementDialog.h"
#include "Radius.h"
#include "Spacing.h"
#include "application/achievement/AchievementUseCase.h"
#include "ui/widgets/PaginationWidget.h"
#include "utils/AmountUtil.h"
#include "utils/Logger.h"

namespace {

// ============================================================================
// 成果标记列自定义委托：渲染"重点"(橙)/"年度"(蓝)彩色标签
// ============================================================================
class AchievementTagDelegate : public QStyledItemDelegate {
public:
    explicit AchievementTagDelegate(QObject *parent = nullptr)
        : QStyledItemDelegate(parent)
    {
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        // 先绘制默认背景（含选中态）
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);
        // 不让默认委托绘制文本（只绘制背景）
        opt.text.clear();
        QStyle *style = opt.widget ? opt.widget->style() : nullptr;
        if (style) {
            style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);
        }

        // 从 UserRole 获取标签列表（"key" / "annual"）
        QStringList tags = index.data(Qt::UserRole).toStringList();
        if (tags.isEmpty()) {
            return;
        }

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);

        // 标签绘制参数
        const int tagHeight = 22;
        const int tagPaddingH = 8;  // 水平内边距
        const int tagGap = 6;       // 标签间距
        const int borderRadius = 4;

        QFont tagFont(QStringLiteral("Segoe UI"));
        tagFont.setPixelSize(14);
        tagFont.setStyleName(QStringLiteral("Normal"));
        painter->setFont(tagFont);
        QFontMetrics fm(tagFont);

        // 垂直居中
        int cellTop = option.rect.top();
        int cellHeight = option.rect.height();
        int y = cellTop + (cellHeight - tagHeight) / 2;
        int x = option.rect.left() + 12;  // 左边距 12px

        for (const QString &tag : tags) {
            QString text;
            QColor bgColor;
            QColor textColor;

            if (tag == QStringLiteral("key")) {
                text = QStringLiteral("\u91CD\u70B9");  // 重点
                bgColor = QColor(0xFF, 0xF7, 0xED);     // #FFF7ED
                textColor = QColor(0xF9, 0x73, 0x16);   // #F97316
            } else if (tag == QStringLiteral("annual")) {
                text = QStringLiteral("\u5E74\u5EA6");  // 年度
                bgColor = QColor(0xEF, 0xF6, 0xFF);     // #EFF6FF
                textColor = QColor(0x24, 0x63, 0xEB);   // #2463EB
            } else {
                continue;
            }

            int textWidth = fm.horizontalAdvance(text);
            int tagWidth = textWidth + tagPaddingH * 2;
            QRectF tagRect(x, y, tagWidth, tagHeight);

            // 绘制圆角背景
            painter->setPen(Qt::NoPen);
            painter->setBrush(bgColor);
            painter->drawRoundedRect(tagRect, borderRadius, borderRadius);

            // 绘制文本
            painter->setPen(textColor);
            painter->drawText(tagRect, Qt::AlignCenter, text);

            x += tagWidth + tagGap;
        }

        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override
    {
        Q_UNUSED(option);
        QStringList tags = index.data(Qt::UserRole).toStringList();
        if (tags.isEmpty()) {
            return QSize(60, 32);
        }
        // 粗略估算宽度
        int width = tags.size() * 50 + 12;
        return QSize(width, 32);
    }
};

}  // namespace

// ============================================================================
// 构造
// ============================================================================

AchievementView::AchievementView(QWidget *parent)
    : QWidget(parent)
    , yearCombo_(nullptr)
    , exportBtn_(nullptr)
    , statTotalLabel_(nullptr)
    , statAmountLabel_(nullptr)
    , statKeyLabel_(nullptr)
    , statRateLabel_(nullptr)
    , tableView_(nullptr)
    , model_(nullptr)
    , pagination_(nullptr)
    , currentYear_(QDate::currentDate().year())
    , currentPage_(1)
{
    setupUI();
    loadYearOptions();
    loadData();
}

// ============================================================================
// UI 构建
// ============================================================================

void AchievementView::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(Spacing::XXL, Spacing::XL, Spacing::XXL, Spacing::XL);
    mainLayout->setSpacing(Spacing::LG);

    setupHeader(mainLayout);
    setupStatsCard(mainLayout);
    setupTable(mainLayout);
}

void AchievementView::setupHeader(QVBoxLayout *mainLayout)
{
    QHBoxLayout *headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(Spacing::MD);

    QVBoxLayout *titleLayout = new QVBoxLayout();
    titleLayout->setSpacing(Spacing::XS);

    QLabel *titleLabel = new QLabel(QStringLiteral("\u5E74\u5EA6\u6210\u679C"), this);  // 年度成果
    QFont titleFont(QStringLiteral("Segoe UI"), 22, QFont::Normal);
    titleLabel->setFont(titleFont);
    titleLabel->setStyleSheet("color:#0F172A;");
    titleLayout->addWidget(titleLabel);

    QLabel *subtitleLabel = new QLabel(
        QStringLiteral("\u5DE5\u4F5C\u6210\u679C\u6301\u7EED\u6C89\u6DC0 \u00B7 "
                       "\u5E74\u5EA6\u603B\u7ED3\u81EA\u52A8\u79EF\u7D2F"),
        this);  // 工作成果持续沉淀 · 年度总结自动积累
    QFont subtitleFont(QStringLiteral("Segoe UI"), 14);
    subtitleLabel->setFont(subtitleFont);
    subtitleLabel->setStyleSheet("color:#0F172A;");
    titleLayout->addWidget(subtitleLabel);

    headerLayout->addLayout(titleLayout);
    headerLayout->addStretch();

    // 年份选择下拉
    yearCombo_ = new QComboBox(this);
    yearCombo_->setObjectName("achievementYearCombo");
    yearCombo_->setStyleSheet(
        "QComboBox {"
        "  padding:6px 10px;"
        "  border:1px solid #E2E8F0;"
        "  border-radius:8px;"
        "  font-size:14px;"
        "  background:#FFFFFF;"
        "  color:#0F172A;"
        "  min-width:80px;"
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
        "}");
    headerLayout->addWidget(yearCombo_);

    // "导出总结" 按钮（primary）
    exportBtn_ = new QPushButton(QStringLiteral("\u5BFC\u51FA\u603B\u7ED3"), this);  // 导出总结
    exportBtn_->setObjectName("achievementExportBtn");
    exportBtn_->setCursor(Qt::PointingHandCursor);
    exportBtn_->setStyleSheet(
        "QPushButton#achievementExportBtn {"
        "  background:#2463EB;"
        "  color:#FFFFFF;"
        "  border:1px solid #2463EB;"
        "  border-radius:8px;"
        "  padding:9px 18px;"
        "  font-size:15px;"
        "  font-weight:700;"
        "}"
        "QPushButton#achievementExportBtn:hover {"
        "  background:#1D4ED8;"
        "}");
    connect(exportBtn_, &QPushButton::clicked, this, &AchievementView::onExportClicked);
    headerLayout->addWidget(exportBtn_);

    mainLayout->addLayout(headerLayout);
}

void AchievementView::setupStatsCard(QVBoxLayout *mainLayout)
{
    // 统计卡片容器
    QFrame *statsCard = new QFrame(this);
    statsCard->setObjectName("achievementStatsCard");
    statsCard->setStyleSheet(
        "QFrame#achievementStatsCard {"
        "  background:#FFFFFF;"
        "  border:none;"
        "  border-radius:12px;"
        "}");
    QHBoxLayout *statsLayout = new QHBoxLayout(statsCard);
    statsLayout->setContentsMargins(Spacing::XL, Spacing::LG, Spacing::XL, Spacing::LG);
    statsLayout->setSpacing(0);

    // 统计项工厂 lambda
    auto createStatItem = [statsCard](QLabel *&valueLabel, const QString &labelText,
                                      const QString &objectName) {
        QVBoxLayout *itemLayout = new QVBoxLayout();
        itemLayout->setSpacing(Spacing::XS);
        itemLayout->setAlignment(Qt::AlignCenter);

        QLabel *valueLbl = new QLabel(QStringLiteral("0"), statsCard);
        valueLbl->setObjectName(objectName);
        valueLbl->setAlignment(Qt::AlignCenter);
        valueLbl->setStyleSheet("color:#0F172A; font-size:28px; font-weight:700; border:none;");
        valueLabel = valueLbl;

        QLabel *labelLbl = new QLabel(labelText, statsCard);
        labelLbl->setAlignment(Qt::AlignCenter);
        labelLbl->setStyleSheet("color:#0F172A; font-size:13px; border:none;");

        itemLayout->addWidget(valueLbl);
        itemLayout->addWidget(labelLbl);
        return itemLayout;
    };

    statsLayout->addLayout(createStatItem(statTotalLabel_,
                                          QStringLiteral("\u6210\u679C\u603B\u6570"),
                                          QStringLiteral("achievementTotalStat")));  // 成果总数

    // 分隔线
    QFrame *sep1 = new QFrame(statsCard);
    sep1->setFrameShape(QFrame::VLine);
    sep1->setStyleSheet("color:#E2E8F0; background:#E2E8F0; max-width:1px;");
    statsLayout->addWidget(sep1);

    statsLayout->addLayout(createStatItem(statAmountLabel_,
                                          QStringLiteral("\u7D2F\u8BA1\u91D1\u989D"),
                                          QStringLiteral("achievementAmountStat")));  // 累计金额

    QFrame *sep2 = new QFrame(statsCard);
    sep2->setFrameShape(QFrame::VLine);
    sep2->setStyleSheet("color:#E2E8F0; background:#E2E8F0; max-width:1px;");
    statsLayout->addWidget(sep2);

    statsLayout->addLayout(createStatItem(statKeyLabel_,
                                          QStringLiteral("\u91CD\u70B9\u6210\u679C"),
                                          QStringLiteral("achievementKeyStat")));  // 重点成果

    QFrame *sep3 = new QFrame(statsCard);
    sep3->setFrameShape(QFrame::VLine);
    sep3->setStyleSheet("color:#E2E8F0; background:#E2E8F0; max-width:1px;");
    statsLayout->addWidget(sep3);

    statsLayout->addLayout(createStatItem(statRateLabel_,
                                          QStringLiteral("\u6210\u679C\u7387"),
                                          QStringLiteral("achievementRateStat")));  // 成果率

    mainLayout->addWidget(statsCard);
}

void AchievementView::setupTable(QVBoxLayout *mainLayout)
{
    // 表格卡片容器
    QFrame *cardFrame = new QFrame(this);
    cardFrame->setObjectName("achievementListCard");
    cardFrame->setStyleSheet(
        "QFrame#achievementListCard {"
        "  background:#FFFFFF;"
        "  border:none;"
        "  border-radius:12px;"
        "}");

    QVBoxLayout *cardLayout = new QVBoxLayout(cardFrame);
    cardLayout->setContentsMargins(0, 0, 0, 0);
    cardLayout->setSpacing(0);

    // QTableView
    tableView_ = new QTableView(cardFrame);
    tableView_->setObjectName("achievementTable");
    tableView_->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableView_->setSelectionMode(QAbstractItemView::SingleSelection);
    tableView_->setShowGrid(false);
    tableView_->setAlternatingRowColors(false);
    tableView_->setStyleSheet(
        "QTableView {"
        "  border:none;"
        "  font-size:14px;"
        "  selection-background-color:#EAF1FF;"
        "  selection-color:#172033;"
        "  background:#FFFFFF;"
        "  outline:none;"
        "}"
        "QTableView::item {"
        "  padding:7px;"
        "  border-bottom:1px solid #DCE3EF;"
        "  background:#FFFFFF;"
        "}"
        "QTableView::item:selected {"
        "  background:#EAF1FF;"
        "}"
        "QHeaderView::section {"
        "  background:#F5F8FC;"
        "  color:#000000;"
        "  font-weight:700;"
        "  font-size:14px;"
        "  padding:10px 7px;"
        "  border:none;"
        "  border-right:1px solid #DCE3EF;"
        "}"
        "QHeaderView {"
        "  background:#F5F8FC;"
        "}");

    // 数据模型
    model_ = new QStandardItemModel(this);
    model_->setColumnCount(ColumnCount);
    model_->setHorizontalHeaderLabels(QStringList()
                                      << QStringLiteral("\u7F16\u53F7")          // 编号
                                      << QStringLiteral("\u5DE5\u4F5C\u5185\u5BB9")  // 工作内容
                                      << QStringLiteral("\u5DE5\u4F5C\u5BF9\u8C61")  // 工作对象
                                      << QStringLiteral("\u6210\u679C\u6570\u91CF")  // 成果数量
                                      << QStringLiteral("\u6210\u679C\u91D1\u989D")  // 成果金额
                                      << QStringLiteral("\u5B8C\u6210\u65F6\u95F4")  // 完成时间
                                      << QStringLiteral("\u6807\u8BB0")            // 标记
                                      << QStringLiteral("操作"));
    tableView_->setModel(model_);

    // 标记列使用自定义委托
    tableView_->setItemDelegateForColumn(ColTag, new AchievementTagDelegate(this));

    // 表头设置
    QHeaderView *header = tableView_->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::Interactive);
    header->setSectionResizeMode(ColActions, QHeaderView::Fixed);
    header->setStretchLastSection(false);
    header->setMinimumSectionSize(60);
    header->setHighlightSections(false);
    header->setDefaultAlignment(Qt::AlignCenter);

    tableView_->verticalHeader()->setVisible(false);
    tableView_->verticalHeader()->setDefaultSectionSize(48);

    setColumnWidths();

    cardLayout->addWidget(tableView_);

    // 分页控件
    pagination_ = new PaginationWidget(cardFrame);
    pagination_->setPageSize(50);
    connect(pagination_, &PaginationWidget::pageChanged, this, &AchievementView::onPageChanged);
    cardLayout->addWidget(pagination_);

    mainLayout->addWidget(cardFrame, 1);
    new RoundedCardBorder(cardFrame);
}

void AchievementView::setColumnWidths()
{
    if (!tableView_) {
        return;
    }
    tableView_->setColumnWidth(ColTaskNo, 130);       // 编号
    tableView_->setColumnWidth(ColWorkContent, 280);   // 工作内容
    tableView_->setColumnWidth(ColWorkObject, 160);    // 工作对象
    tableView_->setColumnWidth(ColResultCount, 90);    // 成果数量
    tableView_->setColumnWidth(ColResultAmount, 110);  // 成果金额
    tableView_->setColumnWidth(ColCreateTime, 110);    // 完成时间
    tableView_->setColumnWidth(ColActions, 128);     // 操作
}

// ============================================================================
// 年份下拉加载
// ============================================================================

void AchievementView::loadYearOptions()
{
    yearCombo_->blockSignals(true);
    yearCombo_->clear();

    int currentYear = QDate::currentDate().year();
    // 从当前年份往前推 6 年
    for (int year = currentYear; year >= currentYear - 5; --year) {
        yearCombo_->addItem(QStringLiteral("%1\u5E74").arg(year), year);  // 2026年
    }

    // 选中当前年份
    int idx = yearCombo_->findData(currentYear_);
    if (idx >= 0) {
        yearCombo_->setCurrentIndex(idx);
    }

    yearCombo_->blockSignals(false);

    connect(yearCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AchievementView::onYearChanged);
}

// ============================================================================
// 数据加载
// ============================================================================

void AchievementView::loadData()
{
    // 查询当年全部成果（按 create_time 倒序）
    const AchievementUseCase useCase;
    allAchievements_ = useCase.listByYear(currentYear_);
    const AchievementYearStats stats = useCase.stats(currentYear_);

    updateStats(stats.total, stats.amount, stats.keyCount, stats.completedTasks);

    // 更新分页
    pagination_->setTotal(stats.total);
    pagination_->setPage(currentPage_);
    currentPage_ = pagination_->currentPage();
    pagination_->updateDisplay();

    // 填充表格（当前页）
    fillTablePage();
}

void AchievementView::fillTablePage()
{
    model_->removeRows(0, model_->rowCount());

    int pageSize = 50;
    int from = (currentPage_ - 1) * pageSize;
    int to = from + pageSize;
    if (to > allAchievements_.size()) {
        to = allAchievements_.size();
    }

    for (int i = from; i < to; ++i) {
        const AchievementListItem &item = allAchievements_.at(i);
        const Achievement &ach = item.achievement;

        QList<QStandardItem*> row;
        row.reserve(ColumnCount);

        // 编号
        auto *noItem = new QStandardItem(item.taskNo);
        noItem->setEditable(false);
        noItem->setTextAlignment(Qt::AlignCenter);
        noItem->setData(QColor(0x94, 0xA3, 0xB8), Qt::ForegroundRole);  // #94A3B8
        QFont monoFont(QStringLiteral("Segoe UI"));
        monoFont.setPixelSize(14);
        monoFont.setStyleHint(QFont::Monospace);
        noItem->setFont(monoFont);
        row.append(noItem);

        // 工作内容
        auto *contentItem = new QStandardItem(ach.workContent);
        contentItem->setEditable(false);
        contentItem->setTextAlignment(Qt::AlignCenter);
        row.append(contentItem);

        // 工作对象
        auto *objectItem = new QStandardItem(ach.workObject);
        objectItem->setEditable(false);
        objectItem->setTextAlignment(Qt::AlignCenter);
        row.append(objectItem);

        // 成果数量（+ 份 单位）
        auto *countItem = new QStandardItem(
            ach.resultCount > 0
                ? QStringLiteral("%1 \u4EFD").arg(ach.resultCount)  // N 份
                : QStringLiteral("0 \u4EFD"));
        countItem->setEditable(false);
        countItem->setTextAlignment(Qt::AlignCenter);
        row.append(countItem);

        // 成果金额（元，0 显示 "—"）
        QString amountText;
        if (ach.resultAmount > 0) {
            amountText = QStringLiteral("\u00A5") + AmountUtil::centsToYuan(ach.resultAmount);  // ¥xxx.xx
        } else {
            amountText = QStringLiteral("\u2014");  // —
        }
        auto *amountItem = new QStandardItem(amountText);
        amountItem->setEditable(false);
        amountItem->setTextAlignment(Qt::AlignCenter);
        row.append(amountItem);

        // 完成时间（yyyy-MM-dd）
        QString dateText;
        if (ach.createTime.isValid()) {
            dateText = ach.createTime.toString("yyyy-MM-dd");
        }
        auto *dateItem = new QStandardItem(dateText);
        dateItem->setEditable(false);
        dateItem->setTextAlignment(Qt::AlignCenter);
        row.append(dateItem);

        // 标记（UserRole 存标签列表，委托渲染）
        auto *tagItem = new QStandardItem();
        tagItem->setEditable(false);
        QStringList tags;
        if (ach.isKeyAchievement) {
            tags << QStringLiteral("key");
        }
        if (ach.isAnnual) {
            tags << QStringLiteral("annual");
        }
        tagItem->setData(tags, Qt::UserRole);
        row.append(tagItem);

        auto *actionsItem = new QStandardItem;
        actionsItem->setEditable(false);
        row.append(actionsItem);
        model_->appendRow(row);
        setRowActions(model_->rowCount() - 1, item);
    }
}

void AchievementView::setRowActions(int row, const AchievementListItem &item)
{
    auto *actions = new QWidget(tableView_);
    auto *layout = new QHBoxLayout(actions);
    layout->setContentsMargins(6, 0, 6, 0);
    layout->setSpacing(Spacing::SM);
    auto *edit = new QPushButton(QStringLiteral("编辑"), actions);
    auto *remove = new QPushButton(QStringLiteral("删除"), actions);
    edit->setObjectName("achievementRowEdit");
    remove->setObjectName("achievementRowDelete");
    const QString buttonStyle = QStringLiteral(
        "QPushButton { background: #FFFFFF; color: %1; border: 1px solid %1; "
        "border-radius: 4px; padding: 4px 10px; font-size: 12px; }"
        "QPushButton:hover { background: %2; }");
    edit->setStyleSheet(buttonStyle.arg(QStringLiteral("#2463EB"), QStringLiteral("#EFF6FF")));
    remove->setStyleSheet(buttonStyle.arg(QStringLiteral("#DC2626"), QStringLiteral("#FEF2F2")));
    edit->setCursor(Qt::PointingHandCursor);
    remove->setCursor(Qt::PointingHandCursor);
    layout->addStretch();
    layout->addWidget(edit);
    layout->addWidget(remove);
    layout->addStretch();
    tableView_->setIndexWidget(model_->index(row, ColActions), actions);

    connect(edit, &QPushButton::clicked, this, [this, item]() {
        const Achievement &achievement = item.achievement;
        AchievementDialog dialog(achievement.taskId, item.taskCategory,
                                 achievement.workContent, this, achievement);
        if (dialog.exec() == QDialog::Accepted) loadData();
    });
    connect(remove, &QPushButton::clicked, this, [this, id = item.achievement.id]() {
        if (AchievementDialog::confirmDelete(id, this)) loadData();
    });
}

void AchievementView::updateStats(int totalCount, qint64 totalAmount, int keyCount, int completedCount)
{
    // 成果总数
    statTotalLabel_->setText(QString::number(totalCount));

    // 累计金额（元）
    statAmountLabel_->setText(QStringLiteral("\u00A5") + AmountUtil::centsToYuan(totalAmount));  // ¥xxx.xx

    // 重点成果
    statKeyLabel_->setText(QString::number(keyCount));

    // 成果率 = 成果数 / 已完成任务数 * 100%
    QString rateText;
    if (completedCount > 0) {
        double rate = static_cast<double>(totalCount) / completedCount * 100.0;
        rateText = QStringLiteral("%1%").arg(QString::number(rate, 'f', 0));
    } else {
        rateText = QStringLiteral("0%");
    }
    statRateLabel_->setText(rateText);
}

// ============================================================================
// 刷新
// ============================================================================

void AchievementView::refresh()
{
    loadData();
}

// ============================================================================
// 槽
// ============================================================================

void AchievementView::onYearChanged(int index)
{
    if (index < 0) {
        return;
    }
    QVariant data = yearCombo_->itemData(index);
    bool ok = false;
    int year = data.toInt(&ok);
    if (!ok || year == currentYear_) {
        return;
    }
    currentYear_ = year;
    currentPage_ = 1;
    loadData();
}

void AchievementView::onExportClicked()
{
    emit exportRequested(currentYear_);
}

void AchievementView::onPageChanged(int page)
{
    currentPage_ = page;
    fillTablePage();
}
