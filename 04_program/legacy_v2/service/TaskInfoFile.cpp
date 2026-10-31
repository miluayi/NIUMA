#include "TaskInfoFile.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

QString TaskInfoFile::path(const QString &folderPath)
{
    return QDir(folderPath).filePath(QStringLiteral(".niuma-task.json"));
}

bool TaskInfoFile::write(const Task &task)
{
    if (task.folderPath.isEmpty() || task.taskNo.isEmpty() || !QDir(task.folderPath).exists()) {
        return false;
    }
    QJsonObject data{
        {QStringLiteral("format_version"), 1},
        {QStringLiteral("task_no"), task.taskNo},
        {QStringLiteral("title"), task.title},
        {QStringLiteral("category"), task.category},
        {QStringLiteral("source"), task.source},
        {QStringLiteral("tag"), task.tag},
        {QStringLiteral("handler"), task.handler},
        {QStringLiteral("deadline"), task.deadline.isValid() ? task.deadline.toString(Qt::ISODate) : QString()},
        {QStringLiteral("description"), task.description},
        {QStringLiteral("is_key_task"), task.isKeyTask},
        {QStringLiteral("has_subdirs"), task.hasSubdirs},
        {QStringLiteral("original_status"), task.originalStatus},
        {QStringLiteral("create_time"), task.createTime.toString(Qt::ISODate)},
        {QStringLiteral("finish_time"), task.finishTime.toString(Qt::ISODate)},
        {QStringLiteral("delete_time"), task.deleteTime.toString(Qt::ISODate)}
    };
    const QString filePath = path(task.folderPath);
    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) return false;
    const QByteArray bytes = QJsonDocument(data).toJson(QJsonDocument::Indented);
    if (file.write(bytes) != bytes.size() || !file.commit()) return false;
#ifdef Q_OS_WIN
    const DWORD attributes = GetFileAttributesW(reinterpret_cast<LPCWSTR>(filePath.utf16()));
    if (attributes == INVALID_FILE_ATTRIBUTES
        || !SetFileAttributesW(reinterpret_cast<LPCWSTR>(filePath.utf16()), attributes | FILE_ATTRIBUTE_HIDDEN)) {
        return false;
    }
#endif
    return true;
}

std::optional<Task> TaskInfoFile::read(const QString &folderPath)
{
    QFile file(path(folderPath));
    if (!file.open(QIODevice::ReadOnly)) return std::nullopt;
    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) return std::nullopt;
    const QJsonObject data = doc.object();
    if (data.value(QStringLiteral("format_version")).toInt() != 1
        || !data.value(QStringLiteral("task_no")).isString()
        || !data.value(QStringLiteral("title")).isString()) return std::nullopt;
    Task task;
    task.taskNo = data.value(QStringLiteral("task_no")).toString();
    task.title = data.value(QStringLiteral("title")).toString();
    task.category = data.value(QStringLiteral("category")).toString();
    task.source = data.value(QStringLiteral("source")).toString();
    task.tag = data.value(QStringLiteral("tag")).toString();
    task.handler = data.value(QStringLiteral("handler")).toString(QStringLiteral("本人"));
    task.deadline = QDate::fromString(data.value(QStringLiteral("deadline")).toString(), Qt::ISODate);
    task.description = data.value(QStringLiteral("description")).toString();
    task.isKeyTask = data.value(QStringLiteral("is_key_task")).toBool();
    task.hasSubdirs = data.value(QStringLiteral("has_subdirs")).toBool();
    task.originalStatus = data.value(QStringLiteral("original_status")).toString();
    task.createTime = QDateTime::fromString(data.value(QStringLiteral("create_time")).toString(), Qt::ISODate);
    task.finishTime = QDateTime::fromString(data.value(QStringLiteral("finish_time")).toString(), Qt::ISODate);
    task.deleteTime = QDateTime::fromString(data.value(QStringLiteral("delete_time")).toString(), Qt::ISODate);
    return task;
}
