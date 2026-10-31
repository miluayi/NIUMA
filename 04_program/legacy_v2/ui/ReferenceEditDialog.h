/**
 * @file ReferenceEditDialog.h
 * @brief 参考资料编辑弹窗
 *
 * 无边框模态对话框，560px 宽，编辑现有参考资料。
 * 字段：资料名称（必填）/ 编号（只读）/ 来源 / 事项标签 / 资料说明
 * 不含 DragDropArea（文件夹结构在创建时固定，编辑模式不可变更文件）
 *
 * 交互：Enter 保存，Esc 取消，资料名称必填验证
 * 保存时仅更新 name/source/tag/description，不修改 refNo/folderName/folderPath
 *
 * 依赖：Qt5 Widgets、application/reference/ReferenceCommandUseCase
 * 文档：qt_implementation_guide.md 3.7
 */
#pragma once

#include <QDateTime>
#include <QDialog>

class QLineEdit;
class QComboBox;
class QTextEdit;
class QLabel;

class ReferenceEditDialog : public QDialog {
    Q_OBJECT

public:
    explicit ReferenceEditDialog(int refId, QWidget *parent = nullptr);

signals:
    void referenceUpdated(int refId);  // 保存成功后发射

private slots:
    void onSaveClicked();
    void onCancelClicked();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    int refId_;

    QLineEdit *nameEdit_;
    QLineEdit *refNoEdit_;       // 编号（只读）
    QComboBox *sourceCombo_;
    QComboBox *tagCombo_;
    QTextEdit *descEdit_;

    // 原始数据（保存时保持不变的字段）
    QString originalRefNo_;
    QString originalFolderName_;
    QString originalFolderPath_;
    QDateTime originalCreateTime_;

    void setupUI();
    void loadSources();
    void loadTags();
    void ensureDictEntry();  // 将手动录入的新来源/标签写入字典表
    void loadData();
    bool validateForm();
    void accept() override;
};
