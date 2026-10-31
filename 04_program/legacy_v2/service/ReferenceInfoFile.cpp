#include "ReferenceInfoFile.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

QString ReferenceInfoFile::path(const QString &folderPath)
{
    return QDir(folderPath).filePath(QStringLiteral(".niuma-reference.json"));
}

bool ReferenceInfoFile::write(const ReferenceMaterial &reference)
{
    if (reference.folderPath.isEmpty() || reference.refNo.isEmpty()
        || !QDir(reference.folderPath).exists()) return false;
    const QJsonObject data{
        {QStringLiteral("format_version"), 1},
        {QStringLiteral("ref_no"), reference.refNo},
        {QStringLiteral("name"), reference.name},
        {QStringLiteral("source"), reference.source},
        {QStringLiteral("tag"), reference.tag},
        {QStringLiteral("description"), reference.description},
        {QStringLiteral("create_time"), reference.createTime.toString(Qt::ISODate)}
    };
    const QString filePath = path(reference.folderPath);
    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) return false;
    const QByteArray bytes = QJsonDocument(data).toJson(QJsonDocument::Indented);
    if (file.write(bytes) != bytes.size() || !file.commit()) return false;
#ifdef Q_OS_WIN
    const DWORD attributes = GetFileAttributesW(reinterpret_cast<LPCWSTR>(filePath.utf16()));
    if (attributes == INVALID_FILE_ATTRIBUTES
        || !SetFileAttributesW(reinterpret_cast<LPCWSTR>(filePath.utf16()),
                               attributes | FILE_ATTRIBUTE_HIDDEN)) return false;
#endif
    return true;
}

std::optional<ReferenceMaterial> ReferenceInfoFile::read(const QString &folderPath)
{
    QFile file(path(folderPath));
    if (!file.open(QIODevice::ReadOnly)) return std::nullopt;
    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) return std::nullopt;
    const QJsonObject data = doc.object();
    if (data.value(QStringLiteral("format_version")).toInt() != 1
        || !data.value(QStringLiteral("ref_no")).isString()
        || !data.value(QStringLiteral("name")).isString()) return std::nullopt;
    ReferenceMaterial reference;
    reference.refNo = data.value(QStringLiteral("ref_no")).toString();
    reference.name = data.value(QStringLiteral("name")).toString();
    reference.source = data.value(QStringLiteral("source")).toString();
    reference.tag = data.value(QStringLiteral("tag")).toString();
    reference.description = data.value(QStringLiteral("description")).toString();
    reference.createTime = QDateTime::fromString(
        data.value(QStringLiteral("create_time")).toString(), Qt::ISODate);
    return reference;
}
