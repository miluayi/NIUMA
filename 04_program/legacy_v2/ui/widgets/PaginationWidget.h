/**
 * @file PaginationWidget.h
 * @brief 分页控件（最小实现），供 TaskListView / ReferenceView 使用
 *
 * 布局：左侧 infoLabel（"显示 X-Y 条，共 Z 条"）+ 右侧页码按钮
 * 页码按钮当前页高亮（蓝色 #2463EB 背景 + 白字），页码过多时显示 "..."
 *
 * 依赖：Qt5 Widgets
 * 文档：task.md T1.9、qt_implementation_guide.md Prompt 3、coding.md 第十二章
 */
#pragma once

#include <QWidget>
#include <QList>

class QPushButton;
class QLabel;

class PaginationWidget : public QWidget {
    Q_OBJECT

public:
    explicit PaginationWidget(QWidget *parent = nullptr);

    void setTotal(int total);
    void setPage(int page);
    void setPageSize(int pageSize);

    int currentPage() const { return currentPage_; }
    int totalPages() const { return totalPages_; }
    int total() const { return total_; }

    void updateDisplay();

    /// 隐藏/显示左侧信息标签"显示 X-Y 条，共 Z 条"
    void setInfoVisible(bool visible);
    /// 获取当前信息标签文本
    QString infoText() const;

signals:
    void pageChanged(int page);

private slots:
    void onPageClicked();

private:
    int currentPage_ = 1;
    int pageSize_ = 10;
    int total_ = 0;
    int totalPages_ = 0;

    QLabel *infoLabel_;
    QList<QPushButton*> pageButtons_;

    void rebuildButtons();
    void updateInfo();
};
