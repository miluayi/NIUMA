/**
 * @file FileService.h
 * @brief 文件系统业务逻辑层：工作目录管理、任务目录创建/移动/归档、文件导入、自愈
 *
 * 职责：
 *   - 工作目录初始化与验证（6 个根目录）
 *   - 任务/参考资料目录命名与创建
 *   - 状态变更时的目录移动（完成/取消/删除/恢复/办理人变更）
 *   - 文件夹丢失自愈
 *   - 工作目录迁移
 *
 * 依赖：database/SettingsDao、database/TaskDao、database/DatabaseManager、
 *       models/Task、models/ReferenceMaterial、utils/FileUtil、utils/PathUtil、utils/Logger
 * 文档：TD 3.9（FileService 接口）、PRD 第六部分、task.md T3.1-T3.7
 *
 * 关键约束：
 *   - 6 个根目录名称固定不可修改
 *   - 任务目录命名：编号_[分类]_任务名称
 *   - 参考资料目录命名：编号_资料名称（无分类）
 *   - 子目录（has_subdirs=1）：01_原始资料/02_工作底稿/03_反馈成果
 *   - 归档结构：02_历史归档/年/月/
 *   - 路径长度 < 260 字符
 *   - 工作目录路径从 settings 表 work_dir 键读取
 */
#pragma once

#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QFuture>

class Task;
class ReferenceMaterial;

class FileService {
public:
    /// 工作目录初始化结果
    struct InitResult {
        bool success = false;
        QString errorMessage;
        QString workDir;  // 实际工作目录路径（{rootPath}/工作目录）
    };

    /// 工作目录迁移报告
    struct MigrationReport {
        int totalRecords = 0;
        int successCount = 0;
        int failCount = 0;
        QStringList failedItems;
    };

    // ===== 6 个根目录名称（固定常量） =====
    static const QString kDirInProgress;   // 01_工作区_进行中
    static const QString kDirArchived;     // 02_历史归档
    static const QString kDirAnnualResult; // 03_年度成果
    static const QString kDirReference;    // 04_参考资料
    static const QString kDirRecycleBin;   // 05_回收站
    static const QString kDirOtherHandler; // 06_他人办理

    // ===== 工作目录管理 =====
    InitResult initializeWorkDir(const QString &rootPath);
    bool validateWorkDir(const QString &rootPath) const;
    bool validateStructure(const QString &workDir) const;

    /** @brief 从 settings 表读取当前工作目录路径 */
    QString getWorkDir() const;

    /** @brief 设置工作目录到 settings 表 */
    bool setWorkDir(const QString &workDir);

    // ===== 目录命名 =====
    QString buildTaskFolderName(const Task &task) const;
    QString buildTaskFolderPath(const Task &task, const QString &workDir) const;
    QString buildRefFolderName(const ReferenceMaterial &ref) const;
    QString buildRefFolderPath(const ReferenceMaterial &ref, const QString &workDir) const;

    // ===== 目录创建 =====
    bool createTaskFolder(const Task &task);
    bool createRefFolder(const ReferenceMaterial &ref);

    // ===== 状态变更目录移动 =====
    bool archiveTaskFolder(const Task &task);
    bool moveTaskFolderForStatusChange(Task &task, const QString &oldStatus);
    bool moveTaskFolderForHandlerChange(Task &task, const QString &targetDirName);
    bool renameTaskFolder(Task &task);

    // ===== 目录删除 =====
    /** @brief 物理删除任务目录（回收站彻底删除 / 过期清理时调用） */
    bool deleteTaskFolder(const Task &task);

    // ===== 自愈 =====
    bool tryAutoRepair(const Task &task);

    // ===== 工作目录迁移 =====
    bool batchUpdatePaths(const QString &oldRoot, const QString &newRoot);
    MigrationReport migrateWorkDir(const QString &oldPath, const QString &newPath);

    // ===== 文件复制 =====
    static bool copyFilesToTaskDir(const QStringList &sourceFiles, const QString &taskDir);

    /** @brief 后台异步复制文件到任务目录（QtConcurrent::run） */
    static QFuture<bool> copyFilesToTaskDirAsync(const QStringList &sourceFiles, const QString &taskDir);

private:
    /** @brief 根据状态返回对应的根目录名 */
    QString rootDirNameForStatus(const QString &status) const;

    /** @brief 构造归档路径 02_历史归档/年/月/ */
    QString buildArchivePath(const QString &workDir, const QDateTime &dateTime) const;

    /** @brief 扫描指定根目录下按 folder_name 匹配任务目录 */
    QString scanForFolderName(const QString &rootDir, const QString &folderName) const;
};
