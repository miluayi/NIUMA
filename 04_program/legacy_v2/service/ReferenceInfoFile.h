#pragma once

#include <QString>
#include <optional>

#include "models/ReferenceMaterial.h"

class ReferenceInfoFile {
public:
    static QString path(const QString &folderPath);
    static bool write(const ReferenceMaterial &reference);
    static std::optional<ReferenceMaterial> read(const QString &folderPath);
};
