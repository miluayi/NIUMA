/**
 * @file TitleBar.h
 * @brief 自定义标题栏组件
 *
 * 高度 52px，包含 NIUMA 标识、导航、搜索、新建和窗口控制按钮。
 * 支持鼠标拖拽移动窗口、双击最大化/还原，以及后退/前进状态同步。
 *
 * 依赖：Qt5 Widgets
 * 文档：PRD 14.1、qt_implementation_guide.md 3.1、coding.md 7.1
 */
#pragma once

#include <QWidget>

class QLabel;
class QLineEdit;
class QPushButton;
class QMouseEvent;
class QString;

class TitleBar : public QWidget {
    Q_OBJECT

public:
    explicit TitleBar(QWidget *parent = nullptr);
    ~TitleBar() override;

signals:
    void minimizeClicked();
    void maximizeClicked();
    void closeClicked();
    void navigateBackClicked();
    void navigateForwardClicked();
    void refreshClicked();
    void createTaskClicked();
    void searchSubmitted(const QString &text);

public:
    void setNavigationEnabled(bool canGoBack, bool canGoForward);
    void setMaximized(bool maximized);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    QLabel *titleLabel_;
    QPushButton *btnMinimize_;
    QPushButton *btnMaximize_;
    QPushButton *btnClose_;
    QPushButton *btnBack_;
    QPushButton *btnForward_;
    QPushButton *btnRefresh_;
    QPushButton *btnCreate_;
    QLineEdit *searchEdit_;
    QPoint dragPos_;
    bool draggingTitleBar_ = false;

    bool isInteractiveArea(const QPoint &position) const;
};
