#pragma once
#include "application/task/TaskFormUseCase.h"
#include "application/task/TaskQueryUseCase.h"
#include "application/reference/ReferenceCommandUseCase.h"
#include "application/reference/ReferenceQueryUseCase.h"
#include "application/periodic/PeriodicTaskUseCase.h"
#include "application/settings/SettingsUseCase.h"
#include "database/TagDictDao.h"
#include <QSqlError>

inline bool verifyTagMigration(const QString &path) {
    auto &manager=DatabaseManager::instance();
    QFile::remove(path);
    if(!manager.initialize(path)) return false;
    {
        QSqlQuery query(manager.database());
        const QStringList sql={"DELETE FROM tag_dict", "DROP TABLE periodic_tag_dict", "DROP TABLE reference_tag_dict",
            "INSERT INTO tag_dict VALUES (201,'原共享',1,7,'2020-01-01 00:00:00')",
            "INSERT INTO tag_dict VALUES (202,'停用共享',0,9,'2020-01-02 00:00:00')",
            "INSERT INTO task(task_no,title,category,tag,status,create_time) VALUES ('TAG-OLD','旧任务','查','原共享','进行中','2020-01-01 00:00:00')",
            "INSERT INTO periodic_task(name,category,period_type,period_value,tag,create_time) VALUES ('旧模板','报','daily',1,'仅旧周期','2020-01-01 00:00:00')",
            "INSERT INTO reference_material(ref_no,name,tag,create_time) VALUES ('TAG-REF','旧资料','仅旧资料','2020-01-01 00:00:00')",
            "CREATE VIEW reference_tag_dict AS SELECT 1 AS id"};
        for(const QString &statement:sql) if(!query.exec(statement)) {fprintf(stderr,"Migration fixture: %s\n",qPrintable(query.lastError().text()));return false;}
    }
    manager.close();
    // A conflicting legacy object forces the second CREATE to fail. The first
    // dictionary must roll back too, and initialization must not report ready.
    if(manager.initialize(path)) return false;
    bool rolledBack=false;
    {
        auto db=QSqlDatabase::addDatabase("QSQLITE","tagRollbackCheck");db.setDatabaseName(path);
        if(!db.open()) return false;
        rolledBack=!db.tables().contains("periodic_tag_dict");
        QSqlQuery query(db);
        rolledBack=rolledBack && query.exec("DROP VIEW reference_tag_dict");
        db.close();
    }
    QSqlDatabase::removeDatabase("tagRollbackCheck");
    if(!rolledBack || !manager.initialize(path)) return false;
    {
        TagDictDao tasks(manager.database()),periodic(manager.database(),TagScope::Periodic),references(manager.database(),TagScope::Reference);
        for(TagDictDao *dao : {&tasks,&periodic,&references}) {
            const auto shared=dao->findById(201),disabled=dao->findById(202);
            if(!shared || shared->name!=QStringLiteral("原共享") || shared->sortOrder!=7 || !shared->isEnabled
                || !disabled || disabled->isEnabled || disabled->sortOrder!=9) return false;
        }
        const auto oldPeriodic=periodic.findByName(QStringLiteral("仅旧周期"));
        if(!oldPeriodic || !references.findByName(QStringLiteral("仅旧资料")) || tasks.findByName(QStringLiteral("仅旧周期"))
            || references.findByName(QStringLiteral("仅旧周期")) || !tasks.isUsed(201) || periodic.isUsed(201) || references.isUsed(201)) return false;
        if(!periodic.setEnabled(201,false) || !tasks.findById(201)->isEnabled || !references.findById(201)->isEnabled) return false;
        TagDict changed=*references.findById(201);changed.name=QStringLiteral("仅改资料");
        if(!references.update(changed) || tasks.findById(201)->name!=QStringLiteral("原共享")
            || periodic.findById(201)->name!=QStringLiteral("原共享") || !periodic.deleteById(oldPeriodic->id)) return false;
        TagDict future;future.name=QStringLiteral("升级后仅任务");if(tasks.insert(future)<0) return false;
        QSqlQuery query(manager.database());
        if(!query.exec("SELECT tag FROM task WHERE task_no='TAG-OLD'") || !query.next() || query.value(0).toString()!=QStringLiteral("原共享")) return false;
    }
    manager.close();
    if(!manager.initialize(path)) return false;
    {
        TagDictDao periodic(manager.database(),TagScope::Periodic),references(manager.database(),TagScope::Reference);
        if(periodic.findByName(QStringLiteral("仅旧周期")) || periodic.findByName(QStringLiteral("升级后仅任务"))
            || references.findByName(QStringLiteral("升级后仅任务")) || periodic.findById(201)->isEnabled) return false;
        SettingsUseCase().resetAllData();
        if(!periodic.findAll().isEmpty() || !references.findAll().isEmpty()) return false;
    }
    manager.close();
    return true;
}

inline bool verifyTagDomainIsolation() {
    QSqlQuery query(DatabaseManager::instance().database());
    for (const QString &table : {QString("tag_dict"),QString("reference_tag_dict"),QString("periodic_tag_dict")})
        if (DatabaseManager::instance().database().tables().contains(table)
            && !query.exec("DELETE FROM "+table)) return false;
    TaskFormUseCase().ensureDictionaryEntries(QString(),QStringLiteral("仅任务标签"),QStringLiteral("本人"));
    ReferenceCommandUseCase().ensureDictionaryEntries(QString(),QStringLiteral("仅资料标签"));
    PeriodicTask task;
    task.name=QStringLiteral("隔离标签模板");task.category=QStringLiteral("报");
    task.periodType="daily";task.periodValue=1;task.tag=QStringLiteral("仅周期标签");task.handler=QStringLiteral("本人");
    const auto created=PeriodicTaskUseCase().create(task);
    if (!created.ok) return false;
    const QStringList tasks=TaskQueryUseCase().loadFormOptions().tags;
    const QStringList references=ReferenceQueryUseCase().formTags();
    const QStringList periodic=PeriodicTaskUseCase().loadFormOptions().tags;
    bool isolated=tasks==QStringList{QStringLiteral("仅任务标签")}
        && references==QStringList{QStringLiteral("仅资料标签")}
        && periodic==QStringList{QStringLiteral("仅周期标签")};
    for(TagScope scope : {TagScope::Task,TagScope::Periodic,TagScope::Reference})
        for(const QString &name : {QStringLiteral("共同甲"),QStringLiteral("共同乙")})
            if(SettingsUseCase().addTag(name,scope)<0) return false;
    if(!query.exec("INSERT INTO task(task_no,title,category,tag,status,create_time) VALUES ('TAG-RANK','排序','查','共同甲','进行中','2020-01-01 00:00:00')")
        || !query.exec("INSERT INTO reference_material(ref_no,name,tag,create_time) VALUES ('TAG-RANK','排序资料','共同乙','2099-01-01 00:00:00')")
        || !query.exec("INSERT INTO periodic_task(name,category,period_type,period_value,tag,create_time) VALUES ('TAG-RANK','报','daily',1,'共同乙','2099-01-01 00:00:00')")) return false;
    const auto taskRecent=TagDictDao(DatabaseManager::instance().database()).findEnabledByRecentUse();
    const auto refRecent=TagDictDao(DatabaseManager::instance().database(),TagScope::Reference).findEnabledByRecentUse();
    const auto perRecent=TagDictDao(DatabaseManager::instance().database(),TagScope::Periodic).findEnabledByRecentUse();
    isolated=isolated && !taskRecent.isEmpty() && taskRecent.first().name==QStringLiteral("共同甲")
        && !refRecent.isEmpty() && refRecent.first().name==QStringLiteral("共同乙")
        && !perRecent.isEmpty() && perRecent.first().name==QStringLiteral("共同乙");
    if(!query.exec("DELETE FROM task WHERE task_no='TAG-RANK'") || !query.exec("DELETE FROM reference_material WHERE ref_no='TAG-RANK'")
        || !query.exec("DELETE FROM periodic_task WHERE name='TAG-RANK'")) return false;
    PeriodicTaskUseCase().remove(created.taskId);
    if(!isolated) fprintf(stderr,"Tag dictionaries leak across task/reference/periodic domains\n");
    return isolated;
}
