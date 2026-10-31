#pragma once
#include <QList>
#include <QStringList>
#include <optional>
#include "models/ReferenceFilter.h"
#include "models/ReferenceMaterial.h"
struct ReferenceListData { QList<ReferenceMaterial> items; int total = 0; QStringList sources; QStringList tags; QList<int> years; };
class ReferenceQueryUseCase { public: ReferenceListData loadList(const ReferenceFilter &filter) const; std::optional<ReferenceMaterial> find(int id) const; QStringList formSources() const; QStringList formTags() const; };
