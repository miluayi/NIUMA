/**
 * @file TaskEditDialog.h
 * @brief 任务编辑弹窗（T2.5）
 *
 * 无边框模态对话框，720px 宽，自适应高度。
 * 预填任务数据，字段：任务编号（只读）/ 任务名称（必填）/ 分类 / 来源 / 事项标签 /
 *       办理人 / 截止日期 / 任务标记（常规/重点）/ 任务说明
 *
 * 交互：Enter 保存，Esc 取消，任务名称必填验证
 * 不可变字段：任务编号、目录结构、状态（状态由 updateTask 自动处理办理人变更联动）
 *
 * 依赖：Qt5 Widgets、application/task、models/Task、ui/ColorPalette、ui/Spacing、ui/Radius
 * 文档：PRD 5.3、qt_implementation_guide.md 3.7、coding.md 第十一节（交互规范）、task.md T2.5
 *
 * 关键约束：
 *  - 任务编号只读（QLineEdit readOnly + 灰色背景），不可修改
 *  - 不显示目录结构选项（创建时确定不可修改）
 *  - 任务标记单选框（常规/重点）可变更
 *  - 保留 id/taskNo/status/hasSubdirs/folderPath 等不可变字段
 *  - 通过 application 命令更新，办理人变更联动由既有 Service 层处理
 */
#pragma once

#include <QDialog>

#include "models/Task.h"

class QLineEdit;
class QComboBox;
class QDateEdit;
class QRadioButton;
class QTextEdit;

class TaskEditDialog : public QDialog {
    Q_OBJECT

public:
    explicit TaskEditDialog(int taskId, QWidget *parent = nullptr);

signals:
    void taskUpdated();  // 保存成功后发射

private slots:
    void onSaveClicked();
    void onCancelClicked();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    // 控件指针
    QLineEdit *taskNoEdit_;        // 任务编号（只读）
    QLineEdit *titleEdit_;         // 任务名称（必填）
    QComboBox *categoryCombo_;     // 分类 催/查/报/办
    QComboBox *sourceCombo_;       // 来源（可编辑 + 自动补全）
    QComboBox *tagCombo_;          // 事项标签（可编辑 + 自动补全）
    QComboBox *handlerCombo_;      // 办理人（可编辑，默认"本人"）
    QDateEdit *deadlineEdit_;      // 截止日期
    QRadioButton *normalTaskRadio_;// 常规任务
    QRadioButton *keyTaskRadio_;   // 重点任务
    QTextEdit *descEdit_;          // 任务说明

    // 数据
    int taskId_;          // 构造传入的任务 ID
    Task originalTask_;   // 原始任务数据，用于保留不可变字段

    // 方法
    void setupUI();
    void loadTaskData();  // 读取任务数据并预填
    void loadFormOptions();
    void ensureDictEntry();
    bool validateForm();  // 表单验证（任务名称必填）
    void accept() override;  // 重写 accept，调用 onSaveClicked
};
