#pragma once

#include <QDirIterator>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStringList>
#include <QVariant>

namespace AttachmentSearch {
// Search current filenames, including externally added/renamed nested files.
// Candidate rows already obey the other filters; never scan an empty path.
inline QStringList matchingIds(const QSqlDatabase &database, const QString &table,
                           const QStringList &conditions, const QStringList &values,
                           const QString &keyword)
{
    QSqlQuery query(database);
    QString sql = "SELECT id, folder_path FROM " + table;
    if (!conditions.isEmpty()) sql += " WHERE " + conditions.join(" AND ");
    query.prepare(sql);
    for (const QString &value : values) query.addBindValue(value);
    QStringList ids;
    if (!query.exec()) return {};
    while (query.next()) {
        const QString path = query.value(1).toString();
        if (path.trimmed().isEmpty() || !QDir(path).exists()) continue;
        QDirIterator files(path, QDir::Files | QDir::Hidden | QDir::NoSymLinks,
                           QDirIterator::Subdirectories);
        while (files.hasNext()) {
            files.next();
            const QString name = files.fileName();
            if (name.startsWith(".niuma-", Qt::CaseInsensitive)) continue;
            if (name.contains(keyword, Qt::CaseInsensitive)) {
                ids.append(QString::number(query.value(0).toInt()));
                break;
            }
        }
    }
    return ids;
}
}
