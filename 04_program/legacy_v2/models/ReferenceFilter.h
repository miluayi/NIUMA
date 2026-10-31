#pragma once

#include <QDate>
#include <QString>

struct ReferenceFilter {
    QString keyword;
    QString source;
    QString tag;
    QDate startDate;
    QDate endDate;
    int year = 0; // 创建年度，0=全部
    int page = 1;
    int pageSize = 50;
};
