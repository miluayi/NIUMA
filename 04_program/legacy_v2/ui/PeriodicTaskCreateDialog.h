/**
 * @file PeriodicTaskCreateDialog.h
 * @brief 周期任务模板创建/编辑弹窗（Phase 7）
 *
 * 无边框模态对话框，500px 宽，自适应高度。
 * 字段：模板名称（必填）/ 分类 / 周期类型 / 周期值 / 来源 / 事项标签 / 办理人 / 提醒时间 / 启用
 *
 * 交互：Enter 保存，Esc 取消，模板名称必填验证
 * 构造：
 *  - PeriodicTaskCreateDialog(parent)：新建模式，标题"新建周期模板"
 *  - PeriodicTaskCreateDialog(id, parent)：编辑模式，标题"编辑周期模板"，预填数据
 *
 * 依赖：Qt5 Widgets、models/PeriodicTask、ui/ColorPalette、ui/Spacing、ui/Radius
 * 文档：Phase 7、PRD 9.3、qt_implementation_guide.md 3.7、coding.md 第十一节
 */
#pragma once

#include <QDialog>

#include "models/PeriodicTask.h"

class QLineEdit;
class QComboBox;
class QSpinBox;
class QTimeEdit;
class QDateEdit;
class QCheckBox;
class QLabel;
class QPushButton;
class QFrame;

class PeriodicTaskCreateDialog : public QDialog {
    Q_OBJECT

public:
    // 新建模式
    explicit PeriodicTaskCreateDialog(QWidget *parent = nullptr);
    // 编辑模式（按 ID 加载现有模板）
    PeriodicTaskCreateDialog(int id, QWidget *parent = nullptr);

    // 返回表单数据组装的 PeriodicTask（编辑模式下保留原 id/createTime）
    PeriodicTask getPeriodicTask() const;

signals:
    void templateSaved();  // 创建/编辑成功后发射

private slots:
    void onSaveClicked();
    void onCancelClicked();
    void onPeriodTypeChanged();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    // 控件
    QLabel *titleLabel_;
    QLineEdit *nameEdit_;
    QComboBox *categoryCombo_;
    QComboBox *periodTypeCombo_;
    QSpinBox *periodValueSpin_;
    QComboBox *sourceCombo_;
    QComboBox *tagCombo_;
    QLineEdit *handlerEdit_;
    QTimeEdit *reminderTimeEdit_;
    QCheckBox *enabledCheck_;
    QPushButton *saveBtn_;
    // 执行日选择（根据周期类型动态切换）
    QFrame *targetDayRow_;
    QComboBox *targetDayCombo_;    // 每周(周一~周日) / 每月(1~31号)
    QDateEdit *targetDayDateEdit_; // 每年(MM月dd日)

    // 状态
    int editingId_;             // >0 表示编辑模式
    PeriodicTask originalTask_; // 编辑模式保留原始数据

    void setupUI();
    void loadSources();
    void loadTags();
    void loadTemplateData();
    bool validateForm();
    void accept() override;

    // 周期类型与中文互转
    static QString periodTypeToText(const QString &type);
    static QString textToPeriodType(const QString &text);
};
