/**
 * @file ReferenceCreateDialog.h
 * @brief 参考资料创建弹窗（T4.9）
 *
 * 无边框模态对话框，560px 宽，自适应高度。
 * 字段：资料名称（必填）/ 编号（自动生成占位）/ 来源 / 事项标签 / 资料说明 / 资料文件（DragDropArea）
 *
 * 交互：Enter 创建，Esc 取消，资料名称必填验证
 * 创建流程：生成编号 → 入库 → 创建文件夹 → 复制文件 → 更新路径 → emit referenceCreated
 *
 * 依赖：Qt5 Widgets、application/reference/ReferenceCommandUseCase、utils/TaskNumberGenerator、
 *       service/FileService、ui/widgets/DragDropArea
 * 文档：task.md T4.9、qt_implementation_guide.md 3.7
 */
#pragma once

#include <QDialog>
#include <QStringList>

class QLineEdit;
class QComboBox;
class QTextEdit;
class QLabel;
class DragDropArea;

class ReferenceCreateDialog : public QDialog {
    Q_OBJECT

public:
    explicit ReferenceCreateDialog(QWidget *parent = nullptr);

    // ---- 获取表单数据 ----
    QString name() const;
    QString source() const;
    QString tag() const;
    QString description() const;
    QStringList sourceFiles() const;

signals:
    void referenceCreated(int refId);  // 创建成功后发射

private slots:
    void onCreateClicked();
    void onCancelClicked();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    QLineEdit *nameEdit_;
    QLineEdit *refNoEdit_;       // 编号（只读占位）
    QComboBox *sourceCombo_;
    QComboBox *tagCombo_;
    QTextEdit *descEdit_;
    DragDropArea *dropArea_;

    void setupUI();
    void loadSources();     // 从 source_dict 加载来源选项
    void loadTags();        // 从 tag_dict 加载标签选项
    void ensureDictEntry(); // 将手动录入的新来源/标签写入字典表
    bool validateForm();    // 表单验证（资料名称必填）
    void accept() override;
};
