#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QLabel>
#include <QSqlQuery>
#include <QTableWidget>
#include <QTimer>
#include <QTemporaryDir>
#include <algorithm>

#include "application/maintenance/MainWindowMaintenanceUseCase.h"
#include "application/maintenance/StartupUseCase.h"
#include "DatabaseManager.h"
#include "FileService.h"
#include "ScanImportDialog.h"
#include "SettingsDao.h"
#include "SettingsView.h"
#include "TaskDao.h"
#include "ReferenceMaterialDao.h"
#include "BackupService.h"
#include "ScanMatchService.h"
#include "TaskInfoFile.h"
#include "database/AchievementDao.h"
#include "ReferenceInfoFile.h"
#include "application/recyclebin/RecycleBinUseCase.h"
#include "utils/FileUtil.h"

namespace {
bool insertTask(const QString &number, const QString &title)
{
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("INSERT INTO task (task_no, title, category, source, handler, status, create_time) VALUES (?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue(number); query.addBindValue(title); query.addBindValue(QStringLiteral("查"));
    query.addBindValue(QStringLiteral("S11")); query.addBindValue(QStringLiteral("本人"));
    query.addBindValue(QStringLiteral("进行中")); query.addBindValue(QStringLiteral("2026-09-22 09:00:00"));
    return query.exec();
}

void closeMessage(bool accept)
{
    QTimer::singleShot(0, [accept]() {
        if (auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget())) {
            accept ? dialog->accept() : dialog->reject();
        }
    });
}
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    if (BackupService::workDirSize(QString()) != 0) return 43;
    const QString root = qEnvironmentVariable("NIUMA_TEST_ROOT");
    if (root.isEmpty()) return 1;
    if (!QDir().mkpath(root)) return 2;
    QTemporaryDir session(QDir(root).filePath(QStringLiteral("session-XXXXXX")));
    if (!session.isValid()) return 2;
    const QString sessionRoot = session.path();
    const QString dataDir = QDir(sessionRoot).filePath("Data");
    qputenv("NIUMA_DATA_DIR", dataDir.toUtf8());
    if (!DatabaseManager::instance().initialize(QDir(dataDir).filePath("database.db"))) return 2;
    QSqlQuery clearInitial(DatabaseManager::instance().database());
    if (!clearInitial.exec("DELETE FROM task") || !clearInitial.exec("DELETE FROM settings")) return 3;
    DatabaseManager::instance().close();
    const QString workRoot = QDir(sessionRoot).filePath("work-root");
    bool requestedWorkDir = false;
    const StartupResult startupResult = StartupUseCase().prepare([&]() {
        requestedWorkDir = true;
        return workRoot;
    });
    if (!startupResult.databaseInitialized || !startupResult.workDirInitialized
        || !requestedWorkDir || startupResult.workDir.isEmpty()) return 2;
    QSqlQuery clear(DatabaseManager::instance().database());
    if (!clear.exec("DELETE FROM task")) return 3;

    const QString workDir = startupResult.workDir;
    if (!insertTask("20260922-900", "备份基线任务")) return 4;
    const MainWindowMaintenanceUseCase maintenance;
    const QString backupPath = maintenance.manualBackupTo(QDir(sessionRoot).filePath("backups"));
    if (backupPath.isEmpty() || !QFile::exists(QDir(backupPath).filePath("database.db"))
        || maintenance.restoreBackup(QDir(sessionRoot).filePath("missing.niuma"))) return 5;
    if (!clear.exec("DELETE FROM task") || !maintenance.restoreBackup(backupPath)) return 6;
    TaskDao taskDao(DatabaseManager::instance().database());
    if (!taskDao.findByTaskNo(QStringLiteral("20260922-900")).has_value()) return 7;

    const QString scanFolder = QDir(workDir).filePath(
        FileService::kDirInProgress + QStringLiteral("/20260922-001_[查]_扫描导入任务"));
    if (!QDir().mkpath(scanFolder)) return 8;
    ScanImportDialog scanDialog;
    scanDialog.show();
    app.processEvents();
    closeMessage(true);
    if (!QMetaObject::invokeMethod(&scanDialog, "onScanClicked", Qt::DirectConnection)) return 9;
    auto *scanTable = scanDialog.findChild<QTableWidget *>("scanImportTable");
    if (!scanTable || scanTable->rowCount() != 1 || scanTable->item(0, 4)->text() != QStringLiteral("待导入")) return 10;
    int success = -1;
    QObject::connect(&scanDialog, &ScanImportDialog::importCompleted,
                     [&](int successCount, int, int) { success = successCount; });
    closeMessage(true);
    if (!QMetaObject::invokeMethod(&scanDialog, "onImportClicked", Qt::DirectConnection)
        || success != 1 || !taskDao.findByTaskNo(QStringLiteral("20260922-001")).has_value()) return 11;
    if (scanTable->rowCount() != 1
        || scanTable->item(0, 4)->text() != QStringLiteral("已导入")
        || !QMetaObject::invokeMethod(&scanDialog, "onFinishClicked", Qt::DirectConnection)
        || scanDialog.result() != QDialog::Accepted) return 81;

    // 新增目录在下次启动时自动导入，信息文件恢复详细字段，附件只识别结构。
    const QString incoming = QDir(workDir).filePath(
        FileService::kDirInProgress + QStringLiteral("/20260922-777_[查]_迁移任务"));
    if (!QDir().mkpath(QDir(incoming).filePath(QStringLiteral("附件子目录")))) return 15;
    Task details;
    details.taskNo = QStringLiteral("20260922-777");
    details.title = QStringLiteral("迁移任务");
    details.category = QStringLiteral("查");
    details.source = QStringLiteral("来源甲");
    details.tag = QStringLiteral("标签乙");
    details.description = QStringLiteral("完整说明");
    details.isKeyTask = true;
    details.deadline = QDate(2026, 10, 9);
    details.folderPath = incoming;
    if (!TaskInfoFile::write(details)) return 16;
    const StartupResult rescan = StartupUseCase().prepare([&]() { return QString(); });
    const auto imported = taskDao.findByTaskNo(details.taskNo);
    if (rescan.importedTaskCount != 1 || !imported
        || imported->source != details.source || imported->tag != details.tag
        || imported->deadline != details.deadline || !imported->hasSubdirs
        || !imported->isKeyTask || !TaskInfoFile::read(incoming)->isKeyTask) return 17;

    // 资料 info 同样恢复详情；没有 info 时仅从合法目录名恢复编号与名称。
    const QString refFolder = QDir(workDir).filePath(
        FileService::kDirReference + QStringLiteral("/20260922-782_迁移资料"));
    if (!QDir().mkpath(refFolder)) return 47;
    ReferenceMaterial refDetails;
    refDetails.refNo = QStringLiteral("20260922-782");
    refDetails.name = QStringLiteral("迁移资料");
    refDetails.source = QStringLiteral("资料来源");
    refDetails.tag = QStringLiteral("资料标签");
    refDetails.description = QStringLiteral("资料说明");
    refDetails.folderPath = refFolder;
    if (!ReferenceInfoFile::write(refDetails)) return 48;
    ScanMatchService scanner;
    ScanImportItem refItem;
    for (const ScanImportItem &item : scanner.scanForImport(workDir))
        if (item.folderPath == refFolder) refItem = item;
    if (refItem.status != QStringLiteral("待导入")
        || !scanner.importScanItem(refItem, workDir).ok) return 49;
    ReferenceMaterialDao refDao(DatabaseManager::instance().database());
    const auto importedRef = refDao.findByRefNo(refDetails.refNo);
    if (!importedRef || importedRef->source != refDetails.source
        || importedRef->tag != refDetails.tag
        || importedRef->description != refDetails.description) return 50;

    ReferenceMaterial changedRefInfo = *importedRef;
    changedRefInfo.name = QStringLiteral("另一个资料名");
    changedRefInfo.source = QStringLiteral("另一个资料来源");
    if (!ReferenceInfoFile::write(changedRefInfo)) return 61;
    ScanImportDialog refFieldDialog;
    refFieldDialog.show();
    if (!QMetaObject::invokeMethod(&refFieldDialog, "onScanClicked", Qt::DirectConnection)) return 62;
    auto *refTable = refFieldDialog.findChild<QTableWidget *>("scanImportTable");
    int refRow = -1;
    for (int i = 0; refTable && i < refTable->rowCount(); ++i)
        if (refTable->item(i, 1)->text() == QFileInfo(refFolder).fileName()) refRow = i;
    if (refRow < 0) return 63;
    refTable->setCurrentCell(refRow, 0);
    bool refPickerVerified = false;
    QTimer::singleShot(0, [&]() {
        auto *picker = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        if (!picker || picker == &refFieldDialog) return;
        auto *name = picker->findChild<QComboBox *>("scanConflictField_name");
        auto *number = picker->findChild<QComboBox *>("scanConflictField_ref_no");
        auto *source = picker->findChild<QComboBox *>("scanConflictField_source");
        auto *tag = picker->findChild<QComboBox *>("scanConflictField_tag");
        refPickerVerified = name && number && source && tag && name->isEnabled()
            && name->count() == 2 && name->itemText(0) == QStringLiteral("迁移资料")
            && name->itemText(1) == QStringLiteral("另一个资料名")
            && !number->isEnabled() && number->count() == 1
            && source->isEnabled() && source->count() == 2
            && source->itemText(0) == QStringLiteral("资料来源")
            && source->itemText(1) == QStringLiteral("另一个资料来源")
            && !tag->isEnabled() && tag->count() == 1
            && picker->grab().save(QDir(root).filePath(QStringLiteral("scan-reference-field-picker.png")));
        picker->reject();
    });
    if (!QMetaObject::invokeMethod(&refFieldDialog, "onResolveClicked", Qt::DirectConnection)
        || !refPickerVerified || !ReferenceInfoFile::write(*importedRef)) return 64;
    refFieldDialog.close();

    Task alteredInfo = details;
    alteredInfo.title = QStringLiteral("错误的 info 名称");
    if (!TaskInfoFile::write(alteredInfo)) return 37;
    ScanImportItem sameFolderConflict;
    for (const ScanImportItem &item : scanner.scanForImport(workDir))
        if (item.folderPath == incoming) sameFolderConflict = item;
    ScanImportDialog taskFieldDialog;
    taskFieldDialog.show();
    if (!QMetaObject::invokeMethod(&taskFieldDialog, "onScanClicked", Qt::DirectConnection)) return 65;
    auto *taskTable = taskFieldDialog.findChild<QTableWidget *>("scanImportTable");
    int taskRow = -1;
    for (int i = 0; taskTable && i < taskTable->rowCount(); ++i)
        if (taskTable->item(i, 1)->text() == QFileInfo(incoming).fileName()) taskRow = i;
    if (taskRow < 0) return 66;
    taskTable->setCurrentCell(taskRow, 0);
    bool taskPickerVerified = false;
    QTimer::singleShot(0, [&]() {
        auto *picker = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        if (!picker || picker == &taskFieldDialog) return;
        auto *title = picker->findChild<QComboBox *>("scanConflictField_title");
        auto *number = picker->findChild<QComboBox *>("scanConflictField_task_no");
        auto *tag = picker->findChild<QComboBox *>("scanConflictField_tag");
        taskPickerVerified = title && number && tag && title->isEnabled()
            && title->count() == 2 && title->itemText(0) == QStringLiteral("迁移任务")
            && title->itemText(1) == QStringLiteral("错误的 info 名称")
            && !number->isEnabled() && number->count() == 1
            && !tag->isEnabled() && tag->count() == 1
            && picker->grab().save(QDir(root).filePath(QStringLiteral("scan-task-field-picker.png")));
        picker->reject();
    });
    if (!QMetaObject::invokeMethod(&taskFieldDialog, "onResolveClicked", Qt::DirectConnection)
        || !taskPickerVerified) return 67;
    taskFieldDialog.close();
    if (!sameFolderConflict.isConflict || sameFolderConflict.existingId != imported->id
        || !scanner.resolveConflict(sameFolderConflict, false,
            {{QStringLiteral("title"), QStringLiteral("folder")}}).ok
        || TaskInfoFile::read(incoming)->title != details.title) return 38;

    // 同编号目录不静默跳过。逐项合并字段且保留两个同名附件。
    const QString duplicate = QDir(workDir).filePath(
        FileService::kDirArchived + QStringLiteral("/20260922-777_[查]_迁移任务"));
    if (!QDir().mkpath(duplicate)) return 18;
    Task duplicateInfo = details;
    duplicateInfo.id = imported->id;
    duplicateInfo.folderPath = duplicate;
    duplicateInfo.tag = QStringLiteral("合并标签");
    AchievementDao achievements(DatabaseManager::instance().database());
    Achievement incomingAchievement;
    incomingAchievement.taskId = imported->id;
    incomingAchievement.workContent = QStringLiteral("合并目录中的成果");
    incomingAchievement.resultAmount = 12599;
    incomingAchievement.createTime = QDateTime(QDate(2022, 6, 1), QTime(10, 30));
    const int incomingAchievementId = achievements.insert(incomingAchievement);
    if (incomingAchievementId <= 0 || !TaskInfoFile::write(duplicateInfo)
        || !achievements.deleteById(incomingAchievementId)) return 19;
    QFile existingAttachment(QDir(incoming).filePath(QStringLiteral("相同.txt")));
    if (!existingAttachment.open(QIODevice::WriteOnly) || existingAttachment.write("old") != 3) return 20;
    existingAttachment.close();
    QFile incomingAttachment(QDir(duplicate).filePath(QStringLiteral("相同.txt")));
    if (!incomingAttachment.open(QIODevice::WriteOnly) || incomingAttachment.write("new") != 3) return 21;
    incomingAttachment.close();
    ScanImportItem conflict;
    for (const ScanImportItem &item : scanner.scanForImport(workDir))
        if (item.folderPath == duplicate) conflict = item;
    if (!conflict.isConflict || conflict.existingId != imported->id) return 22;
    const TaskResult merged = scanner.resolveConflict(conflict, true,
        {{QStringLiteral("tag"), QStringLiteral("info")}});
    const auto afterMerge = taskDao.findById(imported->id);
    const auto mergedAchievement = achievements.findByTaskId(imported->id);
    if (!merged.ok || !afterMerge || afterMerge->tag != QStringLiteral("合并标签")
        || !mergedAchievement || mergedAchievement->workContent != incomingAchievement.workContent
        || mergedAchievement->resultAmount != incomingAchievement.resultAmount
        || mergedAchievement->createTime != incomingAchievement.createTime
        || !QFile::exists(QDir(incoming).filePath(QStringLiteral("相同 (合并来源1).txt")))) return 23;

    // 简易备份仅保留 info 和目录结构；全量备份还保留附件。恢复模式跟随清单。
    const QString simple = BackupService().manualBackupTo(QDir(sessionRoot).filePath("simple-backups"));
    const QString full = BackupService().manualBackupTo(
        QDir(sessionRoot).filePath("full-backups"), BackupService::BackupMode::Full);
    const QString relative = QDir(workDir).relativeFilePath(incoming);
    if (simple.isEmpty() || full.isEmpty()
        || BackupService::readNiumaMeta(simple).value(QStringLiteral("backup_type")) != QStringLiteral("simple")
        || BackupService::readNiumaMeta(full).value(QStringLiteral("backup_type")) != QStringLiteral("full")
        || !QFile::exists(QDir(simple).filePath(QStringLiteral("workdir/") + relative + QStringLiteral("/.niuma-task.json")))
        || !QFile::exists(QDir(simple).filePath(QStringLiteral("workdir/")
            + QDir(workDir).relativeFilePath(refFolder) + QStringLiteral("/.niuma-reference.json")))
        || QFile::exists(QDir(simple).filePath(QStringLiteral("workdir/") + relative + QStringLiteral("/相同.txt")))
        || !QFile::exists(QDir(full).filePath(QStringLiteral("workdir/") + relative + QStringLiteral("/相同.txt")))) return 24;
    if (!BackupService().restoreFromBackup(simple)
        || QFile::exists(QDir(incoming).filePath(QStringLiteral("相同.txt")))
        || !QDir(QDir(incoming).filePath(QStringLiteral("附件子目录"))).exists()) return 25;
    if (!BackupService().restoreFromBackup(full)
        || !QFile::exists(QDir(incoming).filePath(QStringLiteral("相同.txt")))) return 26;

    // 搬迁完整工作目录：原路径失效时重新选择，旧数据库路径整体重定位，
    // 同时导入新电脑合并进来的有效目录。
    const QString movedParent = QDir(sessionRoot).filePath(QStringLiteral("moved-location"));
    const QString movedWorkDir = QDir(movedParent).filePath(QStringLiteral("工作目录"));
    if (!FileUtil::copyDir(workDir, movedWorkDir)
        || !QDir().rename(workDir, QDir(sessionRoot).filePath(QStringLiteral("old-workdir")))) return 27;
    const QString newlyCopied = QDir(movedWorkDir).filePath(
        FileService::kDirInProgress + QStringLiteral("/20260922-778_[查]_外部复制"));
    if (!QDir().mkpath(newlyCopied)) return 28;
    bool relocationPrompted = false;
    const StartupResult relocated = StartupUseCase().prepare([&]() {
        relocationPrompted = true;
        return movedParent;
    });
    const auto relocatedTask = taskDao.findByTaskNo(details.taskNo);
    if (!relocationPrompted || relocated.workDir != movedWorkDir
        || relocated.importedTaskCount != 1 || !relocatedTask
        || relocatedTask->folderPath != QDir(movedWorkDir).filePath(relative)
        || !taskDao.findByTaskNo(QStringLiteral("20260922-778"))) return 29;

    const QString pairA = QDir(movedWorkDir).filePath(
        FileService::kDirInProgress + QStringLiteral("/20260922-780_[查]_合并目标"));
    const QString pairB = QDir(movedWorkDir).filePath(
        FileService::kDirArchived + QStringLiteral("/20260922-780_[查]_合并来源"));
    if (!QDir().mkpath(pairA) || !QDir().mkpath(pairB)) return 39;
    ScanImportItem pairConflict;
    for (const ScanImportItem &item : scanner.scanForImport(movedWorkDir))
        if (item.folderPath == pairB) pairConflict = item;
    if (!pairConflict.isConflict || pairConflict.existingId != 0
        || !scanner.resolveConflict(pairConflict, true,
            {{QStringLiteral("title"), QStringLiteral("folder")}}).ok
        || !taskDao.findByTaskNo(QStringLiteral("20260922-780"))) return 40;

    const QString keepFolder = QDir(movedWorkDir).filePath(
        FileService::kDirArchived + QStringLiteral("/20260922-777_[查]_另一副本"));
    if (!QDir().mkpath(keepFolder)) return 30;
    ScanImportItem keepConflict;
    for (const ScanImportItem &item : scanner.scanForImport(movedWorkDir))
        if (item.folderPath == keepFolder) keepConflict = item;
    if (!keepConflict.isConflict) return 31;
    const TaskResult kept = scanner.resolveConflict(keepConflict, false,
        {{QStringLiteral("title"), QStringLiteral("folder")}});
    const auto keptTask = taskDao.findById(kept.taskId);
    if (!kept.ok || !keptTask || keptTask->taskNo == details.taskNo
        || keptTask->title != QStringLiteral("另一副本")
        || !QDir(keptTask->folderPath).exists()) return 32;

    const QString mismatch = QDir(movedWorkDir).filePath(
        FileService::kDirInProgress + QStringLiteral("/20260922-779_[查]_目录名称"));
    if (!QDir().mkpath(mismatch)) return 33;
    Task mismatchInfo = details;
    mismatchInfo.taskNo = QStringLiteral("20260922-779");
    mismatchInfo.title = QStringLiteral("info名称");
    mismatchInfo.tag = QStringLiteral("info标签");
    mismatchInfo.folderPath = mismatch;
    if (!TaskInfoFile::write(mismatchInfo)) return 34;
    ScanImportItem mismatchConflict;
    for (const ScanImportItem &item : scanner.scanForImport(movedWorkDir))
        if (item.folderPath == mismatch) mismatchConflict = item;
    if (!mismatchConflict.isConflict) return 35;
    ScanImportDialog conflictPreview;
    conflictPreview.show();
    if (!QMetaObject::invokeMethod(&conflictPreview, "onScanClicked", Qt::DirectConnection)) return 41;
    app.processEvents();
    auto *conflictTable = conflictPreview.findChild<QTableWidget *>("scanImportTable");
    int conflictRow = -1;
    for (int i = 0; conflictTable && i < conflictTable->rowCount(); ++i)
        if (conflictTable->item(i, 1)->text() == QFileInfo(mismatch).fileName()) conflictRow = i;
    if (conflictRow < 0 || !(conflictTable->item(conflictRow, 0)->flags() & Qt::ItemIsEnabled))
        return 51;
    conflictTable->item(conflictRow, 0)->setCheckState(Qt::Unchecked);
    if (!QMetaObject::invokeMethod(&conflictPreview, "onSelectAllClicked", Qt::DirectConnection)
        || conflictTable->item(conflictRow, 0)->checkState() != Qt::Checked) return 52;
    if (!conflictPreview.grab().save(QDir(root).filePath(QStringLiteral("scan-conflict-preview.png")))) return 42;
    conflictPreview.close();
    const TaskResult reconciled = scanner.resolveConflict(mismatchConflict, false,
        {{QStringLiteral("title"), QStringLiteral("folder")},
         {QStringLiteral("tag"), QStringLiteral("info")}});
    const auto corrected = taskDao.findById(reconciled.taskId);
    if (!reconciled.ok || !corrected
        || corrected->title != QStringLiteral("目录名称")
        || corrected->tag != QStringLiteral("info标签")
        || !TaskInfoFile::read(corrected->folderPath)
        || TaskInfoFile::read(corrected->folderPath)->title != corrected->title) return 36;

    const QString metadataFolder = QDir(movedWorkDir).filePath(
        FileService::kDirInProgress + QStringLiteral("/20260922-781_[查]_目录字段"));
    if (!QDir().mkpath(metadataFolder)) return 44;
    Task metadataInfo = details;
    metadataInfo.taskNo = QStringLiteral("20260922-781");
    metadataInfo.title = QStringLiteral("info字段");
    metadataInfo.tag = QStringLiteral("保留标签");
    metadataInfo.folderPath = metadataFolder;
    if (!TaskInfoFile::write(metadataInfo)) return 45;
    ScanImportItem metadataConflict;
    for (const ScanImportItem &item : scanner.scanForImport(movedWorkDir))
        if (item.folderPath == metadataFolder) metadataConflict = item;
    const TaskResult metadataMerged = scanner.resolveConflict(metadataConflict, true,
        {{QStringLiteral("title"), QStringLiteral("folder")},
         {QStringLiteral("tag"), QStringLiteral("info")}});
    const auto metadataRecovered = taskDao.findById(metadataMerged.taskId);
    if (!metadataConflict.isConflict || !metadataMerged.ok || !metadataRecovered
        || metadataRecovered->title != QStringLiteral("目录字段")
        || metadataRecovered->tag != QStringLiteral("保留标签")) return 46;

    const QString refMismatch = QDir(movedWorkDir).filePath(
        FileService::kDirReference + QStringLiteral("/20260922-783_目录资料"));
    if (!QDir().mkpath(refMismatch)) return 53;
    ReferenceMaterial refMismatchInfo = refDetails;
    refMismatchInfo.refNo = QStringLiteral("20260922-783");
    refMismatchInfo.name = QStringLiteral("info资料");
    refMismatchInfo.folderPath = refMismatch;
    if (!ReferenceInfoFile::write(refMismatchInfo)) return 54;
    ScanImportItem refConflict;
    for (const ScanImportItem &item : scanner.scanForImport(movedWorkDir))
        if (item.folderPath == refMismatch) refConflict = item;
    const TaskResult refResolved = scanner.resolveReferenceConflict(refConflict, true,
        {{QStringLiteral("name"), QStringLiteral("folder")},
         {QStringLiteral("source"), QStringLiteral("info")}});
    const auto restoredRef = refDao.findById(refResolved.taskId);
    if (!refConflict.isConflict || !refResolved.ok || !restoredRef
        || restoredRef->name != QStringLiteral("目录资料")
        || restoredRef->source != refDetails.source
        || !ReferenceInfoFile::read(restoredRef->folderPath)) return 55;

    const QString bareRefFolder = QDir(movedWorkDir).filePath(
        FileService::kDirReference + QStringLiteral("/20260922-784_无详情资料"));
    if (!QDir().mkpath(bareRefFolder)) return 56;
    ScanImportItem bareRefItem;
    for (const ScanImportItem &item : scanner.scanForImport(movedWorkDir))
        if (item.folderPath == bareRefFolder) bareRefItem = item;
    const TaskResult bareImported = scanner.importScanItem(bareRefItem, movedWorkDir);
    const auto bareRef = refDao.findById(bareImported.taskId);
    if (!bareImported.ok || !bareRef || bareRef->name != QStringLiteral("无详情资料")
        || !bareRef->source.isEmpty() || !bareRef->tag.isEmpty()
        || !ReferenceInfoFile::read(bareRefFolder)) return 57;

    const QString duplicateRefFolder = QDir(movedWorkDir).filePath(
        FileService::kDirReference + QStringLiteral("/20260922-782_第二资料"));
    if (!QDir().mkpath(duplicateRefFolder)) return 58;
    ReferenceMaterial duplicateRefInfo = refDetails;
    duplicateRefInfo.name = QStringLiteral("第二资料");
    duplicateRefInfo.tag = QStringLiteral("合并后的资料标签");
    duplicateRefInfo.folderPath = duplicateRefFolder;
    if (!ReferenceInfoFile::write(duplicateRefInfo)) return 59;
    ScanImportItem duplicateRefConflict;
    for (const ScanImportItem &item : scanner.scanForImport(movedWorkDir))
        if (item.folderPath == duplicateRefFolder) duplicateRefConflict = item;
    const TaskResult duplicateRefMerged = scanner.resolveReferenceConflict(
        duplicateRefConflict, true,
        {{QStringLiteral("tag"), QStringLiteral("info")}});
    const auto mergedRef = refDao.findById(importedRef->id);
    if (!duplicateRefConflict.isConflict || !duplicateRefMerged.ok || !mergedRef
        || mergedRef->tag != QStringLiteral("合并后的资料标签")
        || QDir(duplicateRefFolder).exists()) return 60;

    // 无数据库时按工作区恢复状态；归档只扫描年/月层级，不能把附件子目录当任务。
    const QString activeFolder = QDir(movedWorkDir).filePath(
        FileService::kDirInProgress + QStringLiteral("/20260922-785_[查]_恢复进行中"));
    const QString archivedFolder = QDir(movedWorkDir).filePath(
        FileService::kDirArchived + QStringLiteral("/2026/09/20260922-786_[办]_恢复已完成"));
    const QString numberedAttachment = QDir(archivedFolder).filePath(
        QStringLiteral("20260922-799_[查]_附件目录"));
    const QString deletedFolder = QDir(movedWorkDir).filePath(
        FileService::kDirRecycleBin + QStringLiteral("/20260922-787_[查]_恢复回收站"));
    const QString unknownHandlerFolder = QDir(movedWorkDir).filePath(
        FileService::kDirOtherHandler + QStringLiteral("/20260922-788_[报]_未知办理人"));
    const QString knownHandlerFolder = QDir(movedWorkDir).filePath(
        FileService::kDirOtherHandler + QStringLiteral("/20260922-789_[办]_明确办理人"));
    const QString deletedOtherFolder = QDir(movedWorkDir).filePath(
        FileService::kDirRecycleBin + QStringLiteral("/20260922-790_[查]_删除前他人"));
    for (const QString &path : {activeFolder, numberedAttachment, deletedFolder,
                                unknownHandlerFolder, knownHandlerFolder, deletedOtherFolder})
        if (!QDir().mkpath(path)) return 68;
    Task knownHandler;
    knownHandler.taskNo = QStringLiteral("20260922-789");
    knownHandler.title = QStringLiteral("明确办理人");
    knownHandler.category = QStringLiteral("办");
    knownHandler.handler = QStringLiteral("张三");
    knownHandler.folderPath = knownHandlerFolder;
    if (!TaskInfoFile::write(knownHandler)) return 69;
    Task deletedOther;
    deletedOther.taskNo = QStringLiteral("20260922-790");
    deletedOther.title = QStringLiteral("删除前他人");
    deletedOther.category = QStringLiteral("查");
    deletedOther.handler.clear();
    deletedOther.originalStatus = QStringLiteral("他人办理");
    deletedOther.deleteTime = QDateTime::currentDateTime().addDays(-40);
    deletedOther.folderPath = deletedOtherFolder;
    if (!TaskInfoFile::write(deletedOther)) return 70;
    const QList<ScanImportItem> statusCandidates = scanner.scanForImport(movedWorkDir);
    if (std::any_of(statusCandidates.cbegin(), statusCandidates.cend(),
                    [&](const ScanImportItem &item) { return item.folderPath == numberedAttachment; }))
        return 71;
    const StartupResult statusRescan = StartupUseCase().prepare([&]() { return QString(); });
    const auto activeRecovered = taskDao.findByTaskNo(QStringLiteral("20260922-785"));
    const auto archivedRecovered = taskDao.findByTaskNo(QStringLiteral("20260922-786"));
    const auto deletedRecovered = taskDao.findByTaskNo(QStringLiteral("20260922-787"));
    const auto unknownRecovered = taskDao.findByTaskNo(QStringLiteral("20260922-788"));
    const auto knownRecovered = taskDao.findByTaskNo(QStringLiteral("20260922-789"));
    const auto deletedOtherRecovered = taskDao.findByTaskNo(QStringLiteral("20260922-790"));
    if (statusRescan.importedTaskCount < 6 || !activeRecovered || !archivedRecovered
        || !deletedRecovered || !unknownRecovered || !knownRecovered || !deletedOtherRecovered
        || activeRecovered->status != QStringLiteral("进行中")
        || archivedRecovered->status != QStringLiteral("已完成")
        || deletedRecovered->status != QStringLiteral("已删除")
        || unknownRecovered->status != QStringLiteral("他人办理")
        || unknownRecovered->handler != QStringLiteral("非本人")
        || knownRecovered->status != QStringLiteral("他人办理")
        || knownRecovered->handler != QStringLiteral("张三")
        || deletedOtherRecovered->status != QStringLiteral("已删除")
        || deletedOtherRecovered->originalStatus != QStringLiteral("他人办理")
        || deletedOtherRecovered->handler != QStringLiteral("非本人")
        || !TaskInfoFile::read(unknownHandlerFolder)
        || TaskInfoFile::read(unknownHandlerFolder)->handler != QStringLiteral("非本人")) return 72;
    const QList<Task> recycleRows = RecycleBinUseCase().listDeleted();
    if (std::none_of(recycleRows.cbegin(), recycleRows.cend(),
                     [&](const Task &task) { return task.id == deletedRecovered->id; })
        || std::none_of(recycleRows.cbegin(), recycleRows.cend(),
                        [&](const Task &task) { return task.id == deletedOtherRecovered->id; }))
        return 73;
    if (!RecycleBinUseCase().restoreTask(deletedOtherRecovered->id).ok) return 74;
    const auto restoredOther = taskDao.findById(deletedOtherRecovered->id);
    if (!restoredOther || restoredOther->status != QStringLiteral("他人办理")
        || restoredOther->handler != QStringLiteral("非本人")
        || !QDir(restoredOther->folderPath).exists()
        || QDir(deletedOtherFolder).exists()) return 75;
    for (const ScanImportItem &item : scanner.scanForImport(movedWorkDir)) {
        if (item.folderPath == archivedFolder || item.folderPath == deletedFolder
            || item.folderPath == unknownHandlerFolder) {
            if (!item.isDuplicate || item.isConflict) return 76;
        }
    }

    // 软件外改名后，合并采用新目录名称并更新原记录，不保留旧信息文件。
    const auto renameBefore = taskDao.findByTaskNo(QStringLiteral("20260922-785"));
    if (!renameBefore || !QDir(renameBefore->folderPath).exists()) return 77;
    const QString renamedPath = QDir(QFileInfo(renameBefore->folderPath).absolutePath())
        .filePath(QStringLiteral("20260922-785_[查]_外部改名后"));
    if (!QDir().rename(renameBefore->folderPath, renamedPath)) return 78;
    ScanImportItem renamedConflict;
    for (const ScanImportItem &item : scanner.scanForImport(movedWorkDir))
        if (item.folderPath == renamedPath) renamedConflict = item;
    if (!renamedConflict.isConflict || renamedConflict.existingId != renameBefore->id) return 79;
    const TaskResult renamedMerge = scanner.resolveConflict(renamedConflict, true,
        {{QStringLiteral("title"), QStringLiteral("folder")}});
    const auto renameAfter = taskDao.findById(renameBefore->id);
    if (!renamedMerge.ok || !renameAfter || renameAfter->title != QStringLiteral("外部改名后")
        || renameAfter->folderPath != renamedPath || QDir(renameBefore->folderPath).exists()
        || !TaskInfoFile::read(renamedPath)
        || TaskInfoFile::read(renamedPath)->title != QStringLiteral("外部改名后")) return 80;

    SettingsView settings;
    settings.show();
    app.processEvents();
    closeMessage(false);
    if (!QMetaObject::invokeMethod(&settings, "onInitClicked", Qt::DirectConnection)
        || !taskDao.findByTaskNo(QStringLiteral("20260922-001")).has_value()) return 12;
    closeMessage(true);
    if (!QMetaObject::invokeMethod(&settings, "onInitClicked", Qt::DirectConnection)
        || taskDao.findByTaskNo(QStringLiteral("20260922-001")).has_value()) return 13;
    SettingsDao settingsDao(DatabaseManager::instance().database());
    if (settingsDao.hasKey(QStringLiteral("work_dir"))) return 14;
    return 0;
}
