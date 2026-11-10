/**
 * @file AchievementDialog.h
 * @brief 成果填写弹窗（T4.2）
 *
 * 无边框模态对话框，560px 宽，支持填写和编辑成果信息。
 * 字段：工作内容 / 工作对象 / 成果数量 / 成果金额（元） /
 *       成果说明 / 纳入年度成果 / 重点成果
 *
 * 交互："保存并完成" 入库并 accept，"跳过" 直接 reject（返回 nullopt）
 * 金额：UI 输入元字符串，存储前用 AmountUtil::yuanToCents 转为分（qint64）
 *
 * 依赖：Qt5 Widgets、application/achievement/AchievementUseCase、models/Achievement、utils/AmountUtil
 * 文档：task.md T4.2、qt_implementation_guide.md 3.7、coding.md 第十一章
 */
#pragma once

#include <QDialog>
#include <QString>
#include <optional>

#include "models/Achievement.h"

class QLineEdit;
class QTextEdit;
class QCheckBox;
class QLabel;

class AchievementDialog : public QDialog {
    Q_OBJECT

public:
    AchievementDialog(int taskId, const QString &taskCategory,
                      const QString &taskTitle, QWidget *parent = nullptr,
                      const std::optional<Achievement> &existing = std::nullopt);
    void setKeyAchievementDefault(bool checked);
    static bool confirmDelete(int achievementId, QWidget *parent);

    /**
     * @brief 获取成果数据
     * @return 用户点击"保存并完成"时返回 Achievement，点击"跳过"时返回 nullopt
     */
    std::optional<Achievement> getAchievement() const;

private slots:
    void onSaveClicked();
    void onSkipClicked();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    int taskId_;
    QString taskCategory_;
    QString taskTitle_;
    std::optional<Achievement> existing_;

    // 控件指针
    QLabel *guideLabel_;
    QLineEdit *workContentEdit_;
    QLineEdit *workObjectEdit_;
    QLineEdit *resultCountEdit_;
    QLineEdit *resultAmountEdit_;
    QTextEdit *descEdit_;
    QCheckBox *annualCheck_;
    QCheckBox *keyCheck_;

    // 结果数据（"保存并完成"后填充）
    std::optional<Achievement> result_;

    void setupUI();
    QString guideTextForCategory(const QString &category) const;
    QString descPlaceholderForCategory(const QString &category) const;
    bool validateForm();
    void accept() override;  // 重写 accept，走 onSaveClicked 流程
};
