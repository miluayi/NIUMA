#include "application/stats/StatsUseCase.h"

#include "database/DatabaseManager.h"
#include "database/TaskDao.h"

TaskStats StatsUseCase::summary(const QDate &start, const QDate &end) const
{
    return TaskDao(DatabaseManager::instance().database()).getStatsByDateRange(start, end);
}

QList<CategoryStat> StatsUseCase::categories(const QDate &start, const QDate &end) const
{
    return TaskDao(DatabaseManager::instance().database()).getCategoryDistribution(start, end);
}

QList<SourceStat> StatsUseCase::sources(const QDate &start, const QDate &end) const
{
    return TaskDao(DatabaseManager::instance().database()).getSourceDistribution(start, end);
}

QList<MonthlyTrend> StatsUseCase::monthlyTrend(int year) const
{
    return TaskDao(DatabaseManager::instance().database()).getMonthlyTrend(year);
}

QList<HeatmapData> StatsUseCase::heatmap(int year, const QString &dimension) const
{
    return TaskDao(DatabaseManager::instance().database()).getHeatmapData(year, dimension);
}
