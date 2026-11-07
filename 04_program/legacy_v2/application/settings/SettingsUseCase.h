#pragma once

#include <QList>
#include <QString>

#include "models/ExportConfig.h"
#include "models/Handler.h"
#include "models/SourceDict.h"
#include "models/TagDict.h"

enum class UiFontWeight { Regular, Medium, Semibold };

struct UiFontPreferences {
    int pixelSize = 14;
    UiFontWeight weight = UiFontWeight::Regular;
};

class SettingsUseCase {
public:
    QList<SourceDict> sources() const;
    QList<TagDict> tags(TagScope scope=TagScope::Task) const;
    QList<Handler> handlers() const;
    int addSource(const QString &name) const;
    int addTag(const QString &name,TagScope scope=TagScope::Task) const;
    int addHandler(const QString &name) const;
    bool deleteSource(int id) const;
    bool deleteTag(int id,TagScope scope=TagScope::Task) const;
    bool deleteHandler(int id) const;

    ExportConfig exportConfig() const;
    bool saveExportConfig(const ExportConfig &config) const;
    UiFontPreferences uiFontPreferences() const;
    bool saveUiFontPreferences(const UiFontPreferences &preferences) const;
    QString workDir() const;
    QString lastBackupTime() const;
    void resetAllData() const;
};
