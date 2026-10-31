#pragma once
#include "database/TaskDao.h"
#include "database/ReferenceMaterialDao.h"

inline int verifyAttachmentSearch()
{
    const QString root = QDir(qEnvironmentVariable("NIUMA_TEST_ROOT")).filePath("attachment-search");
    const QString path = QDir(root).filePath("nested/附件检索-REPORT.pdf");
    if (!QDir().mkpath(QFileInfo(path).absolutePath())) return 90;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return 91;
    file.write("content-only-needle");
    file.close();
    const auto db = DatabaseManager::instance().database();
    QSqlQuery query(db);
    if (!query.exec("CREATE TEMP TABLE task AS SELECT * FROM main.task WHERE 0")
        || !query.exec("CREATE TEMP TABLE reference_material AS SELECT * FROM main.reference_material WHERE 0")) return 92;
    TaskDao tasks(db);
    ReferenceMaterialDao references(db);
    for (int index = 1; index <= 2; ++index) {
        Task task;
        task.taskNo = QStringLiteral("ATT-%1").arg(index);
        task.title = QStringLiteral("普通任务");
        task.category = QStringLiteral("办");
        task.handler = QStringLiteral("本人");
        task.status = QStringLiteral("进行中");
        task.source = index == 1 ? "A" : "B";
        task.folderPath = root;
        if (tasks.insert(task) <= 0) return 93;
        ReferenceMaterial reference;
        reference.refNo = task.taskNo;
        reference.name = QStringLiteral("普通资料");
        reference.source = task.source;
        reference.folderPath = root;
        if (references.insert(reference) <= 0) return 94;
    }
    // CREATE TABLE AS does not copy primary-key constraints into TEMP tables.
    if (!query.exec("UPDATE temp.task SET id=rowid")
        || !query.exec("UPDATE temp.reference_material SET id=rowid")) return 102;
    TaskFilter taskFilter;
    ReferenceFilter referenceFilter;
    taskFilter.keyword = referenceFilter.keyword = QStringLiteral("附件检索");
    taskFilter.pageSize = referenceFilter.pageSize = 1;
    if (tasks.countByFilter(taskFilter) != 2 || references.countByFilter(referenceFilter) != 2
        || tasks.findByFilter(taskFilter).size() != 1 || references.findByFilter(referenceFilter).size() != 1) return 95;
    taskFilter.source = referenceFilter.source = "A";
    if (tasks.countByFilter(taskFilter) != 1 || references.countByFilter(referenceFilter) != 1) return 96;
    taskFilter.keyword = referenceFilter.keyword = "report.PDF";
    if (tasks.countByFilter(taskFilter) != 1 || references.countByFilter(referenceFilter) != 1) return 97;
    taskFilter.keyword = referenceFilter.keyword = "content-only-needle";
    if (tasks.countByFilter(taskFilter) || references.countByFilter(referenceFilter)) return 98;
    if (!QFile::remove(path)) return 99;
    taskFilter.keyword = referenceFilter.keyword = QStringLiteral("附件检索");
    if (tasks.countByFilter(taskFilter) || references.countByFilter(referenceFilter)) return 100;
    if (!query.exec("DROP TABLE temp.task") || !query.exec("DROP TABLE temp.reference_material")) return 101;
    return 0;
}
