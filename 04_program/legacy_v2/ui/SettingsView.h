/**
 * @file SettingsView.h
 * @brief 设置中心页面（T4.6）
 *
 * QWidget 页面，用于"设置中心"标签页（MainWindow stacked widget index 7）。
 * 包含三张卡片：工作目录 / 导出设置 / 数据管理。
 *
 * 布局：
 *   QVBoxLayout
 *   ├── page-header（标题"设置中心" + 副标题"管理应用配置和导出选项"）
 *   ├── Card1 工作目录（当前路径 + 更改目录/打开目录按钮）
 *   ├── Card2 导出设置（报告标题/复选框/下拉框 + 保存设置/恢复默认按钮）
 *   └── Card3 数据管理（立即备份/恢复备份按钮 + 最近备份时间）
 *
 * 信号：
 *   - workDirChangeRequested()：点击"更改目录"，由 MainWindow 处理 QFileDialog
 *   - backupRequested()：点击"立即备份"
 *   - restoreRequested()：点击"恢复备份"
 *
 * 依赖：Qt5 Widgets、application/settings/SettingsUseCase、models/ExportConfig、
 *       ui/ColorPalette、ui/Spacing、ui/Radius、utils/Logger
 * 文档：task.md T4.6、PRD 13.2
 */
#pragma once

#include <QWidget>

#include "models/ExportConfig.h"

class QLabel;
class QLineEdit;
class QCheckBox;
class QComboBox;
class QPushButton;
class QFrame;
class QListWidget;
class QButtonGroup;

class SettingsView : public QWidget {
    Q_OBJECT

public:
    explicit SettingsView(QWidget *parent = nullptr);

    void refresh();
    void setWorkDir(const QString &path);

signals:
    void workDirChangeRequested();
    void backupRequested();
    void restoreRequested();
    void scanImportRequested();
    void initCompleted();  // 软件初始化完成，需引导重新设置工作目录
    void fontPreferencesChanged(int pixelSize, int weight);

private slots:
    void onChangeDirClicked();
    void onOpenDirClicked();
    void onSaveClicked();
    void onResetDefaultsClicked();
    void onBackupClicked();
    void onRestoreClicked();
    void onScanImportClicked();
    void onInitClicked();
    void onFontPreferencesChanged();

private:
    // Card 1: 工作目录
    QLabel *workDirLabel_;
    QPushButton *changeDirBtn_;
    QPushButton *openDirBtn_;

    // Card 2: 导出设置
    QLineEdit *reportTitleEdit_;
    QCheckBox *includeAchievementsCheck_;
    QCheckBox *includeCategoryStatsCheck_;
    QCheckBox *includeSourceStatsCheck_;
    QCheckBox *includeAmountSummaryCheck_;
    QCheckBox *includeKeyAchievementsCheck_;
    QComboBox *sortByCombo_;
    QComboBox *dateFormatCombo_;
    QComboBox *amountUnitCombo_;
    QPushButton *saveBtn_;
    QPushButton *resetBtn_;
    QLabel *statusLabel_;  // 保存后显示"设置已保存"

    // Card: 字体预览与显示设置
    QComboBox *fontSizeCombo_;
    QButtonGroup *fontWeightGroup_;

    // Card 3: 数据管理
    QLabel *lastBackupLabel_;
    QPushButton *backupBtn_;
    QPushButton *restoreBtn_;
    QPushButton *scanImportBtn_;

    // Card: 来源与标签与办理人管理
    QListWidget *sourceList_;
    QLineEdit *sourceInput_;
    QListWidget *tagList_;
    QLineEdit *tagInput_;
    QListWidget *handlerList_;
    QLineEdit *handlerInput_;

    void setupUI();
    QFrame* buildHeader();
    QFrame* buildWorkDirCard();
    QFrame* buildDictManagementCard();
    QFrame* buildExportCard();
    QFrame* buildFontCard();
    QFrame* buildDataMgmtCard();
    QFrame* buildAboutCard();

    void loadSourceList();
    void loadTagList();
    void loadHandlerList();
    void onAddSource();
    void onDeleteSource(int dictId);
    void onAddTag();
    void onDeleteTag(int dictId);
    void onAddHandler();
    void onDeleteHandler(int dictId);

    void loadConfig();
    void applyConfigToUi(const ExportConfig &config);
    ExportConfig collectConfigFromUi() const;
    bool saveConfig(const ExportConfig &config);
    void loadWorkDir();
    void loadLastBackupTime();
    void showStatus(const QString &text);
};
