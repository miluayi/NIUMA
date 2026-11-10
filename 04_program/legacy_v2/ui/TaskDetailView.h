/**
 * @file TaskDetailView.h
 * @brief 任务详情弹窗（T2.4）
 *
 * 无边框模态对话框，展示任务完整信息并支持状态切换、备注管理。
 * 宽度约 760px，内容区可滚动，高度自适应。
 *
 * 结构：
 *   标题栏：编号 + 状态/重点/周期标签 + 关闭按钮
 *           任务名称 + 编辑/删除/完成按钮
 *   基本信息区：QGridLayout 4 列（编号/分类/来源/标签/办理人/状态/创建时间/完成时间/截止日期/办理周期）
 *   任务说明区：只读 QTextEdit + 打开文件夹按钮 + 编辑按钮
 *   任务备注时间轴：QListWidget + 自定义 ItemDelegate，底部输入框 + 添加按钮
 *   操作历史时间轴：QListWidget 只读
 *   成果信息区：三态（未完成 / 有成果 / 已完成无成果）
 *
 * 依赖：Qt5 Widgets、application/task/TaskDetailUseCase、
 *       models/Task、models/TaskRemark、models/OperationLog、ui/ColorPalette、ui/Spacing、ui/Radius、utils/Logger
 * 文档：PRD 5.5、qt_implementation_guide.md 3.4、task.md T2.4
 *
 * 关键约束：
 *  - 所有读取与业务操作通过 TaskDetailUseCase
 *  - 完成按钮三态：进行中→绿色"标记任务完成"、已完成→红色"取消完成"、他人办理→隐藏
 *  - 重点标签仅当 isKeyTask=true 时显示；周期标签仅当 isFromPeriodic=true 时显示
 *  - 编辑按钮关闭详情弹窗并发射 editRequested(taskId)，由父窗口打开 TaskEditDialog
 *  - 成果信息区 Phase 4 完整实现，当前仅占位
 */
#pragma once

#include <QDialog>
#include <QDateTime>

class QLabel;
class QPushButton;
class QTextEdit;
class QLineEdit;
class QListWidget;
class QFrame;
class QResizeEvent;

class TaskDetailView : public QDialog {
    Q_OBJECT

public:
    explicit TaskDetailView(int taskId, QWidget *parent = nullptr);

protected:
    void resizeEvent(QResizeEvent *event) override;

signals:
    void editRequested(int taskId);   // 点击编辑按钮时发射
    void taskUpdated();               // 任务状态变更后发射（通知刷新列表）
    void taskDeleted(int taskId);     // 任务删除后发射

private slots:
    void onCompleteClicked();
    void onEditClicked();
    void onDeleteClicked();
    void onAddRemark();
    void onRemarkDeleted(int remarkId);
    void onOpenFolderClicked();
    void refreshData();

private:
    int taskId_;

    // ---- 标题栏 ----
    QLabel *taskNoLabel_;
    QLabel *statusTagLabel_;
    QLabel *keyTagLabel_;
    QLabel *periodicTagLabel_;
    QLabel *titleLabel_;
    QPushButton *editBtn_;
    QPushButton *deleteBtn_;
    QPushButton *completeBtn_;
    QPushButton *closeBtn_;

    // ---- 基本信息区 ----
    QLabel *taskNoValue_;
    QLabel *categoryValue_;
    QLabel *sourceValue_;
    QLabel *tagValue_;
    QLabel *handlerValue_;
    QLabel *statusValue_;
    QLabel *createTimeValue_;
    QLabel *finishTimeValue_;
    QLabel *deadlineValue_;
    QLabel *processingDaysValue_;

    // ---- 任务说明区 ----
    QTextEdit *descEdit_;

    // ---- 任务附件区 ----
    QLabel *attachmentLabel_;

    // ---- 备注时间轴 ----
    QListWidget *remarksList_;
    QLineEdit *remarkInput_;
    QPushButton *addRemarkBtn_;

    // ---- 操作历史 ----
    QListWidget *historyList_;

    // ---- 成果信息区 ----
    QFrame *achievementFrame_;
    QLabel *achievementLabel_;
    QPushButton *addAchievementBtn_;
    QPushButton *deleteAchievementBtn_;

    // ---- UI 构建 ----
    void setupUI();
    QFrame* buildHeader();
    QFrame* buildBasicInfoSection();
    QFrame* buildDescriptionSection();
    QFrame* buildAttachmentSection();
    QFrame* buildRemarksSection();
    QFrame* buildHistorySection();
    QFrame* buildAchievementSection();

    // ---- 数据加载 ----
    void loadTaskData();
    void loadRemarks();
    void loadHistory();
    void loadAchievement();
    void updateCompleteButton(const QString &status);

    // ---- 工具方法 ----
    QLabel* createInfoLabel(const QString &text);
    QLabel* createInfoValue(const QString &text);
    QLabel* createTagLabel(const QString &text, const QString &bgColor, const QString &textColor);
    QString actionToChinese(const QString &action) const;
    QString formatDateTime(const QDateTime &dt) const;
};
