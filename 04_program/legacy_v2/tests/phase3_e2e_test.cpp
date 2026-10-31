/**
 * @file phase3_e2e_test.cpp
 * @brief Phase 3 E2E 测试：FileService 文件系统操作集成测试
 *
 * 验证场景：
 *   a. 工作目录初始化（6 个根目录）
 *   b. 任务目录创建（01_工作区_进行中，命名 编号_[分类]_任务名称）
 *   c. 完成任务 → 移动到归档（02_历史归档/YYYY/MM/）
 *   d. 取消完成 → 移回进行中
 *   e. 删除任务 → 移到回收站（05_回收站）
 *   f. 恢复任务 → 移回进行中
 *   g. 办理人变更 → 他人办理（06_他人办理）
 *   h. 办理人变更回 → 本人（01_工作区_进行中）
 *   i. 自愈：手动移到错误位置，tryAutoRepair 更新 folder_path
 *   j. 文件复制（同步）
 *   k. 文件复制（异步）
 */

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFuture>
#include <QSqlQuery>
#include <QStringList>
#include <QTextStream>

#include <iostream>

#include "DatabaseManager.h"
#include "FileService.h"
#include "SettingsDao.h"
#include "TaskDao.h"
#include "TaskService.h"
#include "models/Task.h"
#include "utils/FileUtil.h"
#include "utils/Logger.h"
#include "utils/PathUtil.h"

// ============================================================================
// 测试结果统计
// ============================================================================
static int g_passed = 0;
static int g_total = 0;

static void report(const QString &name, bool passed, const QString &detail = QString())
{
    g_total++;
    if (passed)
        g_passed++;
    QString status = passed ? QStringLiteral("PASS") : QStringLiteral("FAIL");
    QString line = QStringLiteral("[%1] %2").arg(status, name);
    if (!detail.isEmpty())
        line += QStringLiteral(" - ") + detail;
    std::cout << line.toLocal8Bit().constData() << std::endl;
}

static bool dirExists(const QString &path)
{
    return QDir(path).exists();
}

static QString currentYear()
{
    return QString::number(QDateTime::currentDateTime().date().year());
}

static QString currentMonth()
{
    return QString::number(QDateTime::currentDateTime().date().month()).rightJustified(2, '0');
}

// ============================================================================
// 主函数
// ============================================================================
int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    std::cout << "============================================================" << std::endl;
    std::cout << "NIUMA V2.0 Phase 3 E2E Test (FileService Integration)" << std::endl;
    std::cout << "============================================================" << std::endl;

    // 1. 创建临时测试目录
    QString tempBase = qEnvironmentVariable("NIUMA_TEST_ROOT");
    if (tempBase.isEmpty()) {
        tempBase = QDir::tempPath() + "/niuma_phase3_test";
    }
    tempBase = QDir::cleanPath(tempBase);

    // 2. 清理之前测试数据
    if (QDir(tempBase).exists()) {
        QDir(tempBase).removeRecursively();
    }
    QDir().mkpath(tempBase);

    // 3. 初始化数据库
    QString dbPath = tempBase + "/test_phase3.db";
    if (!DatabaseManager::instance().initialize(dbPath)) {
        std::cout << "FATAL: Database initialization failed" << std::endl;
        return 1;
    }

    // 4. 使用 FileService 初始化工作目录
    FileService fileService;
    FileService::InitResult initResult = fileService.initializeWorkDir(tempBase);
    QString workDir = fileService.getWorkDir(); // tempBase/NIUMA Journal

    if (workDir.isEmpty()) {
        std::cout << "FATAL: Work dir is empty" << std::endl;
        return 1;
    }

    std::cout << "Work dir: " << workDir.toLocal8Bit().constData() << std::endl;
    std::cout << "------------------------------------------------------------" << std::endl;

    TaskService taskService;

    // ===== Test a: 工作目录初始化 =====
    {
        QStringList expectedDirs = {
            FileService::kDirInProgress,
            FileService::kDirArchived,
            FileService::kDirAnnualResult,
            FileService::kDirReference,
            FileService::kDirRecycleBin,
            FileService::kDirOtherHandler
        };
        bool allExist = true;
        for (const QString &dirName : expectedDirs) {
            if (!dirExists(PathUtil::join(workDir, dirName))) {
                allExist = false;
                break;
            }
        }
        report("a. Work dir initialization (6 root dirs)",
               allExist && initResult.success,
               allExist ? "All 6 dirs exist" : "Missing dirs");
    }

    // ===== Test b: 任务目录创建 =====
    int taskId1 = 0;
    QString folderName1;
    {
        Task task;
        task.title = QStringLiteral("测试任务-创建");
        task.category = QStringLiteral("查");
        task.source = QStringLiteral("风险管理股");
        task.handler = QStringLiteral("本人");
        TaskResult result = taskService.createTask(task);
        taskId1 = result.taskId;

        auto optTask = taskService.getTaskById(taskId1);
        folderName1 = optTask->folderName;

        QString expectedPath = PathUtil::join(
            PathUtil::join(workDir, FileService::kDirInProgress), folderName1);

        // 验证命名格式：编号_[分类]_任务名称
        bool nameFormatOk = folderName1.contains(QStringLiteral("_[") + task.category + QStringLiteral("]_"));

        report("b. Task folder creation",
               result.ok && dirExists(expectedPath) && nameFormatOk,
               QStringLiteral("folder=%1").arg(folderName1));
    }

    // ===== Test c: 完成任务 → 移到归档 =====
    {
        TaskResult result = taskService.completeTask(taskId1);

        // 归档路径：02_历史归档/YYYY/MM/folderName
        QString archiveBase = PathUtil::join(
            PathUtil::join(PathUtil::join(workDir, FileService::kDirArchived), currentYear()), currentMonth());
        QString archivePath = PathUtil::join(archiveBase, folderName1);

        report("c. Complete task moves to archive",
               result.ok && dirExists(archivePath),
               QStringLiteral("path=%1").arg(archivePath));
    }

    // ===== Test d: 取消完成 → 移回进行中 =====
    {
        TaskResult result = taskService.cancelComplete(taskId1);

        QString inProgressPath = PathUtil::join(
            PathUtil::join(workDir, FileService::kDirInProgress), folderName1);

        report("d. Cancel complete moves back",
               result.ok && dirExists(inProgressPath),
               QStringLiteral("path=%1").arg(inProgressPath));
    }

    // ===== Test e: 删除任务 → 移到回收站 =====
    {
        TaskResult result = taskService.deleteTask(taskId1);

        QString recyclePath = PathUtil::join(
            PathUtil::join(workDir, FileService::kDirRecycleBin), folderName1);

        report("e. Delete moves to recycle bin",
               result.ok && dirExists(recyclePath),
               QStringLiteral("path=%1").arg(recyclePath));
    }

    // ===== Test f: 恢复任务 → 移回进行中 =====
    {
        TaskResult result = taskService.restoreTask(taskId1);

        QString inProgressPath = PathUtil::join(
            PathUtil::join(workDir, FileService::kDirInProgress), folderName1);

        report("f. Restore moves back",
               result.ok && dirExists(inProgressPath),
               QStringLiteral("path=%1").arg(inProgressPath));
    }

    // ===== Test g: 办理人变更为他人 =====
    int taskId2 = 0;
    QString folderName2;
    {
        Task task;
        task.title = QStringLiteral("测试任务-办理人变更");
        task.category = QStringLiteral("办");
        task.source = QStringLiteral("征管股");
        task.handler = QStringLiteral("本人");
        TaskResult result = taskService.createTask(task);
        taskId2 = result.taskId;

        auto optTask = taskService.getTaskById(taskId2);
        folderName2 = optTask->folderName;

        // 变更办理人为非"本人"
        Task updated = *optTask;
        updated.handler = QStringLiteral("张三");
        TaskResult updateResult = taskService.updateTask(updated);

        QString otherHandlerPath = PathUtil::join(
            PathUtil::join(workDir, FileService::kDirOtherHandler), folderName2);

        report("g. Handler change to other handler",
               updateResult.ok && dirExists(otherHandlerPath),
               QStringLiteral("path=%1").arg(otherHandlerPath));
    }

    // ===== Test h: 办理人变更回本人 =====
    {
        auto optTask = taskService.getTaskById(taskId2);
        Task updated = *optTask;
        updated.handler = QStringLiteral("本人");
        TaskResult updateResult = taskService.updateTask(updated);

        QString inProgressPath = PathUtil::join(
            PathUtil::join(workDir, FileService::kDirInProgress), folderName2);

        report("h. Handler change back to self",
               updateResult.ok && dirExists(inProgressPath),
               QStringLiteral("path=%1").arg(inProgressPath));
    }

    // ===== Test i: 自愈 =====
    int taskId3 = 0;
    QString folderName3;
    {
        Task task;
        task.title = QStringLiteral("测试任务-自愈");
        task.category = QStringLiteral("报");
        task.source = QStringLiteral("税政股");
        task.handler = QStringLiteral("本人");
        TaskResult result = taskService.createTask(task);
        taskId3 = result.taskId;

        auto optTask = taskService.getTaskById(taskId3);
        folderName3 = optTask->folderName;

        // 手动将目录从 01 移到 06（错误位置）
        QString srcPath = PathUtil::join(
            PathUtil::join(workDir, FileService::kDirInProgress), folderName3);
        QString wrongPath = PathUtil::join(
            PathUtil::join(workDir, FileService::kDirOtherHandler), folderName3);
        FileUtil::moveDir(srcPath, wrongPath);

        // 调用自愈（task.folderPath 仍指向 01 旧路径）
        bool repairResult = fileService.tryAutoRepair(*optTask);

        // 验证数据库 folder_path 已更新为实际位置（06）
        auto optRepaired = taskService.getTaskById(taskId3);
        QString dbFolderPath = optRepaired->folderPath;

        report("i. Auto-repair updates folder_path",
               repairResult && dbFolderPath == wrongPath,
               QStringLiteral("db_path=%1").arg(dbFolderPath));
    }

    // ===== Test j: 文件复制（同步） =====
    {
        // 创建源文件
        QString srcDir = tempBase + "/src_files_sync";
        QDir().mkpath(srcDir);
        QString srcFile1 = srcDir + "/file1.txt";
        QString srcFile2 = srcDir + "/file2.txt";
        {
            QFile f1(srcFile1);
            f1.open(QIODevice::WriteOnly);
            f1.write("content1");
        }
        {
            QFile f2(srcFile2);
            f2.open(QIODevice::WriteOnly);
            f2.write("content2");
        }

        // 目标目录：使用 task3 的目录（自愈后在 06）
        QString taskDir = PathUtil::join(
            PathUtil::join(workDir, FileService::kDirOtherHandler), folderName3);

        QStringList sourceFiles = {srcFile1, srcFile2};
        bool copyResult = FileService::copyFilesToTaskDir(sourceFiles, taskDir);

        bool file1Copied = QFile::exists(PathUtil::join(taskDir, "file1.txt"));
        bool file2Copied = QFile::exists(PathUtil::join(taskDir, "file2.txt"));

        report("j. File copy (sync)",
               copyResult && file1Copied && file2Copied,
               QStringLiteral("file1=%1 file2=%2").arg(file1Copied).arg(file2Copied));
    }

    // ===== Test k: 文件复制（异步） =====
    {
        QString srcDir = tempBase + "/src_files_async";
        QDir().mkpath(srcDir);
        QString srcFile1 = srcDir + "/async1.txt";
        QString srcFile2 = srcDir + "/async2.txt";
        {
            QFile f1(srcFile1);
            f1.open(QIODevice::WriteOnly);
            f1.write("async_content1");
        }
        {
            QFile f2(srcFile2);
            f2.open(QIODevice::WriteOnly);
            f2.write("async_content2");
        }

        QString taskDir = PathUtil::join(
            PathUtil::join(workDir, FileService::kDirOtherHandler), folderName3);

        QStringList sourceFiles = {srcFile1, srcFile2};
        QFuture<bool> future = FileService::copyFilesToTaskDirAsync(sourceFiles, taskDir);

        future.waitForFinished();
        bool copyResult = future.result();

        bool file1Copied = QFile::exists(PathUtil::join(taskDir, "async1.txt"));
        bool file2Copied = QFile::exists(PathUtil::join(taskDir, "async2.txt"));

        report("k. File copy (async)",
               copyResult && file1Copied && file2Copied,
               QStringLiteral("file1=%1 file2=%2").arg(file1Copied).arg(file2Copied));
    }

    // ===== 汇总 =====
    // 分类编辑必须同步改名，目录遗失时仍可将数据库记录移入回收站。
    {
        Task task;
        task.title = QStringLiteral("分类改名验证");
        task.category = QStringLiteral("查");
        task.handler = QStringLiteral("本人");
        const TaskResult created = taskService.createTask(task);
        auto before = taskService.getTaskById(created.taskId);
        bool renamed = false;
        bool deletedMissing = false;
        if (before && QDir(before->folderPath).exists()) {
            Task edited = *before;
            edited.category = QStringLiteral("催");
            const TaskResult saved = taskService.updateTask(edited);
            auto after = taskService.getTaskById(created.taskId);
            renamed = saved.ok && after && after->folderName.contains(QStringLiteral("_[催]_"))
                && QDir(after->folderPath).exists() && !QDir(before->folderPath).exists();
            if (after && QDir::cleanPath(after->folderPath).startsWith(
                    QDir::cleanPath(workDir) + '/', Qt::CaseInsensitive)
                && QDir(after->folderPath).removeRecursively()) {
                const TaskResult removed = taskService.deleteTask(created.taskId);
                auto deleted = taskService.getTaskById(created.taskId);
                deletedMissing = removed.ok && deleted
                    && deleted->status == QStringLiteral("已删除")
                    && deleted->folderPath.isEmpty();
            }
        }
        report("l. Category edit renames task folder", renamed);
        report("m. Missing folder does not block delete", deletedMissing);
    }

    // ===== 汇总 =====
    std::cout << "============================================================" << std::endl;
    std::cout << g_passed << "/" << g_total << " tests passed" << std::endl;
    std::cout << "============================================================" << std::endl;

    // 清理（全部通过时清理，失败时保留用于调试）
    if (g_passed == g_total) {
        DatabaseManager::instance().close();
        QDir(tempBase).removeRecursively();
        std::cout << "Temp dir cleaned up." << std::endl;
    } else {
        std::cout << "Test data kept at: " << tempBase.toLocal8Bit().constData() << std::endl;
    }

    return (g_passed == g_total) ? 0 : 1;
}
