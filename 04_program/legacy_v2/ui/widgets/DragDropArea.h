/**
 * @file DragDropArea.h
 * @brief 拖拽文件区域组件
 *
 * 提供三种文件导入方式：拖拽、点击选择、Ctrl+V 粘贴。
 * 选择文件后在下方显示文件名列表，每个文件可单独删除。
 *
 * 依赖：Qt5 Widgets（QPainter、QDragEnterEvent、QDropEvent、QFileDialog、QKeyEvent）
 * 文档：qt_implementation_guide.md 第四节（DragDropArea）、coding.md 9.1、task.md T3.3
 */
#pragma once

#include <QWidget>
#include <QStringList>

class QLabel;
class QFrame;
class QVBoxLayout;
class QScrollArea;

/**
 * @brief 拖拽文件区域，支持拖拽、点击选择和 Ctrl+V 粘贴
 *
 * 上方为虚线边框提示区，下方为已选文件列表（每项含文件名和删除按钮）。
 * 选择文件后发射 filesChanged 信号。
 */
class DragDropArea : public QWidget {
    Q_OBJECT

public:
    explicit DragDropArea(QWidget *parent = nullptr);

    /** @brief 获取已选择的文件路径列表 */
    QStringList filePaths() const { return filePaths_; }

    /** @brief 清空已选文件列表 */
    void clear();

    /** @brief 外部调用粘贴文件（返回是否实际添加了文件） */
    bool pasteFiles(const QStringList &files);

signals:
    /** @brief 文件列表变化时发射 */
    void filesChanged(const QStringList &files);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    QStringList filePaths_;   // 已选文件路径
    bool dragHovered_ = false; // 拖拽悬停状态

    QLabel *dropHintLabel_;       // 上方提示区
    QScrollArea *filesScroll_;    // 文件列表滚动区域（超过6条时滚动）
    QFrame *filesContainer_;      // 下方文件列表容器
    QVBoxLayout *filesLayout_;    // 文件列表布局

    void openFileDialog();
    void pasteFromClipboard();
    void rebuildFileList();       // 根据 filePaths_ 重建文件列表 UI
    void updateHintStyle();       // 更新提示区样式（hover 态）
};
