#pragma once

#include <QComboBox>
#include <QDateEdit>
#include <QList>
#include <QSignalBlocker>
#include <QStringList>
#include <algorithm>
#include <functional>

namespace ListFilterControls {
inline void reloadOptions(QComboBox *combo, const QString &allText, const QStringList &options)
{
    const QString selected = combo->currentIndex() > 0 ? combo->currentText() : QString();
    const QSignalBlocker blocker(combo);
    combo->clear();
    combo->addItem(allText);
    combo->addItems(options);
    // 保留当前条件，即使最后一条符合条件的记录刚刚被删除。
    if (!selected.isEmpty()) {
        if (combo->findText(selected) < 0) combo->addItem(selected);
        combo->setCurrentText(selected);
    }
}

inline void reloadYears(QComboBox *combo, QList<int> years)
{
    const int currentYear = QDate::currentDate().year();
    const int selected = combo->count() ? combo->currentData().toInt() : currentYear;
    for (int year = 2020; year <= currentYear + 1; ++year) years.append(year);
    if (selected > 0) years.append(selected);
    std::sort(years.begin(), years.end(), std::greater<int>());
    years.erase(std::unique(years.begin(), years.end()), years.end());
    const QSignalBlocker blocker(combo);
    combo->clear();
    combo->addItem(QStringLiteral("全部年度"), 0);
    for (int year : years) {
        if (year >= 1900 && year <= 9999) combo->addItem(QString::number(year), year);
    }
    combo->setCurrentIndex(combo->findData(selected));
}

inline void syncYearRange(QComboBox *combo, QDateEdit *start, QDateEdit *end)
{
    const int selected = combo->currentData().toInt();
    const int year = selected > 0 ? selected : QDate::currentDate().year();
    const QSignalBlocker startBlocker(start), endBlocker(end);
    start->setDateRange(QDate(1900, 1, 1), QDate(9999, 12, 31));
    end->setDateRange(QDate(1900, 1, 1), QDate(9999, 12, 31));
    start->setDate(QDate(year, 1, 1));
    end->setDate(QDate(year, 12, 31));
}
}
