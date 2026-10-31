/**
 * @file DragDropArea.cpp
 * @brief 拖拽文件区域组件实现
 *
 * 支持三种文件导入方式：拖拽、点击选择、Ctrl+V 粘贴。
 * 无文件时显示提示语；有文件时在区域内显示文件名列表，每项可单独删除。
 *
 * 依赖：ui/widgets/DragDropArea.h、ui/ColorPalette.h、ui/Spacing.h
 * 文档：qt_implementation_guide.md 第四节、coding.md 9.1、task.md T3.3
 */

#include "DragDropArea.h"

#include <QApplication>
#include <QClipboard>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMimeData>
#include <QMouseEvent>
#include <QPushButton>
#include <QScrollArea>
#include <QUrl>
#include <QVBoxLayout>

#include "ColorPalette.h"
#include "Spacing.h"

// ============================================================================
// DragDropArea 构造
// ============================================================================

DragDropArea::DragDropArea(QWidget *parent)
    : QWidget(parent)
{
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);  // 接收键盘事件以支持 Ctrl+V 粘贴

    // 主布局：单个带虚线边框的容器
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ---- 容器（虚线边框） ----
    // 使用 QWidget 作为容器，内部根据状态切换提示语/文件列表
    // The parent form controls the drop area's height; keep the hint centered without forcing a tall minimum.
    dropHintLabel_ = new QLabel(this);
    dropHintLabel_->setMinimumHeight(0);
    dropHintLabel_->setAlignment(Qt::AlignCenter);
    dropHintLabel_->setCursor(Qt::PointingHandCursor);
    dropHintLabel_->installEventFilter(this);  // 监听点击事件
    dropHintLabel_->setText(QStringLiteral(
        "拖拽文件到此处，或点击选择文件\n（支持 Ctrl+V 粘贴）"));
    dropHintLabel_->setWordWrap(true);
    mainLayout->addWidget(dropHintLabel_);

    // ---- 文件列表滚动区域（超过6条时滚动） ----
    filesScroll_ = new QScrollArea(this);
    filesScroll_->setWidgetResizable(true);
    filesScroll_->setFrameShape(QFrame::NoFrame);  // 无原生边框
    filesScroll_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    filesScroll_->setStyleSheet(
        "QScrollArea {"
        "  border: 2px dashed #CBD5E1;"
        "  border-radius: 8px;"
        "  background: #FFFFFF;"
        "}"
        "QScrollBar:vertical {"
        "  background: #F1F5F9;"
        "  width: 8px;"
        "  border-radius: 4px;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: #CBD5E1;"
        "  border-radius: 4px;"
        "  min-height: 20px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "  background: #94A3B8;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "  height: 0px;"
        "}");
    // 固定高度：6条文件项约 6×30+20=200px，6条以内高度不变，超过6条滚动
    filesScroll_->setMinimumHeight(0);
    filesScroll_->setMaximumHeight(200);
    filesScroll_->hide();  // 默认无文件时隐藏

    // ---- 文件列表容器（放在滚动区域内） ----
    filesContainer_ = new QFrame();
    filesContainer_->setCursor(Qt::PointingHandCursor);
    filesContainer_->installEventFilter(this);  // 监听点击事件
    filesContainer_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    filesContainer_->setStyleSheet("QFrame { background: transparent; }");
    filesLayout_ = new QVBoxLayout(filesContainer_);
    filesLayout_->setContentsMargins(10, 10, 10, 10);
    filesLayout_->setSpacing(4);

    filesScroll_->setWidget(filesContainer_);
    mainLayout->addWidget(filesScroll_);

    // 控件都已创建，现在应用样式（updateHintStyle 会访问 filesContainer_）
    updateHintStyle();
}

// ============================================================================
// 拖拽事件
// ============================================================================

void DragDropArea::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
        dragHovered_ = true;
        updateHintStyle();
    } else {
        event->ignore();
    }
}

void DragDropArea::dragLeaveEvent(QDragLeaveEvent *event)
{
    Q_UNUSED(event)
    dragHovered_ = false;
    updateHintStyle();
}

void DragDropArea::dropEvent(QDropEvent *event)
{
    dragHovered_ = false;
    updateHintStyle();

    QStringList newFiles;
    const QList<QUrl> urls = event->mimeData()->urls();
    for (const QUrl &url : urls) {
        if (url.isLocalFile()) {
            newFiles << url.toLocalFile();
        }
    }

    if (!newFiles.isEmpty()) {
        filePaths_ << newFiles;
        rebuildFileList();
        emit filesChanged(filePaths_);
    }
    event->acceptProposedAction();
}

// ============================================================================
// 事件过滤器（处理提示区和文件列表区的点击）
// ============================================================================

bool DragDropArea::eventFilter(QObject *watched, QEvent *event)
{
    // 点击提示区或文件列表区空白处打开文件对话框
    if ((watched == dropHintLabel_ || watched == filesContainer_)
        && event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            openFileDialog();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

// ============================================================================
// 键盘事件
// ============================================================================

void DragDropArea::keyPressEvent(QKeyEvent *event)
{
    // Ctrl+V 触发剪贴板粘贴
    if (event->key() == Qt::Key_V && event->modifiers() == Qt::ControlModifier) {
        pasteFromClipboard();
        return;
    }
    QWidget::keyPressEvent(event);
}

// ============================================================================
// 公共方法
// ============================================================================

void DragDropArea::clear()
{
    filePaths_.clear();
    rebuildFileList();
    emit filesChanged(filePaths_);
}

bool DragDropArea::pasteFiles(const QStringList &files)
{
    if (files.isEmpty()) {
        return false;
    }
    filePaths_ << files;
    rebuildFileList();
    emit filesChanged(filePaths_);
    return true;
}

// ============================================================================
// 私有方法
// ============================================================================

void DragDropArea::openFileDialog()
{
    QStringList files = QFileDialog::getOpenFileNames(
        this, QStringLiteral("选择文件"), QString(),
        QStringLiteral("所有文件 (*.*)"));
    if (!files.isEmpty()) {
        filePaths_ << files;
        rebuildFileList();
        emit filesChanged(filePaths_);
    }
}

void DragDropArea::pasteFromClipboard()
{
    const QClipboard *clipboard = QApplication::clipboard();
    const QMimeData *mimeData = clipboard->mimeData();
    if (mimeData && mimeData->hasUrls()) {
        QStringList newFiles;
        for (const QUrl &url : mimeData->urls()) {
            if (url.isLocalFile()) {
                newFiles << url.toLocalFile();
            }
        }
        if (!newFiles.isEmpty()) {
            filePaths_ << newFiles;
            rebuildFileList();
            emit filesChanged(filePaths_);
        }
    }
}

void DragDropArea::rebuildFileList()
{
    // 清空旧的文件项 widget
    QLayoutItem *item;
    while ((item = filesLayout_->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }

    if (filePaths_.isEmpty()) {
        // 无文件：显示提示语，隐藏文件列表
        dropHintLabel_->show();
        filesScroll_->hide();
        return;
    }

    // 有文件：隐藏提示语，显示文件列表
    dropHintLabel_->hide();
    filesScroll_->show();
    filesContainer_->show();

    // 添加文件项（仅文件名 + 删除按钮）
    for (int i = 0; i < filePaths_.size(); ++i) {
        const QString &filePath = filePaths_[i];
        QFileInfo fi(filePath);

        QWidget *row = new QWidget(filesContainer_);
        row->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        QHBoxLayout *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(8, 4, 8, 4);
        rowLayout->setSpacing(8);

        // 文件名
        QLabel *nameLabel = new QLabel(fi.fileName(), row);
        nameLabel->setToolTip(filePath);  // 完整路径作为 tooltip
        nameLabel->setStyleSheet("color:#0F172A; font-size:13px; border:none; background:transparent;");
        rowLayout->addWidget(nameLabel, 1);

        // 删除按钮
        QPushButton *delBtn = new QPushButton(QStringLiteral("\u2715"), row);
        delBtn->setObjectName("attachmentRemoveButton");
        delBtn->setFixedSize(22, 22);
        delBtn->setCursor(Qt::PointingHandCursor);
        delBtn->setFocusPolicy(Qt::NoFocus);
        delBtn->setToolTip(QStringLiteral("移除此文件"));
        delBtn->setStyleSheet(
            "QPushButton {"
            "  border:none;"
            "  background:transparent;"
            "  color:#DC2626;"
            "  font-size:12px;"
            "  border-radius:4px;"
            "}"
            "QPushButton:hover {"
            "  background:#FEF2F2;"
            "  color:#DC2626;"
            "}");
        rowLayout->addWidget(delBtn);

        // 点击删除按钮移除该文件
        connect(delBtn, &QPushButton::clicked, this, [this, i]() {
            if (i >= 0 && i < filePaths_.size()) {
                filePaths_.removeAt(i);
                rebuildFileList();
                emit filesChanged(filePaths_);
            }
        });

        filesLayout_->addWidget(row);
    }

    // 触发布局重计算
    filesContainer_->adjustSize();
    filesScroll_->updateGeometry();
}

void DragDropArea::updateHintStyle()
{
    // 根据拖拽悬停状态切换边框颜色
    QString borderColor = dragHovered_ ? QStringLiteral("#2463EB") : QStringLiteral("#CBD5E1");
    QString textColor = dragHovered_ ? QStringLiteral("#2463EB") : QStringLiteral("#94A3B8");

    dropHintLabel_->setStyleSheet(QStringLiteral(
        "QLabel {"
        "  border: 2px dashed %1;"
        "  border-radius: 8px;"
        "  background: #FFFFFF;"
        "  color: %2;"
        "  font-size: 14px;"
        "  font-weight: 400;"
        "}"
    ).arg(borderColor, textColor));

    // 文件列表滚动区域同步边框颜色
    filesScroll_->setStyleSheet(QStringLiteral(
        "QScrollArea {"
        "  border: 2px dashed %1;"
        "  border-radius: 8px;"
        "  background: #FFFFFF;"
        "}"
        "QScrollBar:vertical {"
        "  background: #F1F5F9;"
        "  width: 8px;"
        "  border-radius: 4px;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: #CBD5E1;"
        "  border-radius: 4px;"
        "  min-height: 20px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "  background: #94A3B8;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "  height: 0px;"
        "}"
    ).arg(borderColor));
}
