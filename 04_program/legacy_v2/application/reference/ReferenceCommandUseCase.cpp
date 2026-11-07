#include "application/reference/ReferenceCommandUseCase.h"
#include "database/DatabaseManager.h"
#include "database/ReferenceMaterialDao.h"
#include "database/SourceDictDao.h"
#include "database/TagDictDao.h"
#include "service/ReferenceInfoFile.h"
#include "utils/Logger.h"
void ReferenceCommandUseCase::ensureDictionaryEntries(const QString &source, const QString &tag) const { const QSqlDatabase db=DatabaseManager::instance().database(); if (!source.trimmed().isEmpty()) { SourceDictDao dao(db); if (!dao.findByName(source.trimmed())) { SourceDict item; item.name=source.trimmed(); item.isEnabled=true; item.sortOrder=9999; dao.insert(item); } } if (!tag.trimmed().isEmpty()) { TagDictDao dao(db,TagScope::Reference); if (!dao.findByName(tag.trimmed())) { TagDict item; item.name=tag.trimmed(); item.isEnabled=true; item.sortOrder=9999; dao.insert(item); } } }
int ReferenceCommandUseCase::create(const ReferenceMaterial &reference) const { return ReferenceMaterialDao(DatabaseManager::instance().database()).insert(reference); }
bool ReferenceCommandUseCase::update(const ReferenceMaterial &reference) const {
    if (!ReferenceMaterialDao(DatabaseManager::instance().database()).update(reference)) return false;
    if (!reference.folderPath.isEmpty() && !ReferenceInfoFile::write(reference))
        Logger::instance().warn(QStringLiteral("资料信息文件写入失败: %1").arg(reference.folderPath));
    return true;
}
bool ReferenceCommandUseCase::remove(int id) const { return ReferenceMaterialDao(DatabaseManager::instance().database()).deleteById(id); }
