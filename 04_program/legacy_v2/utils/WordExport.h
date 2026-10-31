/**
 * @file WordExport.h
 * @brief 年度工作总结 Word 导出工具（静态方法集合）
 *
 * Qt 无原生 Word 支持，采用 HTML 格式写入 .doc 文件（Word 兼容）。
 * 文档结构：封面 + 重点工作 + 专项工作 + 主要工作成效。
 *
 * 依赖：Qt5 Core、QtSql、models/ExportConfig、models/Task、models/Achievement
 * 文档：task.md（年度导出）、PRD 13.2
 */
#pragma once
#include <QString>
#include <QList>
#include "../models/ExportConfig.h"

struct Achievement;
struct Task;

class WordExport {
public:
    struct ExportResult {
        bool success = false;
        QString errorMessage;
        QString outputPath;
        int itemCount = 0;
    };

    /**
     * @brief Export annual report to Word (.doc) file
     * @param config Export configuration
     * @param outputPath Target .doc file path
     * @param year Year to export
     * @return ExportResult
     *
     * Document structure:
     * 1. Title page: report title + year
     * 2. 重点工作开展情况: tasks with is_key_task=1, sorted by create_time
     * 3. 专项工作情况: tasks grouped by tag
     * 4. 主要工作成效: achievements with is_annual=1, including amount summary
     */
    static ExportResult exportAnnualReport(const ExportConfig &config, const QString &outputPath, int year);

private:
    static QString escapeHtml(const QString &text);
    static QString formatAmount(qint64 cents, const QString &unit);
    static QString buildTitlePage(const ExportConfig &config, int year);
    static QString buildKeyWorkSection(const QList<Task> &tasks, const ExportConfig &config);
    static QString buildSpecialWorkSection(const QList<Task> &tasks, const ExportConfig &config);
    static QString buildAchievementSection(const QList<Achievement> &achievements, const QList<Task> &tasks, const ExportConfig &config);
    static QString wrapHtmlBody(const QString &bodyContent);
};
