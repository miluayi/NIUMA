#pragma once

#include <QString>

struct ScanImportItem {
    QString folderName;
    QString taskNo;
    QString name;
    QString category;
    QString importType;
    QString sourceDir;
    QString folderPath;
    QString status;
    bool isDuplicate = false;
    int existingId = 0;
    QString conflictReason;
    bool isConflict = false;
};
