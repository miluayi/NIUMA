#pragma once
#include <QList>
#include <QString>
#include "models/Achievement.h"
#include "utils/WordExport.h"
struct AchievementYearStats { int total = 0; qint64 amount = 0; int keyCount = 0; int completedTasks = 0; };
struct AchievementListItem { Achievement achievement; QString taskNo; };
class AchievementUseCase { public: QList<Achievement> findByYear(int year) const; QList<AchievementListItem> listByYear(int year) const; AchievementYearStats stats(int year) const; int create(const Achievement &achievement) const; WordExport::ExportResult exportAnnualReport(int year, const QString &outputPath) const; };
