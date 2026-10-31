/**
 * @file ReferenceDetailDialog.h
 * @brief 参考资料详情弹窗（T4.10）
 *
 * 无边框模态对话框，600px 宽，展示参考资料完整信息。
 *
 * 结构：
 *   标题栏：资料名称 + 关闭按钮
 *   基本信息：QGridLayout 2×2（编号 / 来源 / 事项标签 / 创建时间）
 *   资料说明：标题行 + "打开文件夹"按钮 + 只读说明卡片
 *   底部：编辑 / 删除（左）+ 关闭（右）
 *
 * 依赖：Qt5 Widgets、application/reference/ReferenceQueryUseCase、service/FileService、utils/FileUtil
 * 文档：task.md T4.10、qt_implementation_guide.md 3.4
 */
#pragma once

#include <QDialog>

class QLabel;
class QPushButton;
class QTextEdit;
class QFrame;

class ReferenceDetailDialog : public QDialog {
    Q_OBJECT

public:
    explicit ReferenceDetailDialog(int refId, QWidget *parent = nullptr);

signals:
    void editRequested(int refId);
    void referenceDeleted(int refId);

private slots:
    void onOpenFolderClicked();
    void onEditClicked();
    void onDeleteClicked();
    void onCloseClicked();

private:
    int refId_;

    // 标题栏
    QLabel *titleLabel_;
    QPushButton *closeBtn_;

    // 基本信息
    QLabel *refNoValue_;
    QLabel *sourceValue_;
    QLabel *tagValue_;
    QLabel *createTimeValue_;

    // 资料说明
    QTextEdit *descEdit_;
    QPushButton *openFolderBtn_;

    // 数据
    QString folderPath_;
    QString refName_;

    void setupUI();
    QFrame* buildHeader();
    QFrame* buildBasicInfoSection();
    QFrame* buildDescriptionSection();
    QFrame* buildFooter();
    void loadData();

    QLabel* createInfoLabel(const QString &text);
    QLabel* createInfoValue(const QString &text);
};
