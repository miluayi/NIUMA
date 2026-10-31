#pragma once

#include <QDate>
#include <QList>

#include "models/Statistics.h"

class StatsUseCase {
public:
    TaskStats summary(const QDate &start, const QDate &end) const;
    QList<CategoryStat> categories(const QDate &start, const QDate &end) const;
    QList<SourceStat> sources(const QDate &start, const QDate &end) const;
    QList<MonthlyTrend> monthlyTrend(int year) const;
    QList<HeatmapData> heatmap(int year, const QString &dimension) const;
};
