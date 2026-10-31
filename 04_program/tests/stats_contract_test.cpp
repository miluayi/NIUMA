#include <QApplication>
#include <QComboBox>
#include <QDate>
#include <QDir>
#include <QLabel>
#include <QSqlQuery>
#include <QTabWidget>

#include "DatabaseManager.h"
#include "PathUtil.h"
#include "TaskDao.h"
#include "StatsView.h"

namespace {
bool insertTask(const QString &taskNo, const QString &title, const QString &category,
                const QString &source, const QString &status, const QString &createdAt,
                const QVariant &finishedAt = QVariant())
{
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("INSERT INTO task (task_no, title, category, source, handler, status, is_key_task, description, create_time, finish_time) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue(taskNo); query.addBindValue(title); query.addBindValue(category);
    query.addBindValue(source); query.addBindValue(QStringLiteral("S09")); query.addBindValue(status);
    query.addBindValue(0); query.addBindValue(QStringLiteral("S09 contract fixture"));
    query.addBindValue(createdAt); query.addBindValue(finishedAt);
    return query.exec();
}

bool clearFixture()
{
    QSqlQuery query(DatabaseManager::instance().database());
    return query.exec("DELETE FROM achievement") && query.exec("DELETE FROM task");
}

bool containsLabelText(const QWidget &widget, const QString &text)
{
    for (const QLabel *label : widget.findChildren<QLabel *>()) {
        if (label->text() == text) return true;
    }
    return false;
}

int heatmapCount(const QList<HeatmapData> &items, const QDate &date)
{
    for (const HeatmapData &item : items) {
        if (item.date == date) return item.count;
    }
    return 0;
}
} // namespace

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    const QString root = qEnvironmentVariable("NIUMA_TEST_ROOT");
    if (root.isEmpty()) return 1;
    qputenv("NIUMA_DATA_DIR", QDir(root).filePath("Data").toUtf8());
    if (!QDir().mkpath(root) || !DatabaseManager::instance().initialize(PathUtil::databasePath())) return 2;
    if (!clearFixture()) return 3;

    StatsView emptyView;
    emptyView.show();
    app.processEvents();
    auto *tabs = emptyView.findChild<QTabWidget *>("statsTabs");
    if (!tabs || tabs->count() != 3 || tabs->tabText(0) != QStringLiteral("任务统计")
        || tabs->tabText(1) != QStringLiteral("趋势图") || tabs->tabText(2) != QStringLiteral("热力图")) return 4;
    const auto *emptyTotal = emptyView.findChild<QLabel *>("statsTotal");
    if (!emptyTotal || emptyTotal->text() != QStringLiteral("0")
        || !emptyView.findChild<QLabel *>("statsCategoryEmpty") || !emptyView.findChild<QLabel *>("statsSourceEmpty")
        || !emptyView.findChild<QWidget *>("statsTrendChart") || !emptyView.findChild<QWidget *>("statsHeatmapWidget")) return 5;
    emptyView.hide();

    const int year = QDate::currentDate().year();
    if (!insertTask("S09-001", "一月进行中", "查", "S09 来源A", "进行中", QStringLiteral("%1-01-10 09:00:00").arg(year))
        || !insertTask("S09-002", "二月已完成", "报", "S09 来源A", "已完成", QStringLiteral("%1-02-05 09:00:00").arg(year), QStringLiteral("%1-02-07 10:00:00").arg(year))
        || !insertTask("S09-003", "二月进行中", "办", "S09 来源B", "进行中", QStringLiteral("%1-02-05 11:00:00").arg(year))) return 6;

    TaskDao dao(DatabaseManager::instance().database());
    const TaskStats stats = dao.getStatsByDateRange(QDate(2000, 1, 1), QDate(2099, 12, 31));
    const auto trend = dao.getMonthlyTrend(year);
    const auto createdHeatmap = dao.getHeatmapData(year, "create_time");
    const auto finishedHeatmap = dao.getHeatmapData(year, "finish_time");
    if (stats.totalCount != 3 || stats.completedCount != 1 || stats.inProgressCount != 2 || trend.size() != 12
        || trend.at(0).count != 1 || trend.at(1).count != 2
        || heatmapCount(createdHeatmap, QDate(year, 1, 10)) != 1
        || heatmapCount(createdHeatmap, QDate(year, 2, 5)) != 2
        || heatmapCount(finishedHeatmap, QDate(year, 2, 7)) != 1) return 7;

    StatsView view;
    view.show();
    app.processEvents();
    const auto *total = view.findChild<QLabel *>("statsTotal");
    const auto *completed = view.findChild<QLabel *>("statsCompleted");
    const auto *inProgress = view.findChild<QLabel *>("statsInProgress");
    const auto *rate = view.findChild<QLabel *>("statsCompletionRate");
    if (!total || !completed || !inProgress || !rate || total->text() != QStringLiteral("3")
        || completed->text() != QStringLiteral("1") || inProgress->text() != QStringLiteral("2")
        || rate->text() != QStringLiteral("33.3%") || !containsLabelText(view, QStringLiteral("S09 来源A"))
        || !containsLabelText(view, QStringLiteral("2 项"))) return 8;

    auto *populatedTabs = view.findChild<QTabWidget *>("statsTabs");
    populatedTabs->setCurrentIndex(1);
    app.processEvents();
    const auto *trendTitle = view.findChild<QLabel *>("statsTrendTitle");
    if (!trendTitle || !trendTitle->text().contains(QString::number(year))
        || !view.findChild<QWidget *>("statsTrendChart")->isVisible()) return 9;
    populatedTabs->setCurrentIndex(2);
    auto *dimension = view.findChild<QComboBox *>("statsHeatmapDimensionCombo");
    auto *yearCombo = view.findChild<QComboBox *>("statsHeatmapYearCombo");
    if (!dimension || !yearCombo || yearCombo->width() != 104
        || yearCombo->currentText() != QString::number(QDate::currentDate().year())) return 10;
    dimension->setCurrentIndex(1);
    app.processEvents();
    if (!view.findChild<QWidget *>("statsHeatmapWidget")->isVisible()) return 11;

    if (!clearFixture()) return 12;
    view.refresh();
    if (view.findChild<QLabel *>("statsTotal")->text() != QStringLiteral("0")
        || !view.findChild<QLabel *>("statsCategoryEmpty") || !view.findChild<QLabel *>("statsSourceEmpty")) return 13;
    return 0;
}
