/**
 * @file WordExport.cpp
 * @brief 年度工作总结 Word 导出实现（HTML 格式 .doc）
 *
 * 依赖：utils/WordExport.h、database/TaskDao、database/DatabaseManager、
 *       models/Achievement、models/Task、models/ExportConfig、utils/Logger
 * 文档：task.md（年度导出）、PRD 13.2
 *
 * 关键约束：
 *  - AchievementDao 尚未实现，直接通过 QSqlQuery 查询 achievement 表
 *  - AmountUtil 尚未实现，金额转换在 formatAmount 内联实现
 *  - 数据仅查询一次，各章节复用同一份数据，避免不一致
 *  - 全部文本经 escapeHtml 转义，防止 HTML 注入与渲染错误
 */

#include "WordExport.h"

#include <QDate>
#include <QFile>
#include <QMap>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <algorithm>

#include "database/DatabaseManager.h"
#include "database/TaskDao.h"
#include "models/Achievement.h"
#include "utils/Logger.h"

// ============================================================================
// 文件局部辅助函数（匿名命名空间，仅本文件可见）
// ============================================================================
namespace {

// HTML 转义实际实现（WordExport::escapeHtml 委托至此）
QString escapeHtmlStr(const QString &text)
{
    QString s = text;
    s.replace('&', QStringLiteral("&amp;"));
    s.replace('<', QStringLiteral("&lt;"));
    s.replace('>', QStringLiteral("&gt;"));
    s.replace('"', QStringLiteral("&quot;"));
    s.replace('\'', QStringLiteral("&#39;"));
    return s;
}

// 按 config.sortBy 排序任务（create_time / category / source，默认 create_time）
void sortTasksBy(QList<Task> &tasks, const QString &sortBy)
{
    if (sortBy == QStringLiteral("category")) {
        std::sort(tasks.begin(), tasks.end(), [](const Task &a, const Task &b) {
            if (a.category != b.category) {
                return a.category < b.category;
            }
            return a.createTime < b.createTime;
        });
    } else if (sortBy == QStringLiteral("source")) {
        std::sort(tasks.begin(), tasks.end(), [](const Task &a, const Task &b) {
            if (a.source != b.source) {
                return a.source < b.source;
            }
            return a.createTime < b.createTime;
        });
    } else {
        // create_time（默认）
        std::sort(tasks.begin(), tasks.end(), [](const Task &a, const Task &b) {
            return a.createTime < b.createTime;
        });
    }
}

// 构造任务表格（编号/任务名称/分类/来源/创建时间），重点工作与专项工作复用
QString buildTaskTableHtml(const QList<Task> &tasks, const ExportConfig &config)
{
    QString html;
    html += QStringLiteral("<table>");
    html += QStringLiteral("<thead><tr>");
    html += QStringLiteral("<th>编号</th>");
    html += QStringLiteral("<th>任务名称</th>");
    html += QStringLiteral("<th>分类</th>");
    html += QStringLiteral("<th>来源</th>");
    html += QStringLiteral("<th>创建时间</th>");
    html += QStringLiteral("</tr></thead><tbody>");
    for (const Task &t : tasks) {
        QString createTime = t.createTime.toString(config.dateFormat);
        html += QStringLiteral("<tr>");
        html += QStringLiteral("<td>") + escapeHtmlStr(t.taskNo) + QStringLiteral("</td>");
        html += QStringLiteral("<td>") + escapeHtmlStr(t.title) + QStringLiteral("</td>");
        html += QStringLiteral("<td>") + escapeHtmlStr(t.category) + QStringLiteral("</td>");
        html += QStringLiteral("<td>") + escapeHtmlStr(t.source) + QStringLiteral("</td>");
        html += QStringLiteral("<td>") + escapeHtmlStr(createTime) + QStringLiteral("</td>");
        html += QStringLiteral("</tr>");
    }
    html += QStringLiteral("</tbody></table>");
    return html;
}

// 直接查询指定年度的成果（AchievementDao 尚未实现）
QList<Achievement> loadAchievementsForYear(const QSqlDatabase &db, int year)
{
    QList<Achievement> result;
    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "SELECT id, task_id, work_content, work_object, result_count, "
        "result_amount, description, is_annual, is_key_achievement, create_time "
        "FROM achievement WHERE create_time >= ? AND create_time <= ? "
        "ORDER BY create_time ASC"));
    query.addBindValue(QStringLiteral("%1-01-01 00:00:00").arg(year));
    query.addBindValue(QStringLiteral("%1-12-31 23:59:59").arg(year));
    if (!query.exec()) {
        Logger::instance().error(QStringLiteral(
            "WordExport: 查询 achievement 失败: %1").arg(query.lastError().text()));
        return result;
    }
    while (query.next()) {
        Achievement a;
        a.id = query.value(0).toInt();
        a.taskId = query.value(1).toInt();
        a.workContent = query.value(2).toString();
        a.workObject = query.value(3).toString();
        a.resultCount = query.value(4).toInt();
        a.resultAmount = query.value(5).toLongLong();
        a.description = query.value(6).toString();
        a.isAnnual = query.value(7).toInt() != 0;
        a.isKeyAchievement = query.value(8).toInt() != 0;
        QString createTimeStr = query.value(9).toString();
        if (!createTimeStr.isEmpty()) {
            a.createTime = QDateTime::fromString(createTimeStr, QStringLiteral("yyyy-MM-dd HH:mm:ss"));
        }
        result.append(a);
    }
    return result;
}

} // namespace

// ============================================================================
// 公开接口
// ============================================================================

WordExport::ExportResult WordExport::exportAnnualReport(const ExportConfig &config,
                                                        const QString &outputPath,
                                                        int year)
{
    ExportResult result;

    // 数据仅查询一次，各章节复用
    QSqlDatabase db = DatabaseManager::instance().database();
    TaskDao taskDao(db);
    QList<Task> tasks = taskDao.findByDateRange(QDate(year, 1, 1), QDate(year, 12, 31));
    QList<Achievement> achievements = loadAchievementsForYear(db, year);

    // 构造 HTML 正文
    QString body;
    body += buildTitlePage(config, year);
    body += buildKeyWorkSection(tasks, config);
    body += buildSpecialWorkSection(tasks, config);
    if (config.includeAchievements) {
        body += buildAchievementSection(achievements, tasks, config);
    }
    QString html = wrapHtmlBody(body);

    // 写入文件（UTF-8 with BOM，确保 Word 正确识别中文编码）
    QFile file(outputPath);
    if (!file.open(QIODevice::WriteOnly)) {
        result.errorMessage = QStringLiteral("无法打开输出文件进行写入: %1").arg(outputPath);
        Logger::instance().error(QStringLiteral("WordExport: ") + result.errorMessage);
        return result;
    }
    file.write("\xEF\xBB\xBF");  // UTF-8 BOM
    file.write(html.toUtf8());
    file.close();

    result.success = true;
    result.outputPath = outputPath;
    result.itemCount = tasks.size() + achievements.size();
    return result;
}

// ============================================================================
// 私有辅助方法
// ============================================================================

QString WordExport::escapeHtml(const QString &text)
{
    return escapeHtmlStr(text);
}

QString WordExport::formatAmount(qint64 cents, const QString &unit)
{
    if (unit == QStringLiteral("wanyuan")) {
        double wan = cents / 1000000.0;
        return QStringLiteral("%1 万元").arg(QString::number(wan, 'f', 4));
    }
    // 默认 yuan
    double yuan = cents / 100.0;
    return QStringLiteral("%1 元").arg(QString::number(yuan, 'f', 2));
}

QString WordExport::buildTitlePage(const ExportConfig &config, int year)
{
    QString today = QDate::currentDate().toString(config.dateFormat);
    QString html;
    html += QStringLiteral("<div class='title-page'>");
    html += QStringLiteral("<h1>") + escapeHtml(config.reportTitle) + QStringLiteral("</h1>");
    html += QStringLiteral("<div class='title-year'>") + QString::number(year)
            + QStringLiteral(" 年度</div>");
    html += QStringLiteral("<div class='title-date'>") + escapeHtml(today)
            + QStringLiteral("</div>");
    html += QStringLiteral("</div>");
    html += QStringLiteral("<div style='page-break-after:always'></div>");
    return html;
}

QString WordExport::buildKeyWorkSection(const QList<Task> &tasks, const ExportConfig &config)
{
    // 筛选重点任务（排除已删除）
    QList<Task> keyTasks;
    for (const Task &t : tasks) {
        if (t.isKeyTask && !t.isDeleted()) {
            keyTasks.append(t);
        }
    }
    sortTasksBy(keyTasks, config.sortBy);

    QString html;
    html += QStringLiteral("<h2>一、重点工作开展情况</h2>");
    if (keyTasks.isEmpty()) {
        html += QStringLiteral("<p>无重点工作记录。</p>");
        return html;
    }
    html += buildTaskTableHtml(keyTasks, config);
    return html;
}

QString WordExport::buildSpecialWorkSection(const QList<Task> &tasks, const ExportConfig &config)
{
    // 筛选非重点任务（排除已删除）
    QList<Task> specialTasks;
    for (const Task &t : tasks) {
        if (!t.isKeyTask && !t.isDeleted()) {
            specialTasks.append(t);
        }
    }

    QString html;
    html += QStringLiteral("<h2>二、专项工作情况</h2>");
    if (specialTasks.isEmpty()) {
        html += QStringLiteral("<p>无专项工作记录。</p>");
        return html;
    }

    // 按 tag 分组，保留首次出现顺序；空 tag 归入“未分类”
    QVector<QString> tagOrder;
    QMap<QString, QList<Task>> tagGroups;
    for (const Task &t : specialTasks) {
        QString tag = t.tag.trimmed().isEmpty() ? QStringLiteral("未分类") : t.tag;
        if (!tagGroups.contains(tag)) {
            tagOrder.append(tag);
        }
        tagGroups[tag].append(t);
    }

    int index = 1;
    for (const QString &tag : tagOrder) {
        QList<Task> group = tagGroups.value(tag);
        sortTasksBy(group, config.sortBy);
        html += QStringLiteral("<h3>") + QString::number(index)
                + QStringLiteral(". ") + escapeHtml(tag) + QStringLiteral("</h3>");
        html += buildTaskTableHtml(group, config);
        ++index;
    }
    return html;
}

QString WordExport::buildAchievementSection(const QList<Achievement> &achievements,
                                            const QList<Task> &tasks,
                                            const ExportConfig &config)
{
    // 构造 taskId -> title 映射，用于展示关联任务
    QMap<int, QString> taskTitleMap;
    for (const Task &t : tasks) {
        taskTitleMap.insert(t.id, t.title);
    }

    // 筛选年度成果
    QList<Achievement> annual;
    for (const Achievement &a : achievements) {
        if (a.isAnnual) {
            annual.append(a);
        }
    }

    QString html;
    html += QStringLiteral("<h2>三、主要工作成效</h2>");
    if (annual.isEmpty()) {
        html += QStringLiteral("<p>无年度工作成效记录。</p>");
        return html;
    }

    html += QStringLiteral("<table>");
    html += QStringLiteral("<thead><tr>");
    html += QStringLiteral("<th>工作内容</th>");
    html += QStringLiteral("<th>工作对象</th>");
    html += QStringLiteral("<th>成果数量</th>");
    html += QStringLiteral("<th>成果金额</th>");
    html += QStringLiteral("<th>重点</th>");
    html += QStringLiteral("<th>关联任务</th>");
    html += QStringLiteral("</tr></thead><tbody>");

    qint64 totalAmount = 0;
    int totalCount = 0;
    for (const Achievement &a : annual) {
        totalAmount += a.resultAmount;
        totalCount += a.resultCount;
        QString isKey = a.isKeyAchievement ? QStringLiteral("是") : QStringLiteral("否");
        QString taskTitle = taskTitleMap.value(a.taskId);
        html += QStringLiteral("<tr>");
        html += QStringLiteral("<td>") + escapeHtml(a.workContent) + QStringLiteral("</td>");
        html += QStringLiteral("<td>") + escapeHtml(a.workObject) + QStringLiteral("</td>");
        html += QStringLiteral("<td>") + QString::number(a.resultCount) + QStringLiteral("</td>");
        html += QStringLiteral("<td>") + formatAmount(a.resultAmount, config.amountUnit) + QStringLiteral("</td>");
        html += QStringLiteral("<td>") + isKey + QStringLiteral("</td>");
        html += QStringLiteral("<td>") + escapeHtml(taskTitle) + QStringLiteral("</td>");
        html += QStringLiteral("</tr>");
    }

    // 合计行
    if (config.includeAmountSummary) {
        html += QStringLiteral("<tr class='summary-row'>");
        html += QStringLiteral("<td>合计</td>");
        html += QStringLiteral("<td></td>");
        html += QStringLiteral("<td>") + QString::number(totalCount) + QStringLiteral("</td>");
        html += QStringLiteral("<td>") + formatAmount(totalAmount, config.amountUnit) + QStringLiteral("</td>");
        html += QStringLiteral("<td></td>");
        html += QStringLiteral("<td></td>");
        html += QStringLiteral("</tr>");
    }
    html += QStringLiteral("</tbody></table>");
    return html;
}

QString WordExport::wrapHtmlBody(const QString &bodyContent)
{
    QString html;
    html += QStringLiteral(
        "<html xmlns:o='urn:schemas-microsoft-com:office:office' "
        "xmlns:w='urn:schemas-microsoft-com:office:word' "
        "xmlns='http://www.w3.org/TR/REC-html40'>");
    html += QStringLiteral("<head>");
    html += QStringLiteral("<meta http-equiv='Content-Type' content='text/html; charset=utf-8'>");
    html += QStringLiteral("<meta name='ProgId' content='Word.Document'>");
    html += QStringLiteral("<meta name='Generator' content='NIUMA'>");
    html += QStringLiteral("<style>");
    html += QStringLiteral("body { font-family: '宋体', SimSun, serif; font-size: 12pt; line-height: 1.6; }");
    html += QStringLiteral("h1 { text-align: center; font-size: 24pt; }");
    html += QStringLiteral("h2 { font-size: 16pt; margin-top: 18pt; }");
    html += QStringLiteral("h3 { font-size: 14pt; }");
    html += QStringLiteral("table { border-collapse: collapse; width: 100%; margin: 6pt 0; }");
    html += QStringLiteral("td, th { border: 1px solid #000; padding: 4pt 6pt; font-size: 11pt; }");
    html += QStringLiteral("th { background: #e8e8e8; }");
    html += QStringLiteral(".title-page { text-align: center; margin-top: 160pt; }");
    html += QStringLiteral(".title-year { font-size: 20pt; margin-top: 16pt; }");
    html += QStringLiteral(".title-date { font-size: 12pt; margin-top: 32pt; }");
    html += QStringLiteral(".summary-row { font-weight: bold; background: #f2f2f2; }");
    html += QStringLiteral("</style>");
    html += QStringLiteral("</head>");
    html += QStringLiteral("<body>");
    html += bodyContent;
    html += QStringLiteral("</body>");
    html += QStringLiteral("</html>");
    return html;
}
