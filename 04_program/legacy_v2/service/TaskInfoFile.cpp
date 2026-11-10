#include "TaskInfoFile.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

#include "database/AchievementDao.h"
#include "database/DatabaseManager.h"
#include "utils/Logger.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace {

QJsonValue encodeAchievement(const std::optional<Achievement> &achievement)
{
    if (!achievement) return QJsonValue::Null;
    const auto &record = *achievement;
    return QJsonObject{
        {QStringLiteral("work_content"), record.workContent},
        {QStringLiteral("work_object"), record.workObject},
        {QStringLiteral("result_count"), record.resultCount},
        {QStringLiteral("result_amount_cents"), QString::number(record.resultAmount)},
        {QStringLiteral("description"), record.description},
        {QStringLiteral("is_annual"), record.isAnnual},
        {QStringLiteral("is_key_achievement"), record.isKeyAchievement},
        {QStringLiteral("create_time"), record.createTime.toString(Qt::ISODate)}
    };
}

bool decodeAchievement(const QJsonValue &value, std::optional<Achievement> &achievement)
{
    if (value.isUndefined() || value.isNull()) return true;
    if (!value.isObject()) return false;
    const auto data = value.toObject();
    const auto content = data.value(QStringLiteral("work_content"));
    const auto count = data.value(QStringLiteral("result_count"));
    const int countValue = count.toInt();
    bool amountOk = false;
    const qint64 amount = data.value(QStringLiteral("result_amount_cents")).toString().toLongLong(&amountOk);
    const auto time = QDateTime::fromString(data.value(QStringLiteral("create_time")).toString(), Qt::ISODate);
    if (!content.isString() || !count.isDouble() || count.toDouble() != countValue
        || !amountOk || !time.isValid()) return false;

    Achievement record;
    record.workContent = content.toString();
    record.workObject = data.value(QStringLiteral("work_object")).toString();
    record.resultCount = countValue;
    record.resultAmount = amount;
    record.description = data.value(QStringLiteral("description")).toString();
    record.isAnnual = data.value(QStringLiteral("is_annual")).toBool();
    record.isKeyAchievement = data.value(QStringLiteral("is_key_achievement")).toBool();
    record.createTime = time;
    achievement = record;
    return true;
}

std::optional<QJsonObject> readObject(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return std::nullopt;
    QJsonParseError error;
    const auto doc = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) return std::nullopt;
    const auto data = doc.object();
    if (data.value(QStringLiteral("format_version")).toInt() != 1
        || !data.value(QStringLiteral("task_no")).isString()
        || !data.value(QStringLiteral("title")).isString()) return std::nullopt;
    return data;
}

bool writeObject(const QString &filePath, const QJsonObject &data)
{
    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) return false;
    const QByteArray bytes = QJsonDocument(data).toJson(QJsonDocument::Indented);
    if (file.write(bytes) != bytes.size() || !file.commit()) return false;
#ifdef Q_OS_WIN
    const DWORD attributes = GetFileAttributesW(reinterpret_cast<LPCWSTR>(filePath.utf16()));
    if (attributes == INVALID_FILE_ATTRIBUTES
        || !SetFileAttributesW(reinterpret_cast<LPCWSTR>(filePath.utf16()), attributes | FILE_ATTRIBUTE_HIDDEN)) {
        Logger::instance().warn(QStringLiteral("任务信息文件隐藏属性设置失败: %1").arg(filePath));
    }
#endif
    return true;
}

bool syncAchievementFile(const Task &task, bool recoverMissing)
{
    const QString filePath = TaskInfoFile::path(task.folderPath);
    if (!QFileInfo::exists(filePath)) return TaskInfoFile::write(task);
    auto data = readObject(filePath);
    if (!data) return false;
    if (data->value(QStringLiteral("task_no")).toString() != task.taskNo) return recoverMissing;
    if (recoverMissing && (data->value(QStringLiteral("title")).toString() != task.title
        || data->value(QStringLiteral("category")).toString() != task.category)) return true;

    const QString key = QStringLiteral("achievement");
    AchievementDao dao(DatabaseManager::instance().database());
    bool queried = false;
    const auto current = dao.findByTaskId(task.id, &queried);
    if (!queried) return false;
    if (recoverMissing && !current) {
        std::optional<Achievement> saved;
        if (!decodeAchievement(data->value(key), saved)) return false;
        return !saved || dao.restoreForTask(task.id, *saved);
    }

    const auto value = encodeAchievement(current);
    if (data->value(key) == value) return true;
    data->insert(key, value);
    return writeObject(filePath, *data);
}

} // namespace

QString TaskInfoFile::path(const QString &folderPath)
{
    return QDir(folderPath).filePath(QStringLiteral(".niuma-task.json"));
}

bool TaskInfoFile::write(const Task &task)
{
    if (task.folderPath.isEmpty() || task.taskNo.isEmpty() || !QDir(task.folderPath).exists()) {
        return false;
    }
    bool queried = true;
    const auto achievement = task.id > 0
        ? AchievementDao(DatabaseManager::instance().database()).findByTaskId(task.id, &queried)
        : std::optional<Achievement>();
    if (!queried) return false;
    const QJsonObject data{
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
        {QStringLiteral("delete_time"), task.deleteTime.toString(Qt::ISODate)},
        {QStringLiteral("achievement"), encodeAchievement(achievement)}
    };
    return writeObject(path(task.folderPath), data);
}

std::optional<Task> TaskInfoFile::read(const QString &folderPath)
{
    const auto snapshot = readSnapshot(folderPath);
    return snapshot ? std::optional<Task>(snapshot->task) : std::nullopt;
}

std::optional<TaskInfoSnapshot> TaskInfoFile::readSnapshot(const QString &folderPath)
{
    const auto object = readObject(path(folderPath));
    if (!object) return std::nullopt;
    const auto &data = *object;
    TaskInfoSnapshot snapshot;
    if (!decodeAchievement(data.value(QStringLiteral("achievement")), snapshot.achievement)) return std::nullopt;
    Task &task = snapshot.task;
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
    return snapshot;
}

bool TaskInfoFile::syncAchievement(const Task &task)
{
    return syncAchievementFile(task, false);
}

bool TaskInfoFile::reconcileAchievement(const Task &task)
{
    return syncAchievementFile(task, true);
}
