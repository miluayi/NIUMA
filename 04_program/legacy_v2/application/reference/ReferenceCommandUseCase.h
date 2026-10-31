#pragma once
#include "models/ReferenceMaterial.h"
class ReferenceCommandUseCase { public: void ensureDictionaryEntries(const QString &source, const QString &tag) const; int create(const ReferenceMaterial &reference) const; bool update(const ReferenceMaterial &reference) const; bool remove(int id) const; };
