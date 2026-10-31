#include <QApplication>
#include <QCheckBox>
#include <QDateTime>
#include <QDialog>
#include <QDir>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QSqlQuery>
#include <QTableWidget>
#include <QTimer>

#include "DatabaseManager.h"
#include "PathUtil.h"
#include "RecycleBinView.h"
#include "TaskDao.h"
#include "service/FileService.h"
#include "service/TaskService.h"

namespace {

int insertDeletedTask(const QString &taskNo, const QString &title, const QString &originalStatus,
                      const QDateTime &deletedAt)
{
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("INSERT INTO task (task_no, title, category, source, handler, status, original_status, "
                  "is_key_task, description, create_time, delete_time) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue(taskNo);
    query.addBindValue(title);
    query.addBindValue(QStringLiteral("查"));
    query.addBindValue(QStringLiteral("S10 来源"));
    query.addBindValue(QStringLiteral("S10"));
    query.addBindValue(QStringLiteral("已删除"));
    query.addBindValue(originalStatus);
    query.addBindValue(0);
    query.addBindValue(QStringLiteral("S10 contract fixture"));
    query.addBindValue(deletedAt.addDays(-3).toString("yyyy-MM-dd HH:mm:ss"));
    query.addBindValue(deletedAt.toString("yyyy-MM-dd HH:mm:ss"));
    return query.exec() ? query.lastInsertId().toInt() : -1;
}

bool clearFixture()
{
    QSqlQuery query(DatabaseManager::instance().database());
    return query.exec("DELETE FROM achievement") && query.exec("DELETE FROM task");
}

void closeConfirmation(bool accept)
{
    QTimer::singleShot(0, [accept]() {
        if (auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget())) {
            accept ? dialog->accept() : dialog->reject();
        }
    });
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
    FileService fileService;
    if (!fileService.initializeWorkDir(QDir(root).filePath("work-root")).success) return 4;

    RecycleBinView emptyView;
    emptyView.show();
    app.processEvents();
    const auto *emptyTable = emptyView.findChild<QTableWidget *>("recycleBinTable");
    const auto *emptyState = emptyView.findChild<QLabel *>("recycleBinEmptyState");
    const auto *selectAll = emptyView.findChild<QCheckBox *>("recycleBinSelectAll");
    if (!emptyTable || !emptyState || !selectAll || !emptyState->isVisible() || emptyTable->isVisible()
        || selectAll->isEnabled() || emptyView.findChild<QPushButton *>("recycleBinEmpty")->isEnabled()) return 5;
    emptyView.hide();

    const QDateTime now = QDateTime::currentDateTime();
    const int restoreId = insertDeletedTask("S10-RESTORE", "恢复原状态", "已完成", now.addDays(-1));
    const int bulkId = insertDeletedTask("S10-BULK", "批量删除", "进行中", now.addDays(-2));
    const int singleId = insertDeletedTask("S10-SINGLE", "单项删除", "进行中", now.addDays(-3));
    const int emptyId = insertDeletedTask("S10-EMPTY", "清空删除", "他人办理", now.addDays(-4));
    const int thirtyDayId = insertDeletedTask("S10-30DAY", "保留三十天", "进行中", now.addDays(-30));
    const int expiredId = insertDeletedTask("S10-EXPIRED", "过期清理", "进行中", now.addDays(-31));
    if (restoreId < 0 || bulkId < 0 || singleId < 0 || emptyId < 0 || thirtyDayId < 0 || expiredId < 0) return 6;

    RecycleBinView view;
    view.show();
    app.processEvents();
    auto *table = view.findChild<QTableWidget *>("recycleBinTable");
    if (!table || table->rowCount() != 6 || table->columnCount() != 7
        || table->horizontalHeaderItem(4)->text() != QStringLiteral("剩余天数")) return 7;
    if (table->horizontalHeader()->defaultAlignment() != Qt::AlignCenter
        || table->horizontalHeaderItem(2)->textAlignment() != (Qt::AlignLeft | Qt::AlignVCenter)
        || table->item(0, 1)->textAlignment() != Qt::AlignCenter) return 21;
    if (!view.findChild<QPushButton *>(QStringLiteral("recycleBinRestore_%1").arg(restoreId))
        || !view.findChild<QPushButton *>(QStringLiteral("recycleBinPermanentDelete_%1").arg(singleId))) return 8;

    for (int row = 0; row < table->rowCount(); ++row) {
        if (table->item(row, 1)->text() == QStringLiteral("S10-RESTORE")) {
            auto *checkbox = table->cellWidget(row, 0)->findChild<QCheckBox *>();
            if (!checkbox) return 9;
            checkbox->setChecked(true);
            break;
        }
    }
    if (!QMetaObject::invokeMethod(&view, "onRestoreSelectedClicked", Qt::DirectConnection)) return 10;
    TaskDao dao(DatabaseManager::instance().database());
    const auto restored = dao.findById(restoreId);
    if (!restored.has_value() || restored->status != QStringLiteral("已完成") || restored->deleteTime.isValid()
        || !restored->originalStatus.isEmpty()) return 11;

    view.refresh();
    table = view.findChild<QTableWidget *>("recycleBinTable");
    for (int row = 0; row < table->rowCount(); ++row) {
        if (table->item(row, 1)->text() == QStringLiteral("S10-BULK")) {
            auto *checkbox = table->cellWidget(row, 0)->findChild<QCheckBox *>();
            if (!checkbox) return 12;
            checkbox->setChecked(true);
            break;
        }
    }
    closeConfirmation(false);
    if (!QMetaObject::invokeMethod(&view, "onDeleteSelectedClicked", Qt::DirectConnection)
        || !dao.findById(bulkId).has_value()) return 13;
    closeConfirmation(true);
    if (!QMetaObject::invokeMethod(&view, "onDeleteSelectedClicked", Qt::DirectConnection)
        || dao.findById(bulkId).has_value()) return 14;

    auto *singleDelete = view.findChild<QPushButton *>(QStringLiteral("recycleBinPermanentDelete_%1").arg(singleId));
    if (!singleDelete) return 15;
    closeConfirmation(false);
    singleDelete->click();
    if (!dao.findById(singleId).has_value()) return 16;
    singleDelete = view.findChild<QPushButton *>(QStringLiteral("recycleBinPermanentDelete_%1").arg(singleId));
    if (!singleDelete) return 17;
    closeConfirmation(true);
    singleDelete->click();
    if (dao.findById(singleId).has_value()) return 18;

    TaskService service;
    if (service.cleanExpiredDeleted(30) != 1 || dao.findById(expiredId).has_value()
        || !dao.findById(thirtyDayId).has_value()) return 19;

    view.refresh();
    closeConfirmation(false);
    if (!QMetaObject::invokeMethod(&view, "onEmptyRecycleBinClicked", Qt::DirectConnection)
        || !dao.findById(emptyId).has_value()) return 20;
    closeConfirmation(true);
    if (!QMetaObject::invokeMethod(&view, "onEmptyRecycleBinClicked", Qt::DirectConnection)
        || dao.findById(emptyId).has_value() || dao.findById(thirtyDayId).has_value()) return 21;

    view.refresh();
    if (!view.findChild<QLabel *>("recycleBinEmptyState")->isVisible()
        || view.findChild<QTableWidget *>("recycleBinTable")->isVisible()) return 22;
    return 0;
}
