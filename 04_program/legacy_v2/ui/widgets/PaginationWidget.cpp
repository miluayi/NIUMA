/**
 * @file PaginationWidget.cpp
 * @brief 分页控件实现
 *
 * 页码按钮策略：
 *   - 总页数 <= 7：显示全部页码
 *   - 总页数 > 7：显示首尾页 + 当前页前后各 1 页 + "..."
 *
 * 依赖：ui/widgets/PaginationWidget.h
 * 文档：task.md T1.9、qt_implementation_guide.md Prompt 3、coding.md 第十二章
 */
#include "PaginationWidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVariant>

// ============================================================================
// 构造
// ============================================================================

PaginationWidget::PaginationWidget(QWidget *parent)
    : QWidget(parent)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 8, 12, 8);
    layout->setSpacing(8);

    // 左侧：信息标签 "显示 X-Y 条，共 Z 条"
    infoLabel_ = new QLabel(this);
    infoLabel_->setStyleSheet(
        "color:#94A3B8;"
        "font-size:14px;"
    );
    layout->addWidget(infoLabel_);

    // 中间弹性间隔（justify-content: space-between）
    layout->addStretch();

    // 右侧：页码按钮容器（初始为空，updateDisplay 时动态创建）
    // 页码按钮通过 rebuildButtons() 动态创建并添加到布局

    setStyleSheet(
        "QPushButton {"
        "  min-width:30px;"
        "  max-width:30px;"
        "  min-height:30px;"
        "  max-height:30px;"
        "  border:1px solid #E2E8F0;"
        "  border-radius:6px;"
        "  background:#FFFFFF;"
        "  color:#475569;"
        "  font-size:14px;"
        "  padding:0;"
        "}"
        "QPushButton:hover {"
        "  background:#F5F7FA;"
        "  border-color:#CBD5E1;"
        "}"
        "QPushButton#active {"
        "  background:#2463EB;"
        "  border:1px solid #2463EB;"
        "  color:#FFFFFF;"
        "  font-weight:700;"
        "}"
        "QPushButton#ellipsis {"
        "  border:none;"
        "  background:transparent;"
        "  color:#94A3B8;"
        "}"
        "QPushButton#ellipsis:hover {"
        "  background:transparent;"
        "}"
    );

    updateDisplay();
}

// ============================================================================
// Setter
// ============================================================================

void PaginationWidget::setTotal(int total)
{
    total_ = total;
    if (total_ < 0) {
        total_ = 0;
    }
    totalPages_ = (pageSize_ > 0) ? (total_ + pageSize_ - 1) / pageSize_ : 0;
    if (totalPages_ < 1) {
        totalPages_ = 1;
    }
    if (currentPage_ > totalPages_) {
        currentPage_ = totalPages_;
    }
    if (currentPage_ < 1) {
        currentPage_ = 1;
    }
}

void PaginationWidget::setPage(int page)
{
    if (page < 1) {
        page = 1;
    }
    if (page > totalPages_ && totalPages_ > 0) {
        page = totalPages_;
    }
    currentPage_ = page;
}

void PaginationWidget::setPageSize(int pageSize)
{
    if (pageSize <= 0) {
        pageSize = 10;
    }
    pageSize_ = pageSize;
    setTotal(total_);
}

// ============================================================================
// 刷新显示
// ============================================================================

void PaginationWidget::updateDisplay()
{
    updateInfo();
    rebuildButtons();
}

void PaginationWidget::setInfoVisible(bool visible)
{
    infoLabel_->setVisible(visible);
}

QString PaginationWidget::infoText() const
{
    return infoLabel_->text();
}

// ============================================================================
// 私有：更新信息标签
// ============================================================================

void PaginationWidget::updateInfo()
{
    if (total_ <= 0) {
        infoLabel_->setText(QStringLiteral("\u663E\u793A 0-0 \u6761\uFF0C\u5171 0 \u6761"));
        return;
    }

    int from = (currentPage_ - 1) * pageSize_ + 1;
    int to = currentPage_ * pageSize_;
    if (to > total_) {
        to = total_;
    }
    // "显示 X-Y 条，共 Z 条"
    infoLabel_->setText(QStringLiteral("\u663E\u793A %1-%2 \u6761\uFF0C\u5171 %3 \u6761")
                            .arg(from)
                            .arg(to)
                            .arg(total_));
}

// ============================================================================
// 私有：重建页码按钮
// ============================================================================

void PaginationWidget::rebuildButtons()
{
    // 删除旧按钮
    for (QPushButton *btn : pageButtons_) {
        layout()->removeWidget(btn);
        btn->deleteLater();
    }
    pageButtons_.clear();

    // 总页数 <= 1 时不显示页码按钮
    if (totalPages_ <= 1) {
        return;
    }

    // 确定要显示的页码列表
    QList<int> pages;
    bool showEllipsisBefore = false;
    bool showEllipsisAfter = false;

    if (totalPages_ <= 7) {
        // 总页数 <= 7：全部显示
        for (int i = 1; i <= totalPages_; ++i) {
            pages << i;
        }
    } else {
        // 总页数 > 7：首尾 + 当前页前后各 1 页 + "..."
        pages << 1;

        int startMid = currentPage_ - 1;
        int endMid = currentPage_ + 1;

        if (startMid > 2) {
            showEllipsisBefore = true;
        } else {
            // 当前页靠前，显示 2..currentPage+1
            startMid = 2;
        }

        if (endMid < totalPages_ - 1) {
            showEllipsisAfter = true;
        } else {
            // 当前页靠后，显示 totalPages-1..totalPages-1
            endMid = totalPages_ - 1;
        }

        if (showEllipsisBefore) {
            // 添加 "..." 占位
        } else {
            for (int i = startMid; i < currentPage_; ++i) {
                pages << i;
            }
        }

        pages << currentPage_;

        if (showEllipsisAfter) {
            for (int i = currentPage_ + 1; i <= endMid; ++i) {
                pages << i;
            }
        } else {
            for (int i = currentPage_ + 1; i <= endMid; ++i) {
                pages << i;
            }
        }

        if (totalPages_ > 1) {
            pages << totalPages_;
        }
    }

    // 构建按钮序列（含 "..."）
    QList<QPair<QString, int>> buttonSpecs;  // (text, page)，page=-1 表示 "..."

    if (totalPages_ <= 7) {
        for (int p : pages) {
            buttonSpecs << qMakePair(QString::number(p), p);
        }
    } else {
        buttonSpecs << qMakePair(QString("1"), 1);

        if (showEllipsisBefore) {
            buttonSpecs << qMakePair(QStringLiteral("\u22EF"), -1);  // "..."
        }

        // 中间页码
        int midStart = showEllipsisBefore ? currentPage_ - 1 : 2;
        int midEnd = showEllipsisAfter ? currentPage_ + 1 : totalPages_ - 1;
        for (int i = midStart; i <= midEnd; ++i) {
            if (i > 1 && i < totalPages_) {
                buttonSpecs << qMakePair(QString::number(i), i);
            }
        }

        if (showEllipsisAfter) {
            buttonSpecs << qMakePair(QStringLiteral("\u22EF"), -1);  // "..."
        }

        buttonSpecs << qMakePair(QString::number(totalPages_), totalPages_);
    }

    // 创建按钮
    QHBoxLayout *hLayout = qobject_cast<QHBoxLayout*>(layout());
    for (const auto &spec : buttonSpecs) {
        QPushButton *btn = new QPushButton(spec.first, this);
        btn->setCursor(Qt::PointingHandCursor);
        if (spec.second == -1) {
            btn->setObjectName("ellipsis");
            btn->setEnabled(false);
        } else if (spec.second == currentPage_) {
            btn->setObjectName("active");
            btn->setEnabled(false);
        } else {
            btn->setProperty("page", QVariant(spec.second));
            connect(btn, &QPushButton::clicked, this, &PaginationWidget::onPageClicked);
        }
        hLayout->addWidget(btn);
        pageButtons_.append(btn);
    }
}

// ============================================================================
// 私有槽：页码按钮点击
// ============================================================================

void PaginationWidget::onPageClicked()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) {
        return;
    }
    bool ok = false;
    int page = btn->property("page").toInt(&ok);
    if (!ok || page < 1 || page > totalPages_) {
        return;
    }
    if (page == currentPage_) {
        return;
    }
    currentPage_ = page;
    emit pageChanged(page);
}
