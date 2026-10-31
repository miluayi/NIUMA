#include "application/task/TaskFormUseCase.h"

#include "database/DatabaseManager.h"
#include "database/HandlerDao.h"
#include "database/SourceDictDao.h"
#include "database/TagDictDao.h"
#include "utils/Logger.h"

void TaskFormUseCase::ensureDictionaryEntries(const QString &source, const QString &tag,
                                              const QString &handler) const
{
    const QSqlDatabase database = DatabaseManager::instance().database();
    const QString normalizedSource = source.trimmed();
    const QString normalizedTag = tag.trimmed();
    const QString normalizedHandler = handler.trimmed().isEmpty()
        ? QStringLiteral("本人") : handler.trimmed();

    if (!normalizedSource.isEmpty()) {
        SourceDictDao dao(database);
        if (!dao.findByName(normalizedSource).has_value()) {
            SourceDict entry;
            entry.name = normalizedSource;
            entry.isEnabled = true;
            entry.sortOrder = 9999;
            dao.insert(entry);
            Logger::instance().info(QStringLiteral("自动添加来源到字典: %1").arg(normalizedSource));
        }
    }

    if (!normalizedTag.isEmpty()) {
        TagDictDao dao(database);
        if (!dao.findByName(normalizedTag).has_value()) {
            TagDict entry;
            entry.name = normalizedTag;
            entry.isEnabled = true;
            entry.sortOrder = 9999;
            dao.insert(entry);
            Logger::instance().info(QStringLiteral("自动添加标签到字典: %1").arg(normalizedTag));
        }
    }

    HandlerDao dao(database);
    if (!dao.findByName(normalizedHandler).has_value()) {
        Handler entry;
        entry.name = normalizedHandler;
        entry.isEnabled = true;
        entry.sortOrder = 9999;
        dao.insert(entry);
        Logger::instance().info(QStringLiteral("自动添加办理人到字典: %1").arg(normalizedHandler));
    }
}
