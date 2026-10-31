/**
 * @file TaskCreateDialog.h
 * @brief 任务创建弹窗（T1.8）
 *
 * 无边框模态对话框，720px 宽，自适应高度。
 * 字段：任务名称（必填）/ 分类 / 来源 / 事项标签 / 办理人 / 截止日期 /
 *       任务标记 / 目录结构 / 任务说明 / 任务资料（DragDropArea）
 *
 * 交互：Enter 创建，Esc 取消，任务名称必填验证
 * 状态规则：handler=="本人" → status="进行中"，否则 → status="他人办理"
 *
 * 依赖：Qt5 Widgets、application/task、ui/widgets/DragDropArea
 * 文档：PRD 5.3、qt_implementation_guide.md 3.7、coding.md 第十一节（交互规范）、task.md T1.8
 */
#pragma once

#include <QDate>
#include <QDialog>
#include <QStringList>

class QLineEdit;
class QComboBox;
class QDateEdit;
class QRadioButton;
class QTextEdit;
class QLabel;
class DragDropArea;

class TaskCreateDialog : public QDialog {
    Q_OBJECT

public:
    explicit TaskCreateDialog(QWidget *parent = nullptr);

    // ---- 获取表单数据 ----
    QString title() const;
    QString category() const;
    QString source() const;
    QString tag() const;
    QString handler() const;
    QDate deadline() const;
    bool isKeyTask() const;
    bool hasSubdirs() const;
    QString description() const;
    QStringList sourceFiles() const;  // 拖拽/选择的文件路径列表

signals:
    void taskCreated(const QString &taskNo);  // 创建成功后发射

private slots:
    void onCreateClicked();
    void onCancelClicked();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    // 控件指针
    QLineEdit *titleEdit_;
    QComboBox *categoryCombo_;
    QComboBox *sourceCombo_;
    QComboBox *tagCombo_;
    QComboBox *handlerCombo_;
    QDateEdit *deadlineEdit_;
    QRadioButton *normalTaskRadio_;
    QRadioButton *keyTaskRadio_;
    QRadioButton *flatDirRadio_;
    QRadioButton *subdirRadio_;
    QTextEdit *descEdit_;
    DragDropArea *dropArea_;

    QStringList sourceFiles_;  // 选择的文件列表

    void setupUI();
    void loadFormOptions();
    bool validateForm();    // 表单验证（任务名称必填）
    void ensureDictEntry();
    void accept() override; // 重写 accept，调用 onCreateClicked
};
