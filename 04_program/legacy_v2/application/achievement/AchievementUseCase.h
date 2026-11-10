#pragma once
#include <QList>
#include <QString>
#include "models/Achievement.h"
#include "utils/WordExport.h"
struct AchievementYearStats { int total = 0; qint64 amount = 0; int keyCount = 0; int completedTasks = 0; };
struct AchievementListItem {
    Achievement achievement;
    QString taskNo;
    QString taskCategory;
};

class AchievementUseCase {
public:
    QList<Achievement> findByYear(int year) const;
    QList<AchievementListItem> listByYear(int year) const;
    AchievementYearStats stats(int year) const;
    int create(const Achievement &achievement) const;
    bool update(const Achievement &achievement) const;
    bool remove(int achievementId) const;
    WordExport::ExportResult exportAnnualReport(int year, const QString &outputPath) const;
};
