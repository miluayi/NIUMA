#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlQuery>
#include <algorithm>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "DatabaseManager.h"
#include "application/task/TaskDetailUseCase.h"
#include "application/task/TaskFormUseCase.h"
#include "application/task/TaskQueryUseCase.h"
#include "application/home/HomeCommandUseCase.h"
#include "application/home/HomeDashboardUseCase.h"
#include "service/TaskService.h"
#include "service/FileService.h"
#include "service/ScanMatchService.h"
#include "service/TaskInfoFile.h"
#include "utils/PathUtil.h"
#include "attachment_search_contract.h"

namespace {
int verifyCompletedQueryContract()
{
    // Shadow only this test connection's table to exercise legacy nullable handlers.
    // The production schema and the lifecycle fixtures in main.task stay untouched.
    const QSqlDatabase database = DatabaseManager::instance().database();
    QSqlQuery query(database);
    if (!query.exec("CREATE TEMP TABLE task AS SELECT * FROM main.task WHERE 0")) return 50;
    const int year = QDate::currentDate().year();
    const QString datePrefix = QStringLiteral("%1-06-").arg(year);
    for (int index = 1; index <= 22; ++index) {
        QVariant finish;
        switch (index) {
        case 1: finish = datePrefix + QStringLiteral("15T08:00:00"); break;
        case 2: finish = datePrefix + QStringLiteral("15 20:00:00"); break;
        case 3: finish = datePrefix + QStringLiteral("15 12:34:56"); break;
        case 4: finish = datePrefix + QStringLiteral("15T12:34:56"); break;
        case 5: finish = datePrefix + QStringLiteral("15 12:34:55"); break;
        case 6: break;
        case 7: finish = QStringLiteral(""); break;
        default: finish = datePrefix + QStringLiteral("14 12:00:%1").arg(index, 2, 10, QChar('0')); break;
        }
        query.prepare("INSERT INTO task (id,task_no,title,category,source,tag,handler,status,deadline,"
                      "create_time,finish_time) VALUES (?,?,?,?,?,?,?,?,?,?,?)");
        query.addBindValue(index);
        query.addBindValue(QStringLiteral("ORDER-%1").arg(index, 3, 10, QChar('0')));
        query.addBindValue(index == 18 ? QStringLiteral("不匹配标题") : QStringLiteral("完成排序"));
        query.addBindValue(index == 21 ? QStringLiteral("查") : QStringLiteral("催"));
        query.addBindValue(index == 15 ? QStringLiteral("其它来源") : QStringLiteral("排序来源"));
        query.addBindValue(index == 20 ? QStringLiteral("其它标签") : QStringLiteral("排序标签"));
        if (index == 6) query.addBindValue(QVariant());
        else query.addBindValue(index == 7 ? QStringLiteral("")
                                   : index == 16 ? QStringLiteral("本人")
                                   : index == 2 ? QStringLiteral("非本人") : QStringLiteral("张三"));
        query.addBindValue(index == 19 ? QStringLiteral("进行中") : QStringLiteral("已完成"));
        query.addBindValue(QDate::currentDate().addDays(index).toString(Qt::ISODate));
        query.addBindValue(QStringLiteral("%1-%2-10 12:00:00")
                               .arg(index == 17 ? year - 1 : year)
                               .arg(index == 22 ? 7 : 6, 2, 10, QChar('0')));
        query.addBindValue(finish);
        if (!query.exec()) return 51;
    }
    TaskFilter filter;
    filter.keyword = QStringLiteral("完成排序");
    filter.category = QStringLiteral("催");
    filter.source = QStringLiteral("排序来源");
    filter.tag = QStringLiteral("排序标签");
    filter.handler = QStringLiteral("非本人");
    filter.status = QStringLiteral("已完成");
    filter.year = year;
    filter.startDate = QDate(year, 6, 1);
    filter.endDate = QDate(year, 6, 30);
    filter.pageSize = 5;
    // Reproduce mixed-format completion ordering independently of the new aggregate handler filter.
    TaskFilter namedMixedFilter = filter;
    namedMixedFilter.handler = QStringLiteral("张三");
    namedMixedFilter.pageSize = 0;
    const auto namedMixed = TaskQueryUseCase().loadList(namedMixedFilter);
    const QStringList expectedNamedFirst = {QStringLiteral("ORDER-004"), QStringLiteral("ORDER-003"),
        QStringLiteral("ORDER-005"), QStringLiteral("ORDER-001")};
    if (namedMixed.total != 11 || namedMixed.tasks.size() != 11) return 62;
    for (int index = 0; index < expectedNamedFirst.size(); ++index)
        if (namedMixed.tasks.at(index).taskNo != expectedNamedFirst.at(index)) return 62;
    const QStringList expected = {
        QStringLiteral("ORDER-002"), QStringLiteral("ORDER-004"), QStringLiteral("ORDER-003"),
        QStringLiteral("ORDER-005"), QStringLiteral("ORDER-001"), QStringLiteral("ORDER-014"),
        QStringLiteral("ORDER-013"), QStringLiteral("ORDER-012"), QStringLiteral("ORDER-011"),
        QStringLiteral("ORDER-010"), QStringLiteral("ORDER-009"), QStringLiteral("ORDER-008"),
        QStringLiteral("ORDER-007"), QStringLiteral("ORDER-006")
    };
    for (bool deadlineRequested : {false, true}) {
        filter.sortByDeadline = deadlineRequested;
        QStringList observed;
        for (int pageNumber = 1; pageNumber <= 3; ++pageNumber) {
            filter.page = pageNumber;
            const auto page = TaskQueryUseCase().loadList(filter);
            if (page.total != expected.size() || page.tasks.size() != (pageNumber == 3 ? 4 : 5))
                return deadlineRequested ? 53 : 52;
            for (const Task &task : page.tasks) {
                observed.append(task.taskNo);
                const bool missing = task.taskNo == QStringLiteral("ORDER-006")
                                     || task.taskNo == QStringLiteral("ORDER-007");
                if (task.finishTime.isValid() == missing) return 54;
                if (task.taskNo == QStringLiteral("ORDER-001")
                    && task.finishTime != QDateTime(QDate(year, 6, 15), QTime(8, 0, 0))) return 55;
            }
        }
        if (observed != expected) return deadlineRequested ? 57 : 56;
    }
    filter.page = 1;
    filter.pageSize = 0;
    filter.handler = QStringLiteral("张三");
    const auto named = TaskQueryUseCase().loadList(filter);
    if (named.total != 11 || named.tasks.size() != 11) return 58;
    for (const Task &task : named.tasks) if (task.handler != QStringLiteral("张三")) return 59;
    filter.source = QStringLiteral("不存在的来源");
    const auto empty = TaskQueryUseCase().loadList(filter);
    if (empty.total != 0 || !empty.tasks.isEmpty()) return 60;
    if (!query.exec("DROP TABLE temp.task")) return 61;
    return 0;
}
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    const QString root = qEnvironmentVariable("NIUMA_TEST_ROOT");
    if (root.isEmpty()) return 1;
    qputenv("NIUMA_DATA_DIR", QDir(root).filePath("Data").toUtf8());
    if (!QDir().mkpath(root) || !DatabaseManager::instance().initialize(PathUtil::databasePath())) return 2;
    const QString workRoot = QDir(root).filePath(QStringLiteral("work-root"));
    if (QDir(workRoot).exists() && !QDir(workRoot).removeRecursively()) return 33;
    if (!FileService().setWorkDir(QString())) return 34;
    QSqlQuery clear(DatabaseManager::instance().database());
    if (!clear.exec("DELETE FROM task") || !clear.exec("DELETE FROM source_dict")
        || !clear.exec("DELETE FROM tag_dict") || !clear.exec("DELETE FROM handler_dict")) return 3;

    TaskFormUseCase form;
    form.ensureDictionaryEntries(QStringLiteral("S13 来源"), QStringLiteral("S13 标签"),
                                 QStringLiteral("S13 办理人"));
    const TaskFormOptions options = TaskQueryUseCase().loadFormOptions();
    if (!options.sources.contains(QStringLiteral("S13 来源"))
        || !options.tags.contains(QStringLiteral("S13 标签"))
        || !options.handlers.contains(QStringLiteral("S13 办理人"))) return 4;

    Task task;
    task.title = QStringLiteral("S13 查询任务");
    task.category = QStringLiteral("查");
    task.source = QStringLiteral("S13 来源");
    task.tag = QStringLiteral("S13 标签");
    // 使用“本人”以覆盖可完成的任务生命周期；字典选项仍在上方独立验证。
    task.handler = QStringLiteral("本人");
    if (!TaskService().createTask(task).ok) return 5;

    form.ensureDictionaryEntries(QStringLiteral("S13 最近来源"),
                                 QStringLiteral("S13 最近标签"),
                                 QStringLiteral("S13 最近办理人"));
    form.ensureDictionaryEntries(QString(), QString(), QString());
    Task recentTask = task;
    recentTask.title = QStringLiteral("S13 最近来源任务");
    recentTask.source = QStringLiteral("S13 最近来源");
    recentTask.tag = QStringLiteral("S13 最近标签");
    recentTask.handler = QStringLiteral("S13 最近办理人");
    if (!TaskService().createTask(recentTask).ok) return 11;
    QSqlQuery ageFirstTask(DatabaseManager::instance().database());
    ageFirstTask.prepare("UPDATE task SET create_time = ?, update_time = NULL WHERE title = ?");
    ageFirstTask.addBindValue(QStringLiteral("2000-01-01 00:00:00"));
    ageFirstTask.addBindValue(QStringLiteral("S13 查询任务"));
    if (!ageFirstTask.exec()) return 12;
    const TaskFormOptions recentOptions = TaskQueryUseCase().loadFormOptions();
    const QStringList recentSources = recentOptions.sources;
    const int recentIndex = recentSources.indexOf(QStringLiteral("S13 最近来源"));
    const int firstIndex = recentSources.indexOf(QStringLiteral("S13 来源"));
    if (recentIndex < 0 || firstIndex < 0 || recentIndex >= firstIndex
        || recentOptions.tags.indexOf(QStringLiteral("S13 最近标签"))
            >= recentOptions.tags.indexOf(QStringLiteral("S13 标签"))
        || recentOptions.handlers.first() != QStringLiteral("本人")
        || recentOptions.handlers.indexOf(QStringLiteral("S13 最近办理人"))
            >= recentOptions.handlers.indexOf(QStringLiteral("S13 办理人"))) return 13;
    QSqlQuery removeRecencyFixture(DatabaseManager::instance().database());
    removeRecencyFixture.prepare("DELETE FROM task WHERE title = ?");
    removeRecencyFixture.addBindValue(QStringLiteral("S13 最近来源任务"));
    if (!removeRecencyFixture.exec()) return 14;

    TaskFilter filter;
    filter.keyword = QStringLiteral("S13 查询任务");
    const TaskListData page = TaskQueryUseCase().loadList(filter);
    if (page.total != 1 || page.tasks.size() != 1
        || page.tasks.first().title != QStringLiteral("S13 查询任务")
        || !page.sources.contains(QStringLiteral("S13 来源"))
        || !page.tags.contains(QStringLiteral("S13 标签"))) return 6;

    const int taskId = page.tasks.first().id;
    HomeDashboard dashboard = HomeDashboardUseCase().load(1);
    if (dashboard.inProgressTotal != 1 || dashboard.inProgressTasks.size() != 1
        || !HomeCommandUseCase().setFocus(taskId, true).ok
        || HomeDashboardUseCase().load(1).focusTasks.size() != 1
        || !HomeCommandUseCase().setFocus(taskId, false).ok) return 7;
    TaskDetailUseCase detail;
    if (!detail.findTask(taskId).has_value() || detail.findHistory(taskId).isEmpty()
        || !detail.addRemark(taskId, QStringLiteral("S13 备注")).ok
        || detail.findRemarks(taskId).size() != 1) return 8;
    const int remarkId = detail.findRemarks(taskId).first().id;
    if (!detail.deleteRemark(remarkId).ok || !detail.findRemarks(taskId).isEmpty()
        || !detail.completeTask(taskId).ok) return 9;
    const auto completedTask = detail.findTask(taskId);
    if (!completedTask.has_value() || !completedTask->isCompleted()
        || !detail.cancelComplete(taskId).ok || !detail.deleteTask(taskId).ok) return 9;
    const auto deletedTask = detail.findTask(taskId);
    if (!deletedTask.has_value() || deletedTask->status != QStringLiteral("已删除")) return 10;

    // A task's archive is moved back, never copied. A blocked destination must
    // leave the database status and the source folder unchanged.
    FileService files;
    if (!files.initializeWorkDir(workRoot).success) return 15;
    Task movable;
    movable.title = QStringLiteral("可恢复任务");
    movable.category = QStringLiteral("办");
    movable.source = QStringLiteral("恢复来源");
    movable.tag = QStringLiteral("恢复标签");
    movable.description = QStringLiteral("恢复说明");
    movable.deadline = QDate(2030, 3, 5);
    const TaskResult created = TaskService().createTask(movable);
    if (!created.ok) return 16;
    const auto active = detail.findTask(created.taskId);
    if (!active || !QDir(active->folderPath).exists()) return 17;
    const auto info = TaskInfoFile::read(active->folderPath);
    if (!info || info->deadline != movable.deadline || info->source != movable.source
        || info->tag != movable.tag || info->description != movable.description) return 18;
    if (!QFileInfo(TaskInfoFile::path(active->folderPath)).isHidden()) return 31;
    if (!TaskService().updateKeyTask(created.taskId, true).ok
        || !TaskInfoFile::read(active->folderPath)
        || !TaskInfoFile::read(active->folderPath)->isKeyTask) return 40;
    Task edited = *active;
    edited.title = QStringLiteral("修改后的任务名称");
    edited.description = QStringLiteral("修改后的恢复说明");
    if (!TaskService().updateTask(edited).ok) return 32;
    const auto renamedTask = detail.findTask(created.taskId);
    if (!renamedTask || renamedTask->folderPath == active->folderPath
        || QDir(active->folderPath).exists()
        || !TaskInfoFile::read(renamedTask->folderPath)
        || TaskInfoFile::read(renamedTask->folderPath)->description != edited.description) return 32;
    const QString editedPath = renamedTask->folderPath;
    QFile attachment(QDir(editedPath).filePath(QStringLiteral("attachment.txt")));
    if (!attachment.open(QIODevice::WriteOnly) || attachment.write("original") != 8) return 19;
    attachment.close();
#ifdef Q_OS_WIN
    HANDLE lockedFile = CreateFileW(reinterpret_cast<LPCWSTR>(attachment.fileName().utf16()),
                                    GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (lockedFile == INVALID_HANDLE_VALUE) return 35;
    const TaskResult blocked = detail.completeTask(created.taskId);
    const auto unchanged = detail.findTask(created.taskId);
    CloseHandle(lockedFile);
    if (blocked.ok || !unchanged || !unchanged->isActive() || !QDir(editedPath).exists()) return 36;
#endif
    if (!detail.completeTask(created.taskId).ok) return 20;
    const auto archived = detail.findTask(created.taskId);
    if (!archived || !QDir(archived->folderPath).exists() || QDir(editedPath).exists()) return 21;
    if (!QDir().mkpath(editedPath)) return 22;
    if (detail.cancelComplete(created.taskId).ok) return 23;
    const auto stillArchived = detail.findTask(created.taskId);
    if (!stillArchived || !stillArchived->isCompleted()
        || !QDir(archived->folderPath).exists()) return 24;
    if (!QDir(editedPath).removeRecursively()) return 25;
    if (!detail.cancelComplete(created.taskId).ok) return 26;
    const auto restored = detail.findTask(created.taskId);
    if (!restored || !restored->isActive() || !QDir(restored->folderPath).exists()
        || QDir(archived->folderPath).exists()
        || !QFile::exists(QDir(restored->folderPath).filePath(QStringLiteral("attachment.txt")))) return 27;

    // Rebuild the record from its surviving task directory and hidden info file.
    QSqlQuery remove(DatabaseManager::instance().database());
    remove.prepare("DELETE FROM task WHERE id = ?");
    remove.addBindValue(created.taskId);
    if (!remove.exec()) return 28;
    const QList<ScanImportItem> candidates = ScanMatchService().scanForImport(files.getWorkDir());
    auto candidate = std::find_if(candidates.begin(), candidates.end(), [&](const ScanImportItem &item) {
        return item.taskNo == active->taskNo;
    });
    if (candidate == candidates.end() || candidate->status != QStringLiteral("待导入")
        || !ScanMatchService().importScanItem(*candidate, files.getWorkDir()).ok) return 29;
    const auto imported = TaskService().getTaskByNo(active->taskNo);
    if (!imported || imported->title != edited.title || imported->source != movable.source || imported->tag != movable.tag
        || imported->description != edited.description || imported->deadline != movable.deadline) return 30;
    const int queryContract = verifyCompletedQueryContract();
    const int attachments = verifyAttachmentSearch();
    if (attachments != 0) return attachments;
    if (queryContract != 0) return queryContract;
    DatabaseManager::instance().close();
    return 0;
}
