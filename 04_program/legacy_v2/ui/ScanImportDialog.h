/**
 * @file ScanImportDialog.h
 * @brief 扫描导入弹窗（T8.7）
 *
 * 模态对话框，900px 宽，扫描工作目录下未导入的任务和资料目录，预览后选择导入。
 *
 * 布局：
 *  - 标题"扫描导入"
 *  - 顶部说明文本
 *  - "开始扫描"按钮
 *  - 预览表格（5 列：复选框/目录名/解析结果/导入类型/状态）
 *  - 底部按钮："全选"、"导入所选"、"取消"
 *
 * 交互：
 *  - 点击"开始扫描"调用扫描导入用例并显示结果
 *  - 点击"导入所选"导入勾选项，显示结果反馈
 *  - 关闭时发射 importCompleted 信号
 *
 * 依赖：Qt5 Widgets、application/maintenance/ScanImportUseCase、ui/Spacing、ui/ColorPalette
 * 文档：task.md T8.7、PRD 13.3
 */
#pragma once

#include <QDialog>
#include <QList>

#include "models/ScanImportItem.h"

class QTableWidget;
class QPushButton;
class QLabel;

class ScanImportDialog : public QDialog {
    Q_OBJECT

public:
    explicit ScanImportDialog(QWidget *parent = nullptr);

signals:
    /**
     * @brief 导入完成信号
     * @param successCount 成功导入数
     * @param skipCount 跳过数（已存在或未勾选）
     * @param failCount 失败数
     */
    void importCompleted(int successCount, int skipCount, int failCount);

private slots:
    void onScanClicked();
    void onSelectAllClicked();
    void onImportClicked();
    void onFinishClicked();
    void onResolveClicked();
    void onCancelClicked();

private:
    void setupUI();
    void populateTable(const QList<ScanImportItem> &items);
    void clearTable();

    QTableWidget *table_;
    QLabel *hintLabel_;
    QList<ScanImportItem> scanItems_;  // 当前表格数据
};
