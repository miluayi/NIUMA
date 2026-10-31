#pragma once

#include <QDate>
#include <QString>

struct TaskStats {
    int totalCount = 0;
    int completedCount = 0;
    int inProgressCount = 0;
    int otherHandlerCount = 0;
    double completionRate = 0.0;
};

struct CategoryStat {
    QString category;
    int count = 0;
};

struct SourceStat {
    QString source;
    int count = 0;
};

struct MonthlyTrend {
    int month = 0;
    int count = 0;
};

struct HeatmapData {
    QDate date;
    int count = 0;
};
