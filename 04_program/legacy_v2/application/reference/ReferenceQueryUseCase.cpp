#include "application/reference/ReferenceQueryUseCase.h"
#include "database/DatabaseManager.h"
#include "database/ReferenceMaterialDao.h"
#include "database/SourceDictDao.h"
#include "database/TagDictDao.h"
namespace { template <typename T> QStringList names(const QList<T> &items) { QStringList result; for (const T &item : items) result.append(item.name); return result; } }
ReferenceListData ReferenceQueryUseCase::loadList(const ReferenceFilter &filter) const
{
    ReferenceMaterialDao dao(DatabaseManager::instance().database());
    ReferenceListData result;
    result.items = dao.findByFilter(filter);
    result.total = dao.countByFilter(filter);
    const QList<ReferenceMaterial> all = dao.findAll();
    for (const ReferenceMaterial &reference : all) {
        const int year = reference.createTime.date().year();
        if (reference.createTime.isValid() && !result.years.contains(year)) result.years.append(year);
        if (!reference.source.isEmpty() && !result.sources.contains(reference.source)) result.sources.append(reference.source);
        if (!reference.tag.isEmpty() && !result.tags.contains(reference.tag)) result.tags.append(reference.tag);
    }
    result.sources.sort();
    result.tags.sort();
    return result;
}
std::optional<ReferenceMaterial> ReferenceQueryUseCase::find(int id) const { return ReferenceMaterialDao(DatabaseManager::instance().database()).findById(id); }
QStringList ReferenceQueryUseCase::formSources() const { return names(SourceDictDao(DatabaseManager::instance().database()).findEnabledByRecentUse()); }
QStringList ReferenceQueryUseCase::formTags() const { return names(TagDictDao(DatabaseManager::instance().database(),TagScope::Reference).findEnabledByRecentUse()); }
