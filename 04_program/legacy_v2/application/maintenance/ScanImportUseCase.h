#pragma once

#include <QList>

#include "models/ScanImportItem.h"

struct ScanImportData {
    bool workDirConfigured = false;
    QList<ScanImportItem> items;
};

struct ScanImportSummary {
    int successCount = 0;
    int skipCount = 0;
    int failCount = 0;
};

class ScanImportUseCase {
public:
    ScanImportData scan() const;
    ScanImportSummary importSelected(const QList<ScanImportItem> &selectedItems,
                                     int totalItemCount) const;
};
