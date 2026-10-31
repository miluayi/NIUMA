/**
 * @file StyledMessageBox.h
 * @brief 软件UI风格的确认弹窗（替代QMessageBox）
 *
 * 提供与软件整体风格统一的确认/提示弹窗，无边框圆角，自定义按钮样式。
 *
 * 用法：
 *   StyledMessageBox::question(parent, "确认删除", "确定要删除吗？");
 *   StyledMessageBox::information(parent, "提示", "操作成功");
 *   StyledMessageBox::warning(parent, "警告", "请输入任务名称");
 *
 * 依赖：Qt5 Widgets
 */
#pragma once

#include <QDialog>

class QLabel;
class QPushButton;

class StyledMessageBox : public QDialog {
    Q_OBJECT
public:
    enum StandardButton { No = 0, Yes = 1, Ok = Yes, Cancel = No };

    /// 确认弹窗（是/否），返回 Yes 或 No
    static int question(QWidget *parent, const QString &title, const QString &text,
                        const QString &yesText = QStringLiteral("确认"),
                        const QString &noText = QStringLiteral("取消"));

    /// 三种结果：0 取消，1 主选项，2 次选项。
    static int choice(QWidget *parent, const QString &title, const QString &text,
                      const QString &primaryText, const QString &secondaryText);

    /// 信息提示弹窗（仅"我知道了"按钮）
    static void information(QWidget *parent, const QString &title, const QString &text);

    /// 警告弹窗（仅"我知道了"按钮，红色标题）
    static void warning(QWidget *parent, const QString &title, const QString &text);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    explicit StyledMessageBox(QWidget *parent = nullptr);
    void setupUI(const QString &title, const QString &text, bool isWarning,
                 bool twoButtons, const QString &yesText, const QString &noText);

    QLabel *textLabel_ = nullptr;
    QPushButton *yesBtn_ = nullptr;
    QPushButton *noBtn_ = nullptr;
};
