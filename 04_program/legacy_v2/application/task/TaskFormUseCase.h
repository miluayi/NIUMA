#pragma once

#include <QString>

class TaskFormUseCase {
public:
    void ensureDictionaryEntries(const QString &source, const QString &tag,
                                 const QString &handler) const;
};
