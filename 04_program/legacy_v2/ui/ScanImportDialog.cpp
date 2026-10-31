/**
 * @file ScanImportDialog.cpp
 * @brief 扫描导入弹窗实现（T8.7）
 *
 * 实现：setupUI 搭建界面 / onScanClicked 展示扫描结果 / onImportClicked 提交所选项
 *
 * 依赖：ui/ScanImportDialog.h、ui/Spacing.h、ui/ColorPalette.h、application/maintenance/ScanImportUseCase
 * 文档：task.md T8.7、PRD 13.3
 */

#include "ScanImportDialog.h"

#include <QFrame>
#include <QDir>
#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QScrollArea>

#include "ColorPalette.h"
#include "Spacing.h"
#include "application/maintenance/ScanImportUseCase.h"
#include "database/DatabaseManager.h"
#include "database/TaskDao.h"
#include "database/ReferenceMaterialDao.h"
#include "service/ScanMatchService.h"
#include "service/FileService.h"
#include "service/TaskInfoFile.h"
#include "service/ReferenceInfoFile.h"
#include "widgets/WindowDragger.h"
#include "widgets/RoundedCornerFilter.h"
#include "widgets/StyledMessageBox.h"

namespace {
void addDistinctChoice(QComboBox *box, const QString &value, const QString &source)
{
    if (box->findData(value, Qt::UserRole + 1) >= 0) return;
    box->addItem(value.isEmpty() ? QStringLiteral("（空）") : value, source);
    box->setItemData(box->count() - 1, value, Qt::UserRole + 1);
}
}

// ============================================================================
// 构造
// ============================================================================

ScanImportDialog::ScanImportDialog(QWidget *parent)
    : QDialog(parent)
    , table_(nullptr)
    , hintLabel_(nullptr)
{
    setupUI();
}

// ============================================================================
// UI 搭建
// ============================================================================

void ScanImportDialog::setupUI()
{
    setObjectName("scanImportDialog");
    // 窗口属性：无边框、模态、可调整大小
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setModal(true);
    setMinimumWidth(900);
    setSizeGripEnabled(true);
    installEventFilter(new RoundedCornerFilter(12, this));

    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(1, 1, 1, 1);
    mainLayout->setSpacing(0);
    setStyleSheet("QDialog#scanImportDialog { background: #FFFFFF; "
                  "border: 1px solid #DCE3EF; border-radius: 12px; }");

    // ---- 自定义标题栏 ----
    QFrame *headerFrame = new QFrame(this);
    headerFrame->setObjectName("dialogHeader");
    headerFrame->setStyleSheet(
        "#dialogHeader { background: #FFFFFF; border-bottom: none; }");
    headerFrame->installEventFilter(new WindowDragger(this));
    QHBoxLayout *headerLayout = new QHBoxLayout(headerFrame);
    headerLayout->setContentsMargins(Spacing::XXL, Spacing::XL, Spacing::XL, Spacing::XL);
    headerLayout->setSpacing(Spacing::SM);

    QLabel *titleLabel = new QLabel(QStringLiteral("扫描导入"), headerFrame);
    titleLabel->setObjectName("scanImportTitle");
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

    connect(closeBtn, &QPushButton::clicked, this, &ScanImportDialog::onCancelClicked);

    mainLayout->addWidget(headerFrame);

    // ---- 内容区域 ----
    QFrame *bodyFrame = new QFrame(this);
    bodyFrame->setObjectName("dialogBody");
    bodyFrame->setStyleSheet(
        "#dialogBody { background: #FFFFFF; }"
        "#dialogBody QLabel { color: #475569; font-size: 13px; }");
    QVBoxLayout *bodyLayout = new QVBoxLayout(bodyFrame);
    bodyLayout->setContentsMargins(Spacing::XXL, Spacing::XL, Spacing::XXL, Spacing::XL);
    bodyLayout->setSpacing(Spacing::MD);

    // 说明文本
    hintLabel_ = new QLabel(
        QStringLiteral("扫描工作目录下未导入的任务和资料目录，预览后选择导入"),
        bodyFrame);
    hintLabel_->setObjectName("scanImportHint");
    hintLabel_->setStyleSheet("color: #64748B; font-size: 13px; border: none;");
    bodyLayout->addWidget(hintLabel_);

    // 开始扫描按钮
    QPushButton *scanBtn = new QPushButton(QStringLiteral("开始扫描"), bodyFrame);
    scanBtn->setObjectName("scanImportScan");
    scanBtn->setCursor(Qt::PointingHandCursor);
    scanBtn->setFocusPolicy(Qt::NoFocus);
    scanBtn->setMinimumWidth(120);
    scanBtn->setStyleSheet(
        "#scanImportScan { background: #2463EB; color: #FFFFFF; "
        "border: 1px solid #2463EB; border-radius: 8px; padding: 9px 18px; "
        "font-size: 14px; font-weight: 700; }"
        "#scanImportScan:hover { background: #1D4ED8; }");
    bodyLayout->addWidget(scanBtn);

    connect(scanBtn, &QPushButton::clicked, this, &ScanImportDialog::onScanClicked);

    // 预览表格
    table_ = new QTableWidget(bodyFrame);
    table_->setObjectName("scanImportTable");
    table_->setColumnCount(5);
    table_->setHorizontalHeaderLabels({
        QStringLiteral("选择"),
        QStringLiteral("目录名"),
        QStringLiteral("解析结果"),
        QStringLiteral("导入类型"),
        QStringLiteral("状态")
    });
    table_->setStyleSheet(
        "QTableWidget { border: 1px solid #E2E8F0; border-radius: 8px; "
        "gridline-color: #F1F5F9; font-size: 13px; }"
        "QHeaderView::section { background: #F5F7FA; color: #475569; "
        "border: none; border-bottom: none; padding: 8px; font-size: 13px; }"
        "QTableWidget::item { padding: 8px; }");
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->verticalHeader()->setVisible(false);
    table_->horizontalHeader()->setStretchLastSection(false);
    table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    table_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    table_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    table_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    table_->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    table_->setColumnWidth(0, 50);
    table_->setColumnWidth(3, 100);
    table_->setColumnWidth(4, 120);
    table_->setMinimumHeight(320);
    bodyLayout->addWidget(table_);
    connect(table_, &QTableWidget::currentCellChanged, this,
            [this](int row, int, int, int) {
        if (row >= 0 && row < scanItems_.size() && scanItems_.at(row).isConflict)
            hintLabel_->setText(QStringLiteral("冲突原因：") + scanItems_.at(row).conflictReason);
    });

    mainLayout->addWidget(bodyFrame);

    // ---- 底部按钮栏 ----
    QFrame *footerFrame = new QFrame(this);
    footerFrame->setObjectName("dialogFooter");
    footerFrame->setStyleSheet(
        "#dialogFooter { background: #FFFFFF; border-top: 1px solid #E2E8F0; }");
    QHBoxLayout *footerLayout = new QHBoxLayout(footerFrame);
    footerLayout->setContentsMargins(Spacing::XXL, Spacing::LG, Spacing::XXL, Spacing::LG);
    footerLayout->setSpacing(Spacing::SM);

    QPushButton *selectAllBtn = new QPushButton(QStringLiteral("全选"), footerFrame);
    selectAllBtn->setObjectName("scanImportSelectAll");
    selectAllBtn->setCursor(Qt::PointingHandCursor);
    selectAllBtn->setFocusPolicy(Qt::NoFocus);
    selectAllBtn->setMinimumWidth(96);
    selectAllBtn->setStyleSheet(
        "#scanImportSelectAll { background: #FFFFFF; color: #475569; "
        "border: 1px solid #E2E8F0; border-radius: 8px; padding: 9px 18px; font-size: 14px; }"
        "#scanImportSelectAll:hover { background: #F5F7FA; border-color: #CBD5E1; }");

    footerLayout->addWidget(selectAllBtn);
    QPushButton *resolveBtn = new QPushButton(QStringLiteral("解决选中冲突"), footerFrame);
    resolveBtn->setObjectName("scanImportResolve");
    resolveBtn->setCursor(Qt::PointingHandCursor);
    resolveBtn->setStyleSheet(
        "QPushButton { background: #FFFFFF; color: #2463EB; border: 1px solid #BFDBFE; "
        "border-radius: 8px; padding: 9px 18px; font-size: 14px; }"
        "QPushButton:hover { background: #EFF6FF; }");
    footerLayout->addWidget(resolveBtn);
    footerLayout->addStretch();

    QPushButton *cancelBtn = new QPushButton(QStringLiteral("取消"), footerFrame);
    cancelBtn->setObjectName("scanImportCancel");
    cancelBtn->setCursor(Qt::PointingHandCursor);
    cancelBtn->setFocusPolicy(Qt::NoFocus);
    cancelBtn->setMinimumWidth(96);
    cancelBtn->setStyleSheet(
        "#scanImportCancel { background: #FFFFFF; color: #475569; "
        "border: 1px solid #E2E8F0; border-radius: 8px; padding: 9px 18px; font-size: 14px; }"
        "#scanImportCancel:hover { background: #F5F7FA; border-color: #CBD5E1; }");

    QPushButton *importBtn = new QPushButton(QStringLiteral("导入所选"), footerFrame);
    importBtn->setObjectName("scanImportConfirm");
    importBtn->setCursor(Qt::PointingHandCursor);
    importBtn->setFocusPolicy(Qt::NoFocus);
    importBtn->setMinimumWidth(96);
    importBtn->setDefault(true);
    importBtn->setStyleSheet(
        "#scanImportConfirm { background: #2463EB; color: #FFFFFF; "
        "border: 1px solid #2463EB; border-radius: 8px; padding: 9px 18px; "
        "font-size: 14px; font-weight: 700; }"
        "#scanImportConfirm:hover { background: #1D4ED8; }");

    QPushButton *finishBtn = new QPushButton(QStringLiteral("完成"), footerFrame);
    finishBtn->setObjectName("scanImportFinish");
    finishBtn->setCursor(Qt::PointingHandCursor);
    finishBtn->setMinimumWidth(96);
    finishBtn->setStyleSheet(QString(cancelBtn->styleSheet()).replace(
        QStringLiteral("scanImportCancel"), QStringLiteral("scanImportFinish")));
    footerLayout->addWidget(cancelBtn);
    footerLayout->addWidget(finishBtn);
    footerLayout->addWidget(importBtn);

    connect(selectAllBtn, &QPushButton::clicked, this, &ScanImportDialog::onSelectAllClicked);
    connect(resolveBtn, &QPushButton::clicked, this, &ScanImportDialog::onResolveClicked);
    connect(cancelBtn, &QPushButton::clicked, this, &ScanImportDialog::onCancelClicked);
    connect(finishBtn, &QPushButton::clicked, this, &ScanImportDialog::onFinishClicked);
    connect(importBtn, &QPushButton::clicked, this, &ScanImportDialog::onImportClicked);

    mainLayout->addWidget(footerFrame);
}

// ============================================================================
// 表格操作
// ============================================================================

void ScanImportDialog::clearTable()
{
    table_->setRowCount(0);
    scanItems_.clear();
}

void ScanImportDialog::populateTable(const QList<ScanImportItem> &items)
{
    clearTable();
    scanItems_ = items;

    table_->setRowCount(static_cast<int>(items.size()));

    for (int i = 0; i < items.size(); ++i) {
        const ScanImportItem &item = items.at(i);

        // 列 0：待导入项和待解决冲突均可勾选；已存在项无需再次导入。
        QTableWidgetItem *checkItem = new QTableWidgetItem();
        checkItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        if (item.status == QStringLiteral("待导入")) {
            checkItem->setCheckState(Qt::Checked);
        } else if (item.isConflict) {
            checkItem->setCheckState(Qt::Unchecked);
        } else {
            checkItem->setCheckState(Qt::Unchecked);
            checkItem->setFlags(checkItem->flags() & ~Qt::ItemIsEnabled);
        }
        table_->setItem(i, 0, checkItem);

        // 列 1：目录名
        QTableWidgetItem *nameItem = new QTableWidgetItem(item.folderName);
        table_->setItem(i, 1, nameItem);

        // 列 2：解析结果（编号 + 分类 + 名称）
        QString parsed;
        if (item.importType == QStringLiteral("参考资料")) {
            parsed = QStringLiteral("编号: %1\n名称: %2").arg(item.taskNo, item.name);
        } else {
            parsed = QStringLiteral("编号: %1\n分类: %2\n名称: %3")
                         .arg(item.taskNo,
                              item.category.isEmpty() ? QStringLiteral("无") : item.category,
                              item.name);
        }
        QTableWidgetItem *parsedItem = new QTableWidgetItem(parsed);
        table_->setItem(i, 2, parsedItem);

        // 列 3：导入类型
        QTableWidgetItem *typeItem = new QTableWidgetItem(item.importType);
        table_->setItem(i, 3, typeItem);

        // 列 4：状态
        QTableWidgetItem *statusItem = new QTableWidgetItem(item.status);
        if (item.isConflict) {
            statusItem->setText(QStringLiteral("待解决冲突"));
            statusItem->setForeground(QColor("#DC2626"));
            statusItem->setToolTip(item.conflictReason);
        } else if (item.status == QStringLiteral("跳过-已存在")) {
            statusItem->setForeground(QColor("#EAB308"));
        } else if (item.status == QStringLiteral("解析失败")) {
            statusItem->setForeground(QColor("#DC2626"));
        } else {
            statusItem->setForeground(QColor("#16A34A"));
        }
        table_->setItem(i, 4, statusItem);
    }
}

// ============================================================================
// 槽：扫描 / 全选 / 导入 / 取消
// ============================================================================

void ScanImportDialog::onScanClicked()
{
    const ScanImportData scan = ScanImportUseCase().scan();
    if (!scan.workDirConfigured) {
        StyledMessageBox::warning(this, QStringLiteral("提示"),
                             QStringLiteral("工作目录未设置，请先在设置中配置工作目录"));
        return;
    }

    // 调用扫描服务
    const QList<ScanImportItem> &items = scan.items;

    if (items.isEmpty()) {
        clearTable();
        StyledMessageBox::information(this, QStringLiteral("扫描结果"),
                                 QStringLiteral("没有待导入或待解决的目录"));
        return;
    }

    populateTable(items);

    int pendingCount = 0;
    for (const ScanImportItem &item : items) {
        if (item.status == QStringLiteral("待导入")) {
            ++pendingCount;
        }
    }
    hintLabel_->setText(QStringLiteral("扫描完成：共 %1 项，待导入 %2 项")
                            .arg(items.size()).arg(pendingCount));
    for (int i = 0; i < items.size(); ++i) {
        if (items.at(i).isConflict) {
            table_->selectRow(i);
            hintLabel_->setText(QStringLiteral("冲突原因：") + items.at(i).conflictReason);
            break;
        }
    }
}

void ScanImportDialog::onSelectAllClicked()
{
    if (table_->rowCount() == 0) {
        return;
    }

    // 检查当前是否全部勾选
    bool allChecked = true;
    for (int i = 0; i < table_->rowCount(); ++i) {
        QTableWidgetItem *item = table_->item(i, 0);
        if (item && (item->flags() & Qt::ItemIsEnabled) && item->checkState() != Qt::Checked) {
            allChecked = false;
            break;
        }
    }

    // 切换全选/全不选（仅对可勾选项生效）
    Qt::CheckState newState = allChecked ? Qt::Unchecked : Qt::Checked;
    for (int i = 0; i < table_->rowCount(); ++i) {
        QTableWidgetItem *item = table_->item(i, 0);
        if (item && (item->flags() & Qt::ItemIsEnabled)) {
            item->setCheckState(newState);
        }
    }
}

void ScanImportDialog::onImportClicked()
{
    if (scanItems_.isEmpty()) {
        StyledMessageBox::information(this, QStringLiteral("提示"),
                                 QStringLiteral("请先点击\"开始扫描\""));
        return;
    }

    // 收集勾选的待导入项和冲突项。
    QList<ScanImportItem> selectedItems;
    QStringList conflictPaths;
    for (int i = 0; i < table_->rowCount(); ++i) {
        QTableWidgetItem *checkItem = table_->item(i, 0);
        if (checkItem && checkItem->checkState() == Qt::Checked && i < scanItems_.size()) {
            if (scanItems_.at(i).isConflict)
                conflictPaths.append(scanItems_.at(i).folderPath);
            else
                selectedItems.append(scanItems_.at(i));
        }
    }

    if (selectedItems.isEmpty() && conflictPaths.isEmpty()) {
        bool hasPending = false;
        for (const ScanImportItem &item : scanItems_)
            if (item.status == QStringLiteral("待导入") || item.isConflict) hasPending = true;
        StyledMessageBox::information(this, QStringLiteral("提示"),
            hasPending ? QStringLiteral("请至少勾选一项进行导入")
                       : QStringLiteral("已无待处理项，请点击完成"));
        return;
    }

    ScanImportSummary result;
    if (!selectedItems.isEmpty())
        result = ScanImportUseCase().importSelected(selectedItems, scanItems_.size());

    for (const QString &path : conflictPaths) {
        // 前一项解决后会重新扫描，使用路径定位原来勾选的目录。
        int row = -1;
        for (int i = 0; i < scanItems_.size(); ++i) {
            if (scanItems_.at(i).folderPath == path && scanItems_.at(i).isConflict) {
                row = i;
                break;
            }
        }
        if (row < 0) continue;
        table_->setCurrentCell(row, 0);
        onResolveClicked();
    }

    // 发射信号
    if (!selectedItems.isEmpty())
        emit importCompleted(result.successCount, result.skipCount, result.failCount);

    // 显示结果反馈
    if (!selectedItems.isEmpty())
        StyledMessageBox::information(this, QStringLiteral("导入完成"),
                                 QStringLiteral("成功 %1 条、跳过 %2 条、失败 %3 条")
                                     .arg(result.successCount).arg(result.skipCount).arg(result.failCount));

    // 本轮导入结果保留在表格中供查看；下次扫描由服务过滤已导入项。
    if (conflictPaths.isEmpty()) {
    QStringList remainingPaths;
    for (const ScanImportItem &remaining : ScanImportUseCase().scan().items)
        remainingPaths.append(remaining.folderPath);
    for (int i = 0; i < scanItems_.size(); ++i) {
        bool wasSelected = false;
        for (const ScanImportItem &selected : selectedItems)
            if (selected.folderPath == scanItems_.at(i).folderPath) wasSelected = true;
        if (wasSelected && !remainingPaths.contains(scanItems_.at(i).folderPath)) {
            scanItems_[i].status = QStringLiteral("已导入");
            if (auto *check = table_->item(i, 0)) {
                check->setCheckState(Qt::Unchecked);
                check->setFlags(check->flags() & ~Qt::ItemIsEnabled);
            }
            if (auto *status = table_->item(i, 4)) status->setText(QStringLiteral("已导入"));
        }
    }
    }
}

void ScanImportDialog::onResolveClicked()
{
    const int row = table_->currentRow();
    if (row < 0 || row >= scanItems_.size() || !scanItems_.at(row).isConflict) {
        StyledMessageBox::information(this, QStringLiteral("选择冲突"),
            QStringLiteral("请先选中一条待解决冲突的任务。"));
        return;
    }
    const ScanImportItem item = scanItems_.at(row);
    if (item.importType == QStringLiteral("参考资料")) {
        ReferenceMaterialDao refDao(DatabaseManager::instance().database());
        auto existing = item.existingId > 0 ? refDao.findById(item.existingId)
            : std::optional<ReferenceMaterial>();
        bool hasPeer = false;
        for (const ScanImportItem &peer : scanItems_) {
            if (peer.importType == item.importType && peer.taskNo == item.taskNo
                && peer.folderPath != item.folderPath) { hasPeer = true; break; }
        }
        const bool sameFolder = existing && QDir::cleanPath(existing->folderPath)
            == QDir::cleanPath(item.folderPath);
        const bool metadataOnly = !existing && !hasPeer;
        const int action = sameFolder ? 1 : StyledMessageBox::choice(this,
            QStringLiteral("解决资料冲突"),
            QStringLiteral("%1\n%2\n\n保留并更名可建立独立资料；合并可逐项选择字段，附件同名时自动改名。")
                .arg(item.folderName, item.conflictReason),
            QStringLiteral("保留并更名"), metadataOnly ? QStringLiteral("合并信息")
                                                     : QStringLiteral("合并"));
        if (action == 0) return;
        if (action == 2 && !existing) {
            for (const ScanImportItem &peer : scanItems_) {
                if (peer.importType != item.importType || peer.taskNo != item.taskNo
                    || peer.folderPath == item.folderPath) continue;
                ReferenceMaterial preview;
                preview.refNo = peer.taskNo;
                preview.name = peer.name;
                if (const auto saved = ReferenceInfoFile::read(peer.folderPath)) {
                    if (saved->refNo == peer.taskNo && saved->name == peer.name)
                        preview = *saved;
                }
                existing = preview;
                break;
            }
        }
        const auto info = ReferenceInfoFile::read(item.folderPath);
        QDialog picker(this);
        picker.setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
        picker.setModal(true);
        picker.setMinimumSize(700, 450);
        picker.installEventFilter(new RoundedCornerFilter(12, &picker));
        picker.setStyleSheet("QDialog { background: #FFFFFF; border: 1px solid #DCE3EF; "
                             "border-radius: 12px; } QLabel { color: #0F172A; border: none; } "
                             "QComboBox { padding: 8px; border: 1px solid #CBD5E1; border-radius: 6px; } "
                             "QComboBox:disabled { color: #94A3B8; background: #F1F5F9; "
                             "border-color: #E2E8F0; } "
                             "QPushButton { background: #2463EB; color: #FFFFFF; "
                             "border-radius: 8px; padding: 9px 18px; }");
        auto *layout = new QVBoxLayout(&picker);
        layout->setContentsMargins(24, 20, 24, 20);
        auto *title = new QLabel(QStringLiteral("逐项选择资料字段"), &picker);
        title->setFont(QFont(QStringLiteral("MiSans"), 18));
        layout->addWidget(title);
        auto *hint = new QLabel(QStringLiteral("只需选择内容不一致的字段；灰色字段已一致，无需修改。"), &picker);
        layout->addWidget(hint);
        auto *form = new QFormLayout();
        QMap<QString, QComboBox *> combos;
        auto addField = [&](const QString &key, const QString &label,
                            const QString &oldValue, const QString &folderValue,
                            const QString &infoValue) {
            auto *box = new QComboBox(&picker);
            box->setObjectName(QStringLiteral("scanConflictField_") + key);
            const bool identityField = key == QStringLiteral("ref_no")
                || key == QStringLiteral("name");
            if (existing) addDistinctChoice(box, oldValue, QStringLiteral("existing"));
            if (identityField || (!existing && !info))
                addDistinctChoice(box, folderValue, QStringLiteral("folder"));
            if (info) addDistinctChoice(box, infoValue, QStringLiteral("info"));
            box->setCurrentIndex(existing && action == 2 ? 0 : box->count() - 1);
            box->setEnabled(box->count() > 1);
            box->setMinimumWidth(470);
            form->addRow(label, box);
            combos.insert(key, box);
        };
        if (action != 2 || !existing)
            addField(QStringLiteral("ref_no"), QStringLiteral("编号"),
                existing ? existing->refNo : item.taskNo, item.taskNo,
                info ? info->refNo : item.taskNo);
        addField(QStringLiteral("name"), QStringLiteral("资料名称"),
            existing ? existing->name : item.name, item.name,
            info ? info->name : item.name);
        addField(QStringLiteral("source"), QStringLiteral("来源"),
            existing ? existing->source : QString(), QString(),
            info ? info->source : QString());
        addField(QStringLiteral("tag"), QStringLiteral("标签"),
            existing ? existing->tag : QString(), QString(),
            info ? info->tag : QString());
        addField(QStringLiteral("description"), QStringLiteral("说明"),
            existing ? existing->description : QString(), QString(),
            info ? info->description : QString());
        layout->addLayout(form);
        auto *buttons = new QHBoxLayout();
        auto *cancel = new QPushButton(QStringLiteral("取消"), &picker);
        auto *apply = new QPushButton(QStringLiteral("应用选择"), &picker);
        buttons->addStretch(); buttons->addWidget(cancel); buttons->addWidget(apply);
        layout->addLayout(buttons);
        connect(cancel, &QPushButton::clicked, &picker, &QDialog::reject);
        connect(apply, &QPushButton::clicked, &picker, &QDialog::accept);
        if (picker.exec() != QDialog::Accepted) return;
        QMap<QString, QString> choices;
        for (auto it = combos.cbegin(); it != combos.cend(); ++it)
            choices.insert(it.key(), it.value()->currentData().toString());
        const TaskResult resolved = ScanMatchService().resolveReferenceConflict(item,
            action == 2, choices);
        if (!resolved.ok) {
            StyledMessageBox::warning(this, QStringLiteral("解决失败"), resolved.error);
            return;
        }
        emit importCompleted(1, 0, 0);
        populateTable(ScanImportUseCase().scan().items);
        return;
    }
    TaskDao dao(DatabaseManager::instance().database());
    auto existing = item.existingId > 0 ? dao.findById(item.existingId) : std::optional<Task>();
    const bool sameFolder = existing && QDir::cleanPath(existing->folderPath)
        == QDir::cleanPath(item.folderPath);
    bool hasPeer = false;
    for (const ScanImportItem &peer : scanItems_) {
        if (peer.importType == QStringLiteral("任务") && peer.taskNo == item.taskNo
            && peer.folderPath != item.folderPath) { hasPeer = true; break; }
    }
    const bool metadataOnly = !existing && !hasPeer;
    const int action = sameFolder ? 1 : StyledMessageBox::choice(this,
        QStringLiteral("解决任务冲突"),
        metadataOnly
            ? QStringLiteral("%1\n%2\n\n可按目录名保留并更名，或逐项合并目录名与 info 的字段。")
                .arg(item.folderName, item.conflictReason)
            : QStringLiteral("%1\n%2\n\n保留并更名会创建独立任务；合并会将附件复制到现有任务，同名附件自动改名，来源目录改名留存。")
                .arg(item.folderName, item.conflictReason),
        QStringLiteral("保留并更名"), metadataOnly ? QStringLiteral("合并信息") : QStringLiteral("合并"));
    if (action == 0) return;
    if (action == 2 && !existing) {
        for (const ScanImportItem &peer : scanItems_) {
            if (peer.importType != QStringLiteral("任务") || peer.taskNo != item.taskNo
                || peer.folderPath == item.folderPath) continue;
            Task preview;
            preview.taskNo = peer.taskNo;
            preview.title = peer.name;
            preview.category = peer.category;
            if (const auto saved = TaskInfoFile::read(peer.folderPath)) {
                if (saved->taskNo == peer.taskNo && saved->title == peer.name
                    && saved->category == peer.category)
                    preview = *saved;
            }
            existing = preview;
            break;
        }
    }
    const auto info = TaskInfoFile::read(item.folderPath);
    QDialog picker(this);
    picker.setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    picker.setModal(true);
    picker.setObjectName("scanConflictFields");
    picker.setMinimumSize(700, 560);
    picker.installEventFilter(new RoundedCornerFilter(12, &picker));
    picker.setStyleSheet(
        "QDialog { background: #FFFFFF; } QLabel { color: #0F172A; font-size: 13px; }"
        "QScrollArea { border: none; background: #FFFFFF; }"
        "QComboBox { color: #0F172A; background: #FFFFFF; border: 1px solid #CBD5E1; "
        "border-radius: 6px; padding: 7px; min-height: 24px; }"
        "QComboBox:disabled { color: #94A3B8; background: #F1F5F9; "
        "border-color: #E2E8F0; }"
        "QPushButton { background: #2463EB; color: #FFFFFF; border-radius: 8px; "
        "padding: 9px 18px; min-width: 90px; }");
    auto *layout = new QVBoxLayout(&picker);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(16);
    auto *header = new QFrame(&picker);
    header->installEventFilter(new WindowDragger(&picker));
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    auto *title = new QLabel(QStringLiteral("逐项选择任务字段"), header);
    title->setFont(QFont(QStringLiteral("MiSans"), 18));
    auto *close = new QPushButton(QStringLiteral("✕"), header);
    close->setFixedSize(32, 32);
    close->setStyleSheet("QPushButton { background: transparent; color: #94A3B8; border: none; }"
                         "QPushButton:hover { background: #F5F7FA; color: #0F172A; }");
    headerLayout->addWidget(title);
    headerLayout->addStretch();
    headerLayout->addWidget(close);
    connect(close, &QPushButton::clicked, &picker, &QDialog::reject);
    layout->addWidget(header);
    auto *hint = new QLabel(QStringLiteral("只需选择内容不一致的字段；灰色字段已一致，无需修改。"), &picker);
    hint->setWordWrap(true);
    layout->addWidget(hint);
    auto *scroll = new QScrollArea(&picker);
    scroll->setWidgetResizable(true);
    auto *content = new QWidget(scroll);
    auto *form = new QFormLayout(content);
    QMap<QString, QComboBox *> combos;
    auto addField = [&](const QString &key, const QString &label,
                        const QString &oldValue, const QString &folderValue,
                        const QString &infoValue) {
        auto *box = new QComboBox(content);
        box->setObjectName(QStringLiteral("scanConflictField_") + key);
        const bool identityField = key == QStringLiteral("task_no")
            || key == QStringLiteral("title") || key == QStringLiteral("category");
        if (existing) addDistinctChoice(box, oldValue, QStringLiteral("existing"));
        if (identityField || (!existing && !info))
            addDistinctChoice(box, folderValue, QStringLiteral("folder"));
        if (info) addDistinctChoice(box, infoValue, QStringLiteral("info"));
        box->setCurrentIndex(existing && action == 2 ? 0
            : metadataOnly && action == 1 && identityField ? 0 : box->count() - 1);
        box->setEnabled(box->count() > 1);
        box->setMinimumWidth(480);
        form->addRow(label, box);
        combos.insert(key, box);
    };
    if (action != 2 || !existing)
        addField(QStringLiteral("task_no"), QStringLiteral("编号"),
                 existing ? existing->taskNo : item.taskNo, item.taskNo,
                 info ? info->taskNo : item.taskNo);
    addField(QStringLiteral("title"), QStringLiteral("任务名称"),
             existing ? existing->title : item.name, item.name,
             info ? info->title : item.name);
    addField(QStringLiteral("category"), QStringLiteral("分类"),
             existing ? existing->category : item.category, item.category,
             info ? info->category : item.category);
    addField(QStringLiteral("source"), QStringLiteral("来源"),
             existing ? existing->source : QString(), QString(),
             info ? info->source : QString());
    addField(QStringLiteral("tag"), QStringLiteral("标签"),
             existing ? existing->tag : QString(), QString(),
             info ? info->tag : QString());
    addField(QStringLiteral("handler"), QStringLiteral("办理人"),
             existing ? existing->handler : QString(),
             item.sourceDir == FileService::kDirOtherHandler ? QStringLiteral("非本人")
                 : QStringLiteral("本人"),
             info ? info->handler : QString());
    addField(QStringLiteral("deadline"), QStringLiteral("截止日期"),
             existing && existing->deadline.isValid() ? existing->deadline.toString(Qt::ISODate) : QString(),
             QString(), info && info->deadline.isValid() ? info->deadline.toString(Qt::ISODate) : QString());
    addField(QStringLiteral("description"), QStringLiteral("说明"),
             existing ? existing->description : QString(), QString(),
             info ? info->description : QString());
    addField(QStringLiteral("is_key_task"), QStringLiteral("重点任务"),
             existing && existing->isKeyTask ? QStringLiteral("是") : QStringLiteral("否"),
             QStringLiteral("否"), info && info->isKeyTask ? QStringLiteral("是") : QStringLiteral("否"));
    scroll->setWidget(content);
    layout->addWidget(scroll);
    auto *buttons = new QHBoxLayout();
    auto *cancel = new QPushButton(QStringLiteral("取消"), &picker);
    cancel->setStyleSheet("QPushButton { background: #FFFFFF; color: #475569; border: 1px solid #CBD5E1; "
                          "border-radius: 8px; padding: 9px 18px; min-width: 90px; }");
    auto *apply = new QPushButton(QStringLiteral("应用选择"), &picker);
    buttons->addStretch();
    buttons->addWidget(cancel);
    buttons->addWidget(apply);
    layout->addLayout(buttons);
    connect(cancel, &QPushButton::clicked, &picker, &QDialog::reject);
    connect(apply, &QPushButton::clicked, &picker, &QDialog::accept);
    if (picker.exec() != QDialog::Accepted) return;
    QMap<QString, QString> choices;
    for (auto it = combos.cbegin(); it != combos.cend(); ++it)
        choices.insert(it.key(), it.value()->currentData().toString());
    const TaskResult result = ScanMatchService().resolveConflict(item, action == 2, choices);
    if (!result.ok) {
        StyledMessageBox::warning(this, QStringLiteral("解决失败"), result.error);
        return;
    }
    emit importCompleted(1, 0, 0);
    populateTable(ScanImportUseCase().scan().items);
}

void ScanImportDialog::onCancelClicked()
{
    reject();
}

void ScanImportDialog::onFinishClicked()
{
    accept();
}
