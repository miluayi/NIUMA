/**
 * @file ExportConfig.h
 * @brief Word 导出配置数据模型，存储于 settings 表（key=export_config）
 *
 * 文档：TD 2.7（settings 表）、3.8、PRD 13.2
 * 约束：配置损坏时静默回退默认值并记录日志
 */
#pragma once
#include <QString>

struct ExportConfig {
    QString reportTitle = "年度工作总结";
    bool includeAchievements = true;
    bool includeCategoryStats = true;
    bool includeSourceStats = true;
    bool includeAmountSummary = true;
    bool includeKeyAchievements = true;
    QString sortBy = "create_time";  // create_time / category / source
    QString dateFormat = "yyyy-MM-dd";
    QString amountUnit = "yuan";     // yuan / wanyuan

    // 序列化为 JSON 字符串（存 settings 表）
    QString toJson() const;
    static ExportConfig fromJson(const QString &json);
    static ExportConfig defaultValue() { return ExportConfig{}; }
};
