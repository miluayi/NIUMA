/**
 * @file ExportConfig.cpp
 * @brief ExportConfig 序列化/反序列化实现
 *
 * 文档：TD 2.7（settings 表）、PRD 13.2
 * 约束：配置损坏时静默回退默认值
 */
#include "ExportConfig.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

QString ExportConfig::toJson() const {
    QJsonObject obj;
    obj["reportTitle"] = reportTitle;
    obj["includeAchievements"] = includeAchievements;
    obj["includeCategoryStats"] = includeCategoryStats;
    obj["includeSourceStats"] = includeSourceStats;
    obj["includeAmountSummary"] = includeAmountSummary;
    obj["includeKeyAchievements"] = includeKeyAchievements;
    obj["sortBy"] = sortBy;
    obj["dateFormat"] = dateFormat;
    obj["amountUnit"] = amountUnit;
    return QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

ExportConfig ExportConfig::fromJson(const QString &json) {
    if (json.isEmpty()) {
        return defaultValue();
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        // 配置损坏，静默回退默认值
        return defaultValue();
    }

    QJsonObject obj = doc.object();
    ExportConfig config;

    if (obj.contains("reportTitle")) {
        config.reportTitle = obj.value("reportTitle").toString();
    }
    if (obj.contains("includeAchievements")) {
        config.includeAchievements = obj.value("includeAchievements").toBool(true);
    }
    if (obj.contains("includeCategoryStats")) {
        config.includeCategoryStats = obj.value("includeCategoryStats").toBool(true);
    }
    if (obj.contains("includeSourceStats")) {
        config.includeSourceStats = obj.value("includeSourceStats").toBool(true);
    }
    if (obj.contains("includeAmountSummary")) {
        config.includeAmountSummary = obj.value("includeAmountSummary").toBool(true);
    }
    if (obj.contains("includeKeyAchievements")) {
        config.includeKeyAchievements = obj.value("includeKeyAchievements").toBool(true);
    }
    if (obj.contains("sortBy")) {
        config.sortBy = obj.value("sortBy").toString("create_time");
    }
    if (obj.contains("dateFormat")) {
        config.dateFormat = obj.value("dateFormat").toString("yyyy-MM-dd");
    }
    if (obj.contains("amountUnit")) {
        config.amountUnit = obj.value("amountUnit").toString("yuan");
    }

    return config;
}
