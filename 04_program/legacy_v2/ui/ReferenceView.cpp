/**
 * @file ReferenceView.cpp
 * @brief 参考资料页面实现（T4.8）
 *
 * 依赖：ui/ReferenceView.h、application/reference、
 *       models/ReferenceMaterial、service/FileService、ui/widgets/PaginationWidget、utils/Logger
 * 文档：task.md T4.8、qt_implementation_guide.md 3.3
 */

#include "ReferenceView.h"
#include "widgets/RoundedCardBorder.h"
#include "widgets/ListFilterControls.h"
#include "widgets/YearPicker.h"

#include <QComboBox>
#include <QDateEdit>
#include <QDesktopServices>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QSizePolicy>
#include <QStandardItemModel>
#include <QTableView>
#include <QUrl>
#include <QVBoxLayout>
#include <QWheelEvent>

#include "ColorPalette.h"
#include "Radius.h"
#include "Spacing.h"
#include "application/reference/ReferenceQueryUseCase.h"
#include "models/ReferenceMaterial.h"
#include "service/FileService.h"
#include "ui/widgets/PaginationWidget.h"
#include "utils/Logger.h"

// 列定义
namespace {
enum Column {
    ColRefNo = 0,
    ColName = 1,
    ColSource = 2,
    ColTag = 3,
    ColCreateTime = 4,
    ColCount = 5
};
}

// ============================================================================
// 构造
// ============================================================================

ReferenceView::ReferenceView(QWidget *parent)
    : QWidget(parent)
    , searchEdit_(nullptr)
    , sourceCombo_(nullptr)
    , tagCombo_(nullptr)
    , yearCombo_(nullptr)
    , startDateEdit_(nullptr)
    , endDateEdit_(nullptr)
    , searchBtn_(nullptr)
    , resetBtn_(nullptr)
    , tableView_(nullptr)
    , model_(nullptr)
    , emptyLabel_(nullptr)
    , pagination_(nullptr)
{
    filter_.pageSize = 50;
    setupUI();
    loadFilterOptions();
    filter_.year = QDate::currentDate().year();
    filter_.startDate = startDateEdit_->date();
    filter_.endDate = endDateEdit_->date();
}

// ============================================================================
// UI 构建
// ============================================================================

void ReferenceView::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(Spacing::XXL, Spacing::XL, Spacing::XXL, Spacing::XL);
    mainLayout->setSpacing(Spacing::LG);

    // === page-header ===
    QHBoxLayout *headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(Spacing::MD);

    QVBoxLayout *titleLayout = new QVBoxLayout();
    titleLayout->setSpacing(Spacing::XS);

    QLabel *titleLabel = new QLabel(QStringLiteral("参考资料"), this);
    QFont titleFont(QStringLiteral("MiSans"), 22, QFont::Normal);
    titleLabel->setFont(titleFont);
    titleLabel->setStyleSheet("color:#0F172A;");
    titleLayout->addWidget(titleLabel);

    QLabel *subtitleLabel = new QLabel(
        QStringLiteral("归纳整理工作中常用的各类文件资料"), this);
    QFont subtitleFont(QStringLiteral("MiSans"), 14);
    subtitleLabel->setFont(subtitleFont);
    subtitleLabel->setStyleSheet("color:#0F172A;");
    titleLayout->addWidget(subtitleLabel);

    headerLayout->addLayout(titleLayout);
    headerLayout->addStretch();

    // "打开目录" 按钮（secondary）
    QPushButton *openDirBtn = new QPushButton(QStringLiteral("打开目录"), this);
    openDirBtn->setCursor(Qt::PointingHandCursor);
    openDirBtn->setStyleSheet(
        "QPushButton {"
        "  background:#FFFFFF;"
        "  color:#475569;"
        "  border:1px solid #E2E8F0;"
        "  border-radius:8px;"
        "  padding:9px 18px;"
        "  font-size:15px;"
        "}"
        "QPushButton:hover {"
        "  background:#F5F7FA;"
        "  border-color:#CBD5E1;"
        "}"
    );
    connect(openDirBtn, &QPushButton::clicked, this, &ReferenceView::onOpenDirClicked);
    headerLayout->addWidget(openDirBtn);

    // "＋ 新建资料" 按钮（primary）
    QPushButton *createBtn = new QPushButton(QStringLiteral("＋ 新建资料"), this);
    createBtn->setObjectName("btnPrimary");
    createBtn->setCursor(Qt::PointingHandCursor);
    createBtn->setStyleSheet(
        "QPushButton#btnPrimary {"
        "  background:#2463EB;"
        "  color:#FFFFFF;"
        "  border:1px solid #2463EB;"
        "  border-radius:8px;"
        "  padding:9px 18px;"
        "  font-size:15px;"
        "  font-weight:700;"
        "}"
        "QPushButton#btnPrimary:hover {"
        "  background:#1D4ED8;"
        "}"
    );
    connect(createBtn, &QPushButton::clicked, this, &ReferenceView::onCreateReferenceClicked);
    headerLayout->addWidget(createBtn);

    mainLayout->addLayout(headerLayout);

    // === 筛选栏 ===
    setupFilterBar();

    // === 表格区 ===
    setupTable();
}

void ReferenceView::setupFilterBar()
{
    QHBoxLayout *filterLayout = new QHBoxLayout();
    filterLayout->setContentsMargins(0, 0, 0, 0);
    filterLayout->setSpacing(Spacing::SM);

    // 搜索框（可拉伸，窗口放大时仅搜索框变宽）
    searchEdit_ = new QLineEdit(this);
    searchEdit_->setObjectName("referenceSearchEdit");
    searchEdit_->setPlaceholderText(QStringLiteral("搜索名称、编号或附件名..."));
    searchEdit_->setMinimumWidth(200);
    searchEdit_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    searchEdit_->setStyleSheet(
        "QLineEdit {"
        "  padding:8px 12px;"
        "  border:1px solid #E2E8F0;"
        "  border-radius:8px;"
        "  font-size:14px;"
        "  background:#FFFFFF;"
        "  color:#0F172A;"
        "}"
        "QLineEdit:focus {"
        "  border:2px solid #2463EB;"
        "  padding:7px 11px;"
        "}"
    );
    filterLayout->addWidget(searchEdit_);

    // 来源下拉（固定宽度）
    sourceCombo_ = new QComboBox(this);
    sourceCombo_->setObjectName("referenceSourceCombo");
    sourceCombo_->setFixedWidth(140);
    sourceCombo_->setStyleSheet(
        "QComboBox {"
        "  padding:8px 12px;"
        "  border:1px solid #E2E8F0;"
        "  border-radius:8px;"
        "  font-size:14px;"
        "  background:#FFFFFF;"
        "  color:#0F172A;"
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
        "}"
    );
    filterLayout->addWidget(sourceCombo_);

    // 标签下拉（固定宽度）
    tagCombo_ = new QComboBox(this);
    tagCombo_->setObjectName("referenceTagCombo");
    tagCombo_->setFixedWidth(140);
    tagCombo_->setStyleSheet(sourceCombo_->styleSheet());
    filterLayout->addWidget(tagCombo_);

    QHBoxLayout *dateLayout = new QHBoxLayout();
    dateLayout->setContentsMargins(0, 0, 0, 0);
    dateLayout->setSpacing(Spacing::SM);
    yearCombo_ = new YearPicker(this);
    yearCombo_->setObjectName("referenceYearCombo");
    yearCombo_->setFixedWidth(156);
    ListFilterControls::reloadYears(yearCombo_, {});
    dateLayout->addWidget(yearCombo_);

    // 开始日期
    startDateEdit_ = new QDateEdit(this);
    startDateEdit_->setCalendarPopup(true);
    startDateEdit_->setDisplayFormat("yyyy-MM-dd");
    startDateEdit_->setFixedWidth(130);
    startDateEdit_->setStyleSheet(
        "QDateEdit {"
        "  padding:8px 10px;"
        "  border:1px solid #E2E8F0;"
        "  border-radius:8px;"
        "  font-size:14px;"
        "  background:#FFFFFF;"
        "  color:#0F172A;"
        "}"
    );
    dateLayout->addWidget(startDateEdit_);

    // "至" 字
    QLabel *toLabel = new QLabel(QStringLiteral("至"), this);
    toLabel->setStyleSheet("color:#0F172A; font-size:14px;");
    dateLayout->addWidget(toLabel);

    // 结束日期
    endDateEdit_ = new QDateEdit(this);
    endDateEdit_->setCalendarPopup(true);
    endDateEdit_->setDisplayFormat("yyyy-MM-dd");
    endDateEdit_->setFixedWidth(130);
    endDateEdit_->setStyleSheet(startDateEdit_->styleSheet());
    dateLayout->addWidget(endDateEdit_);

    // 搜索按钮（primary sm）
    searchBtn_ = new QPushButton(QStringLiteral("搜索"), this);
    searchBtn_->setObjectName("referenceSearchButton");
    searchBtn_->setCursor(Qt::PointingHandCursor);
    searchBtn_->setStyleSheet(
        "QPushButton {"
        "  background:#2463EB;"
        "  color:#FFFFFF;"
        "  border:1px solid #2463EB;"
        "  border-radius:8px;"
        "  padding:8px 16px;"
        "  font-size:14px;"
        "  font-weight:700;"
        "}"
        "QPushButton:hover {"
        "  background:#1D4ED8;"
        "}"
    );
    connect(searchBtn_, &QPushButton::clicked, this, &ReferenceView::onSearchClicked);
    dateLayout->addWidget(searchBtn_);

    // 重置按钮（secondary sm）
    resetBtn_ = new QPushButton(QStringLiteral("重置"), this);
    resetBtn_->setObjectName("referenceResetButton");
    resetBtn_->setCursor(Qt::PointingHandCursor);
    resetBtn_->setStyleSheet(
        "QPushButton {"
        "  background:#FFFFFF;"
        "  color:#475569;"
        "  border:1px solid #E2E8F0;"
        "  border-radius:8px;"
        "  padding:8px 16px;"
        "  font-size:14px;"
        "}"
        "QPushButton:hover {"
        "  background:#F5F7FA;"
        "  border-color:#CBD5E1;"
        "}"
    );
    connect(resetBtn_, &QPushButton::clicked, this, &ReferenceView::onResetClicked);
    dateLayout->addWidget(resetBtn_);
    dateLayout->addStretch();

    ListFilterControls::syncYearRange(yearCombo_, startDateEdit_, endDateEdit_);
    connect(yearCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ReferenceView::onYearChanged);
    for (QComboBox *combo : {sourceCombo_, tagCombo_}) {
        connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ReferenceView::onFilterChanged);
    }
    for (QDateEdit *edit : {startDateEdit_, endDateEdit_}) {
        connect(edit, &QDateEdit::dateChanged, this, [this]() { dateRangeEdited_ = true; });
    }

    QVBoxLayout *mainLayout = qobject_cast<QVBoxLayout*>(this->layout());
    if (mainLayout) {
        mainLayout->addLayout(filterLayout);
        mainLayout->addLayout(dateLayout);
    }
}

void ReferenceView::setupTable()
{
    // 表格卡片容器（QFrame#Card）
    QFrame *cardFrame = new QFrame(this);
    cardFrame->setObjectName("referenceListCard");
    cardFrame->setStyleSheet(
        "QFrame#referenceListCard {"
        "  background:#FFFFFF;"
        "  border:none;"
        "  border-radius:12px;"
        "}"
    );

    QVBoxLayout *cardLayout = new QVBoxLayout(cardFrame);
    cardLayout->setContentsMargins(1, 1, 1, 1);
    cardLayout->setSpacing(0);

    // QTableView
    tableView_ = new QTableView(cardFrame);
    tableView_->setObjectName("referenceTable");
    tableView_->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableView_->setSelectionMode(QAbstractItemView::SingleSelection);
    tableView_->setShowGrid(false);
    tableView_->setAlternatingRowColors(false);
    tableView_->setMouseTracking(true);
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
        "}"
    );

    // 数据模型
    model_ = new QStandardItemModel(this);
    model_->setColumnCount(ColCount);
    model_->setHorizontalHeaderLabels({
        QStringLiteral("编号"),
        QStringLiteral("资料名称"),
        QStringLiteral("来源"),
        QStringLiteral("标签"),
        QStringLiteral("创建时间")
    });
    tableView_->setModel(model_);

    // 表头设置
    QHeaderView *header = tableView_->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::Interactive);
    header->setStretchLastSection(false);
    header->setHighlightSections(false);
    header->setDefaultAlignment(Qt::AlignCenter);
    header->setMinimumSectionSize(60);

    tableView_->verticalHeader()->setVisible(false);
    tableView_->verticalHeader()->setDefaultSectionSize(48);
    tableView_->viewport()->installEventFilter(this);

    setColumnWidths();

    // 信号连接
    connect(tableView_, &QTableView::clicked, this, &ReferenceView::onReferenceClicked);
    connect(tableView_, &QTableView::doubleClicked, this, &ReferenceView::onReferenceDoubleClicked);

    cardLayout->addWidget(tableView_, 1);

    // 空状态标签
    emptyLabel_ = new QLabel(QStringLiteral("暂无参考资料"), tableView_->viewport());
    emptyLabel_->setObjectName("referenceEmptyState");
    emptyLabel_->setAlignment(Qt::AlignCenter);
    emptyLabel_->setStyleSheet(
        "QLabel {"
        "  color:#0F172A;"
        "  font-size:15px;"
        "  background:transparent;"
        "  border:none;"
        "}"
    );
    emptyLabel_->hide();
    emptyLabel_->setAttribute(Qt::WA_TransparentForMouseEvents);
    emptyLabel_->setGeometry(tableView_->viewport()->rect());

    // 分页控件
    pagination_ = new PaginationWidget(cardFrame);
    pagination_->setPageSize(50);
    connect(pagination_, &PaginationWidget::pageChanged, this, &ReferenceView::onPageChanged);
    cardLayout->addWidget(pagination_);

    QVBoxLayout *mainLayout = qobject_cast<QVBoxLayout*>(this->layout());
    if (mainLayout) {
        mainLayout->addWidget(cardFrame, 1);
    }
    new RoundedCardBorder(cardFrame);
}

bool ReferenceView::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == tableView_->viewport() && event->type() == QEvent::Resize) {
        emptyLabel_->setGeometry(tableView_->viewport()->rect());
    }
    if (watched == tableView_->viewport() && event->type() == QEvent::Wheel) {
        auto *wheel = static_cast<QWheelEvent *>(event);
        QScrollBar *vertical = tableView_->verticalScrollBar();
        QScrollBar *horizontal = tableView_->horizontalScrollBar();
        if (vertical->maximum() == vertical->minimum()
            && horizontal->maximum() > horizontal->minimum()) {
            const int delta = wheel->angleDelta().y();
            if (delta != 0) horizontal->setValue(horizontal->value() - delta);
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void ReferenceView::setColumnWidths()
{
    if (!tableView_) {
        return;
    }
    tableView_->setColumnWidth(ColRefNo, 140);      // 编号
    tableView_->setColumnWidth(ColName, 320);       // 资料名称
    tableView_->setColumnWidth(ColSource, 120);     // 来源
    tableView_->setColumnWidth(ColTag, 120);        // 标签
    tableView_->setColumnWidth(ColCreateTime, 140); // 创建时间

    // 设置最小列宽确保标题完整显示
    QHeaderView *header = tableView_->horizontalHeader();
    header->setMinimumSectionSize(60);
}

// ============================================================================
// 筛选选项加载
// ============================================================================

void ReferenceView::loadFilterOptions()
{
    const ReferenceListData data = ReferenceQueryUseCase().loadList(filter_);
    ListFilterControls::reloadOptions(sourceCombo_, QStringLiteral("全部来源"), data.sources);
    ListFilterControls::reloadOptions(tagCombo_, QStringLiteral("全部标签"), data.tags);
    ListFilterControls::reloadYears(yearCombo_, data.years);
}

// ============================================================================
// 刷新与数据加载
// ============================================================================

void ReferenceView::refresh()
{
    loadFilterOptions();  // 重新加载来源和标签下拉选项（基于已存在参考资料）
    applyFilterToModel();
}

void ReferenceView::showCreatedReference()
{
    resetFilter();
    refresh();
}

void ReferenceView::applyFilterToModel()
{
    const ReferenceListData listData = ReferenceQueryUseCase().loadList(filter_);
    const QList<ReferenceMaterial> &refs = listData.items;
    const int total = listData.total;

    // 清空并重新填充模型
    model_->removeRows(0, model_->rowCount());

    QFont monoFont(QStringLiteral("MiSans"));
    monoFont.setPixelSize(14);
    monoFont.setStyleHint(QFont::Monospace);
    QFont bodyFont = tableView_->font();
    bodyFont.setPixelSize(14);
    bodyFont.setWeight(QFont::Normal);

    for (const ReferenceMaterial &ref : refs) {
        QList<QStandardItem*> row;

        // 编号（monospace, 14px）
        QStandardItem *refNoItem = new QStandardItem(ref.refNo);
        refNoItem->setFont(monoFont);
        refNoItem->setData(ref.id, Qt::UserRole);  // 存储 refId
        refNoItem->setEditable(false);
        refNoItem->setTextAlignment(Qt::AlignCenter);
        refNoItem->setForeground(QColor("#0F172A"));
        row.append(refNoItem);

        // 资料名称
        QStandardItem *nameItem = new QStandardItem(ref.name);
        nameItem->setFont(bodyFont);
        nameItem->setEditable(false);
        nameItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        nameItem->setForeground(QColor("#0F172A"));
        row.append(nameItem);

        // 来源（居中）
        QStandardItem *sourceItem = new QStandardItem(ref.source);
        sourceItem->setTextAlignment(Qt::AlignCenter);
        sourceItem->setFont(bodyFont);
        sourceItem->setEditable(false);
        sourceItem->setForeground(QColor("#0F172A"));
        row.append(sourceItem);

        // 标签（居中，灰色 tag 样式）
        QStandardItem *tagItem = new QStandardItem(ref.tag);
        tagItem->setTextAlignment(Qt::AlignCenter);
        tagItem->setFont(bodyFont);
        tagItem->setEditable(false);
        tagItem->setBackground(QColor("#F1F5F9"));
        tagItem->setForeground(QColor("#0F172A"));
        row.append(tagItem);

        // 创建时间（居中, yyyy-MM-dd）
        QString dateStr = ref.createTime.isValid()
                              ? ref.createTime.toString("yyyy-MM-dd")
                              : QStringLiteral("-");
        QStandardItem *timeItem = new QStandardItem(dateStr);
        timeItem->setTextAlignment(Qt::AlignCenter);
        timeItem->setFont(bodyFont);
        timeItem->setEditable(false);
        timeItem->setForeground(QColor("#0F172A"));
        row.append(timeItem);

        model_->appendRow(row);
    }

    pagination_->setTotal(total);
    pagination_->setPage(filter_.page);
    pagination_->updateDisplay();

    updateEmptyState(total);
}

void ReferenceView::updateEmptyState(int total)
{
    if (total == 0) {
        emptyLabel_->setText(QStringLiteral("没有符合当前筛选条件的资料"));
        // 保持 tableView_ 可见（标题行常显），仅隐藏分页
        tableView_->show();
        emptyLabel_->show();
        emptyLabel_->raise();
        pagination_->hide();
    } else {
        tableView_->show();
        emptyLabel_->hide();
        pagination_->show();
    }
}

// ============================================================================
// 筛选栏操作
// ============================================================================

void ReferenceView::onFilterChanged()
{
    // 来源：index=0 为"全部来源"
    if (sourceCombo_->currentIndex() > 0) {
        filter_.source = sourceCombo_->currentText();
    } else {
        filter_.source.clear();
    }

    // 标签：index=0 为"全部标签"
    if (tagCombo_->currentIndex() > 0) {
        filter_.tag = tagCombo_->currentText();
    } else {
        filter_.tag.clear();
    }

    filter_.page = 1;
    applyFilterToModel();
}

void ReferenceView::onSearchClicked()
{
    filter_.keyword = searchEdit_->text().trimmed();
    filter_.year = yearCombo_->currentData().toInt();
    const bool useRange = filter_.year > 0 || dateRangeEdited_;
    filter_.startDate = useRange ? startDateEdit_->date() : QDate();
    filter_.endDate = useRange ? endDateEdit_->date() : QDate();
    onFilterChanged();
}

void ReferenceView::onYearChanged()
{
    ListFilterControls::syncYearRange(yearCombo_, startDateEdit_, endDateEdit_);
    dateRangeEdited_ = false;
    filter_.year = yearCombo_->currentData().toInt();
    filter_.startDate = filter_.year > 0 ? startDateEdit_->date() : QDate();
    filter_.endDate = filter_.year > 0 ? endDateEdit_->date() : QDate();
    onFilterChanged();
}

void ReferenceView::onResetClicked()
{
    resetFilter();
}

void ReferenceView::resetFilter()
{
    searchEdit_->clear();
    for (QComboBox *combo : {sourceCombo_, tagCombo_}) {
        const QSignalBlocker blocker(combo);
        combo->setCurrentIndex(0);
    }
    {
        const QSignalBlocker blocker(yearCombo_);
        yearCombo_->setCurrentIndex(yearCombo_->findData(QDate::currentDate().year()));
    }
    ListFilterControls::syncYearRange(yearCombo_, startDateEdit_, endDateEdit_);
    dateRangeEdited_ = false;

    filter_.keyword.clear();
    filter_.source.clear();
    filter_.tag.clear();
    filter_.year = QDate::currentDate().year();
    filter_.startDate = startDateEdit_->date();
    filter_.endDate = endDateEdit_->date();
    filter_.page = 1;
    filter_.pageSize = 50;

    applyFilterToModel();
}

void ReferenceView::onCreateReferenceClicked()
{
    emit createReferenceClicked();
}

void ReferenceView::onOpenDirClicked()
{
    FileService fileService;
    QString workDir = fileService.getWorkDir();
    if (workDir.isEmpty()) {
        Logger::instance().warn(QStringLiteral("ReferenceView: 工作目录未设置"));
        return;
    }
    QString refDir = workDir + "/" + FileService::kDirReference;
    QDesktopServices::openUrl(QUrl::fromLocalFile(refDir));
}

// ============================================================================
// 表格交互
// ============================================================================

void ReferenceView::onReferenceClicked(const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }
    int row = index.row();
    QStandardItem *item = model_->item(row, ColRefNo);
    if (item) {
        int refId = item->data(Qt::UserRole).toInt();
        if (refId > 0) {
            emit referenceClicked(refId);
        }
    }
}

void ReferenceView::onReferenceDoubleClicked(const QModelIndex &index)
{
    onReferenceClicked(index);
}

void ReferenceView::onPageChanged(int page)
{
    filter_.page = page;
    applyFilterToModel();
}
