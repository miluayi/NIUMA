/**
 * @file ScanMatchService.h
 * @brief 路径校对与扫描导入服务（无状态），扫描工作目录、修复路径偏差、导入未入库的目录
 *
 * 依赖：database/TaskDao、database/ReferenceMaterialDao、database/DatabaseManager、
 *       models/Task、models/ReferenceMaterial、service/TaskService（TaskResult）、
 *       utils/PathUtil、utils/Logger
 * 文档：task.md T8.6、TD 4.4、PRD 11.3.3/13.3、coding.md 第五章
 *
 * 关键约束：
 *  - ScanMatchService 为普通无状态 C++ 类，不继承 QObject
 *  - 所有 DAO 在方法内构造，不持有成员指针
 *  - 路径拼接使用 PathUtil::join（仅 2 参数，多段嵌套）
 *  - 扫描目录：01_工作区_进行中、02_历史归档（年/月）、04_参考资料、05_回收站、06_他人办理
 *  - 目录名解析：任务目录 编号_[分类]_任务名称、参考资料目录 编号_资料名称
 *  - 编号格式 YYYYMMDD-NNN（如 20260603-001）
 *  - 重复检测：按编号匹配，已存在标记 isDuplicate=true
 */
#pragma once

#include <QList>
#include <QString>
#include <QMap>

#include "service/TaskService.h"  // TaskResult
#include "models/ScanImportItem.h"

/**
 * @brief 路径校对结果
 */
struct ScanResult {
    int taskId = 0;           // 任务/资料 ID（未匹配时为 0）
    QString folderName;       // 磁盘上的目录名
    QString taskNo;           // 解析出的编号（YYYYMMDD-NNN）
    QString expectedPath;     // 数据库中存储的路径
    QString actualPath;       // 磁盘上的实际路径
    bool matched = false;     // 是否成功匹配到数据库记录
    QString errorMessage;     // 失败时的错误信息
};

/**
 * @brief 路径校对与扫描导入服务（无状态）
 *
 * 提供两大功能：
 * 1. 路径校对（scanAndRepairAll）：扫描工作目录，比对数据库中的 folder_path，
 *    自动修复路径偏差，返回校对结果列表。
 * 2. 扫描导入（scanForImport + importScanItem）：扫描工作目录下未入库的目录，
 *    解析目录名，预览后导入。
 */
class ScanMatchService {
public:
    ScanMatchService() = default;
    ~ScanMatchService() = default;

    // ---- 路径校对 ----

    /**
     * @brief 扫描任务及资料工作区，校对并修复路径偏差
     * @param workDir 工作目录路径
     * @return 校对结果列表（含成功和失败项）
     *
     * 扫描目录：01_工作区_进行中、02_历史归档（年/月）、04_参考资料、05_回收站、06_他人办理
     * 匹配算法：解析目录名提取编号，按编号匹配 task.task_no 或 reference_material.ref_no，
     * 若 folder_path 与实际路径不一致则更新数据库。
     */
    QList<ScanResult> scanAndRepairAll(const QString &workDir);

    /**
     * @brief 手动修复任务路径
     * @param taskId 任务 ID
     * @param newPath 新路径
     * @return 是否修复成功
     */
    bool manualRepairPath(int taskId, const QString &newPath);

    /**
     * @brief 手动修复参考资料路径
     * @param refId 参考资料 ID
     * @param newPath 新路径
     * @return 是否修复成功
     */
    bool manualRepairRefPath(int refId, const QString &newPath);

    // ---- 扫描导入 ----

    /**
     * @brief 扫描工作目录，返回待导入项列表
     * @param workDir 工作目录路径
     * @return 待导入项列表（含已存在和解析失败项）
     *
     * 名称解析规则：
     * - 任务目录：编号_[分类]_任务名称
     * - 参考资料目录：编号_资料名称
     * 重复处理：按编号匹配，已存在标记 isDuplicate=true
     */
    QList<ScanImportItem> scanForImport(const QString &workDir);

    /**
     * @brief 导入扫描项
     * @param item 扫描项（需包含 sourceDir、folderPath）
     * @param workDir 工作目录路径
     * @return TaskResult，成功时 taskId 为新记录 ID
     *
     * 办理人/状态映射：
     * - 01_工作区_进行中 → handler='本人'/status='进行中'
     * - 02_历史归档 → status='已完成'，办理人优先取 info
     * - 05_回收站 → status='已删除'，删除前状态优先取 info
     * - 06_他人办理 → status='他人办理'，无法确认办理人时使用'非本人'
     */
    TaskResult importScanItem(const ScanImportItem &item, const QString &workDir);

    TaskResult resolveConflict(const ScanImportItem &item, bool merge,
                               const QMap<QString, QString> &fieldSources);
    TaskResult resolveReferenceConflict(const ScanImportItem &item, bool merge,
                                        const QMap<QString, QString> &fieldSources);

private:
    /**
     * @brief 从目录名中提取编号（YYYYMMDD-NNN）
     * @param folderName 目录名
     * @return 编号字符串，无法解析时返回空
     */
    QString extractTaskNo(const QString &folderName) const;

    /**
     * @brief 验证编号格式（YYYYMMDD-NNN）
     * @param taskNo 编号
     * @return 是否合法
     */
    bool isValidTaskNo(const QString &taskNo) const;

    /**
     * @brief 解析任务目录名，提取编号、分类、名称
     * @param folderName 目录名（格式：编号_[分类]_任务名称）
     * @param taskNo 输出编号
     * @param category 输出分类（去括号）
     * @param name 输出任务名称
     * @return 是否解析成功
     */
    bool parseTaskFolderName(const QString &folderName, QString &taskNo,
                             QString &category, QString &name) const;

    /**
     * @brief 解析参考资料目录名，提取编号、名称
     * @param folderName 目录名（格式：编号_资料名称）
     * @param refNo 输出编号
     * @param name 输出资料名称
     * @return 是否解析成功
     */
    bool parseRefFolderName(const QString &folderName, QString &refNo,
                            QString &name) const;

    /**
     * @brief 仅扫描工作区根层匹配编号格式的任务目录，不扫描附件子目录
     * @param rootDir 根目录路径
     * @return 匹配的子目录完整路径列表
     */
    QStringList scanFolders(const QString &rootDir) const;
    QStringList scanArchivedFolders(const QString &rootDir) const;
};
