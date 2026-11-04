#include <QApplication>
#include "text_selection_contract.h"
#include <QAbstractButton>
#include <QComboBox>
#include <QDialog>
#include <QDate>
#include <QDateEdit>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QScrollBar>
#include <QSet>
#include <QShortcut>
#include <QSqlQuery>
#include <QStandardItemModel>
#include <QTableView>
#include <QTextEdit>
#include <QTextStream>
#include <QTimer>
#include <QWheelEvent>

#include "DatabaseManager.h"
#include "ReferenceCreateDialog.h"
#include "ReferenceDetailDialog.h"
#include "ReferenceEditDialog.h"
#include "ReferenceView.h"
#include "database/ReferenceMaterialDao.h"
#include "service/FileService.h"
#include "service/ReferenceInfoFile.h"
#include "utils/PathUtil.h"

namespace {

void closeConfirmation(bool accept)
{
    QTimer::singleShot(0, [accept]() {
        if (auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget())) {
            accept ? dialog->accept() : dialog->reject();
        }
    });
}

QWidget *yearPopup(QComboBox *year)
{
    year->showPopup();
    QApplication::processEvents();
    if (auto *popup = year->findChild<QWidget *>(QStringLiteral("appUiYearPopup"))) return popup;
    for (auto *widget : QApplication::topLevelWidgets())
        if (widget->objectName() == QStringLiteral("appUiYearPopup") && widget->isVisible()) return widget;
    return nullptr;
}

QString widgetDiagnostic(QWidget *widget)
{
    if (!widget) return QStringLiteral("null");
    const QRect geometry = widget->geometry();
    return QStringLiteral("%1 name=%2 pointer=%3 visible=%4 enabled=%5 modal=%6 geometry=%7,%8 %9x%10")
        .arg(QString::fromLatin1(widget->metaObject()->className()), widget->objectName(),
             QString::number(reinterpret_cast<quintptr>(widget), 16))
        .arg(widget->isVisible()).arg(widget->isEnabled()).arg(widget->isModal())
        .arg(geometry.x()).arg(geometry.y()).arg(geometry.width()).arg(geometry.height());
}

QString yearDiagnosticText(const char *stage, QComboBox *year, QWidget *popup)
{
    QString text;
    QTextStream stream(&text);
    stream << stage << '\n' << "selected=" << year->currentData().toInt() << '\n'
           << "year=" << widgetDiagnostic(year) << '\n'
           << "view=" << widgetDiagnostic(year->window()) << '\n'
           << "activeWindow=" << widgetDiagnostic(QApplication::activeWindow()) << '\n'
           << "activeModal=" << widgetDiagnostic(QApplication::activeModalWidget()) << '\n'
           << "activePopup=" << widgetDiagnostic(QApplication::activePopupWidget()) << '\n'
           << "popup=" << widgetDiagnostic(popup) << '\n';
    for (auto *widget : QApplication::topLevelWidgets())
        stream << "topLevel=" << widgetDiagnostic(widget) << '\n';
    stream.flush();
    return text;
}

QString yearGridContext;

bool yearGridFailure(const char *reason, QComboBox *year, QWidget *popup)
{
    QFile file(QDir(qEnvironmentVariable("NIUMA_TEST_ROOT"))
                   .filePath(QStringLiteral("reference-year-diagnostics.txt")));
    if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QTextStream stream(&file);
        stream.setCodec("UTF-8");
        stream << yearGridContext << yearDiagnosticText(reason, year, popup);
    }
    return false;
}

bool pickYearFromGrid(QComboBox *year, int chosenYear, const QString &screenshot)
{
    yearGridContext = yearDiagnosticText("before showPopup", year, nullptr);
    auto *popup = yearPopup(year);
    if (!popup || !popup->isVisible()) return yearGridFailure("popup missing or hidden after showPopup", year, popup);
    auto *previous = popup->findChild<QAbstractButton *>(QStringLiteral("appUiYearPrevious"));
    auto *next = popup->findChild<QAbstractButton *>(QStringLiteral("appUiYearNext"));
    auto *all = popup->findChild<QAbstractButton *>(QStringLiteral("appUiYearAll"));
    const auto cells = popup->findChildren<QAbstractButton *>(QStringLiteral("appUiYearCell"));
    QSet<int> years;
    for (auto *cell : cells) years.insert(cell->property("year").toInt());
    if (!previous || !next || !all || cells.size() != 12 || years.size() != 12)
        return yearGridFailure("missing controls or distinct year cells", year, popup);
    next->click();
    QSet<int> nextYears;
    for (auto *cell : popup->findChildren<QAbstractButton *>(QStringLiteral("appUiYearCell")))
        nextYears.insert(cell->property("year").toInt());
    for (int value : years) if (!nextYears.contains(value + 10))
        return yearGridFailure("next-decade years do not advance by ten", year, popup);
    previous->click();
    if (!popup->grab().save(screenshot)) return yearGridFailure("cannot save grid capture", year, popup);
    for (auto *cell : popup->findChildren<QAbstractButton *>(QStringLiteral("appUiYearCell"))) {
        if (cell->property("year").toInt() == chosenYear) {
            cell->click();
            QApplication::processEvents();
            if (year->currentData().toInt() != chosenYear || popup->isVisible())
                return yearGridFailure("selection not applied or popup did not close", year, popup);
            return true;
        }
    }
    return yearGridFailure("chosen year not present after previous-decade navigation", year, popup);
}

int verifyFilterContract(ReferenceView &view, QTableView *table)
{
    QSqlQuery query(DatabaseManager::instance().database());
    if (!query.exec("DELETE FROM reference_material")) return 40;
    const int currentYear = QDate::currentDate().year();
    const int previousYear = currentYear - 1;
    struct FilterRow { const char *number; const char *source; const char *tag; int year; };
    const FilterRow rows[] = {
        {"REF-FILTER-001", "来源甲", "标签甲", currentYear},
        {"REF-FILTER-002", "来源乙", "标签乙", currentYear},
        {"REF-FILTER-003", "来源甲", "标签甲", previousYear}
    };
    for (const auto &row : rows) {
        query.prepare("INSERT INTO reference_material (ref_no,name,source,tag,create_time) VALUES (?,?,?,?,?)");
        query.addBindValue(QString::fromLatin1(row.number));
        query.addBindValue(QStringLiteral("年度筛选资料"));
        query.addBindValue(QString::fromUtf8(row.source));
        query.addBindValue(QString::fromUtf8(row.tag));
        query.addBindValue(QStringLiteral("%1-06-15 12:00:00").arg(row.year));
        if (!query.exec()) return 41;
    }
    QMetaObject::invokeMethod(&view, "onResetClicked", Qt::DirectConnection);
    view.refresh();
    auto *year = view.findChild<QComboBox *>(QStringLiteral("referenceYearCombo"));
    auto *source = view.findChild<QComboBox *>(QStringLiteral("referenceSourceCombo"));
    auto *tag = view.findChild<QComboBox *>(QStringLiteral("referenceTagCombo"));
    auto *search = view.findChild<QLineEdit *>(QStringLiteral("referenceSearchEdit"));
    auto *start = view.findChildren<QDateEdit *>().value(0);
    auto *end = view.findChildren<QDateEdit *>().value(1);
    if (!year || !source || !tag || !search || !start || !end) return 42;
    if (year->currentText() != QString::number(currentYear)
        || start->date() != QDate(currentYear, 1, 1) || end->date() != QDate(currentYear, 12, 31)
        || table->model()->rowCount() != 2) return 43;
    search->setText(QStringLiteral("尚未搜索"));
    start->setDate(QDate(previousYear, 1, 1));
    end->setDate(QDate(previousYear, 12, 31));
    const QList<QComboBox *> instantFilters = {source, tag};
    const QStringList choices = {QStringLiteral("来源甲"), QStringLiteral("标签甲")};
    for (int index = 0; index < instantFilters.size(); ++index) {
        instantFilters.at(index)->setCurrentText(choices.at(index));
        if (table->model()->rowCount() != 1) return 44 + index;
        view.refresh();
        if (instantFilters.at(index)->currentText() != choices.at(index)
            || search->text() != QStringLiteral("尚未搜索")
            || start->date() != QDate(previousYear, 1, 1)
            || end->date() != QDate(previousYear, 12, 31)
            || table->model()->rowCount() != 1) return 46 + index;
        instantFilters.at(index)->setCurrentIndex(0);
        if (table->model()->rowCount() != 2) return 48 + index;
    }
    start->setDate(QDate(currentYear, 7, 1));
    end->setDate(QDate(currentYear, 7, 31));
    if (!pickYearFromGrid(year, previousYear,
            QDir(qEnvironmentVariable("NIUMA_TEST_ROOT")).filePath(QStringLiteral("reference-year-picker.png"))))
        return 59;
    if (start->date() != QDate(previousYear, 1, 1) || end->date() != QDate(previousYear, 12, 31)
        || table->model()->rowCount() != 1 || search->text() != QStringLiteral("尚未搜索")) return 50;
    view.refresh();
    if (year->currentText() != QString::number(previousYear) || table->model()->rowCount() != 1)
        return 51;
    source->setCurrentText(QStringLiteral("来源甲"));
    if (table->model()->rowCount() != 1
        || table->model()->index(0, 0).data().toString() != QStringLiteral("REF-FILTER-003")) return 58;
    source->setCurrentIndex(0);
    if (table->model()->rowCount() != 1
        || table->model()->index(0, 0).data().toString() != QStringLiteral("REF-FILTER-003")) return 52;
    auto *popup = yearPopup(year);
    auto *all = popup ? popup->findChild<QAbstractButton *>(QStringLiteral("appUiYearAll")) : nullptr;
    if (!all) return 60;
    all->click();
    if (start->date() != QDate(currentYear, 1, 1) || end->date() != QDate(currentYear, 12, 31)
        || table->model()->rowCount() != 3 || year->currentData().toInt() != 0) return 53;
    if (table->model()->rowCount() != 3) return 54;
    search->clear();
    start->setDate(QDate(previousYear, 1, 1));
    end->setDate(QDate(previousYear, 12, 31));
    if (table->model()->rowCount() != 3) return 55;
    QMetaObject::invokeMethod(&view, "onSearchClicked", Qt::DirectConnection);
    if (table->model()->rowCount() != 1) return 56;
    year->setCurrentText(QString::number(currentYear));
    search->setText(QStringLiteral("REF-FILTER-002"));
    QMetaObject::invokeMethod(&view, "onSearchClicked", Qt::DirectConnection);
    if (table->model()->rowCount() != 1
        || table->model()->index(0, 0).data().toString() != QStringLiteral("REF-FILTER-002")) return 57;

    // Keep the submitted number search while year selection resets a draft manual range.
    search->setText(QStringLiteral("REF-FILTER-003"));
    start->setDate(QDate(currentYear, 7, 1));
    end->setDate(QDate(currentYear, 7, 31));
    year->setCurrentText(QString::number(previousYear));
    auto *empty = view.findChild<QLabel *>(QStringLiteral("referenceEmptyState"));
    if (table->model()->rowCount() != 0 || !empty || !empty->isVisible()
        || empty->text() != QStringLiteral("没有符合当前筛选条件的资料")
        || start->date() != QDate(previousYear, 1, 1) || end->date() != QDate(previousYear, 12, 31)
        || search->text() != QStringLiteral("REF-FILTER-003")) return 61;
    year->setCurrentText(QStringLiteral("全部年度"));
    if (table->model()->rowCount() != 1
        || table->model()->index(0, 0).data().toString() != QStringLiteral("REF-FILTER-002")) return 62;
    QMetaObject::invokeMethod(&view, "onSearchClicked", Qt::DirectConnection);
    if (table->model()->rowCount() != 1
        || table->model()->index(0, 0).data().toString() != QStringLiteral("REF-FILTER-003")) return 63;
    source->setCurrentText(QStringLiteral("来源乙"));
    if (table->model()->rowCount() != 0 || !empty->isVisible()) return 64;
    source->setCurrentIndex(0);
    year->setCurrentText(QString::number(currentYear));
    search->setText(QStringLiteral("年度筛选资料"));
    source->setCurrentText(QStringLiteral("来源甲"));
    QMetaObject::invokeMethod(&view, "onSearchClicked", Qt::DirectConnection);
    if (table->model()->rowCount() != 1) return 65;
    tag->setCurrentText(QStringLiteral("标签乙"));
    if (table->model()->rowCount() != 0 || !empty->isVisible()) return 66;
    tag->setCurrentIndex(0);
    start->setDate(QDate(currentYear, 7, 1));
    end->setDate(QDate(currentYear, 7, 31));
    if (table->model()->rowCount() != 1) return 67;
    QMetaObject::invokeMethod(&view, "onSearchClicked", Qt::DirectConnection);
    if (table->model()->rowCount() != 0 || !empty->isVisible()) return 68;
    view.resize(1200, 800);
    QApplication::processEvents();
    if (!view.grab().save(QDir(qEnvironmentVariable("NIUMA_TEST_ROOT"))
                             .filePath(QStringLiteral("reference-filter-zero.png")))) return 69;
    popup = yearPopup(year);
    bool selectedSameYear = false;
    if (popup) for (auto *cell : popup->findChildren<QAbstractButton *>(QStringLiteral("appUiYearCell"))) {
        if (cell->property("year").toInt() == currentYear) {
            cell->click();
            selectedSameYear = true;
            break;
        }
    }
    if (!selectedSameYear || table->model()->rowCount() != 1
        || start->date() != QDate(currentYear, 1, 1) || end->date() != QDate(currentYear, 12, 31))
        return 70;
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
    QSqlQuery query(DatabaseManager::instance().database());
    if (!query.exec("DELETE FROM reference_material")) return 3;
    FileService fileService;
    if (!fileService.initializeWorkDir(QDir(root).filePath("work-root")).success) return 4;

    ReferenceView view;
    view.resize(300, 420);
    view.show();
    view.refresh();
    app.processEvents();
    auto *table = view.findChild<QTableView *>(QStringLiteral("referenceTable"));
    auto *empty = view.findChild<QLabel *>(QStringLiteral("referenceEmptyState"));
    if (!table || !empty || table->model()->rowCount() != 0 || !empty->isVisible()) return 5;
    if (empty->parentWidget() != table->viewport()
        || (empty->geometry().center() - table->viewport()->rect().center()).manhattanLength() > 1) return 22;
    if (!view.grab().save(QDir(root).filePath(QStringLiteral("reference-empty.png")))) return 23;

    ReferenceCreateDialog create;
    create.show();
    app.processEvents();
    app.processEvents();
    auto *name = create.findChild<QLineEdit *>(QStringLiteral("referenceNameEdit"));
    auto *description = create.findChild<QTextEdit *>(QStringLiteral("referenceDescriptionEdit"));
    auto *source = create.findChild<QComboBox *>(QStringLiteral("referenceSourceCombo"));
    auto *tag = create.findChild<QComboBox *>(QStringLiteral("referenceTagCombo"));
    auto *attachment = create.findChild<QWidget *>(QStringLiteral("referenceAttachmentArea"));
    if (!name || !description || !source || !tag || create.minimumWidth() != 560
        || description->height() != 240 || !attachment || attachment->height() != 120
        || !source->currentText().isEmpty() || !tag->currentText().isEmpty()) return 6;
    if (create.testAttribute(Qt::WA_TranslucentBackground) || !create.mask().isEmpty()
        || !create.property("nativePopupHandle").toULongLong()) return 21;
    QKeyEvent escape(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
    QApplication::sendEvent(&create, &escape);
    if (create.result() != QDialog::Rejected) return 7;

    ReferenceCreateDialog createSaved;
    auto *savedName = createSaved.findChild<QLineEdit *>(QStringLiteral("referenceNameEdit"));
    auto *savedDescription = createSaved.findChild<QTextEdit *>(QStringLiteral("referenceDescriptionEdit"));
    auto *savedSource = createSaved.findChild<QComboBox *>(QStringLiteral("referenceSourceCombo"));
    auto *savedTag = createSaved.findChild<QComboBox *>(QStringLiteral("referenceTagCombo"));
    auto *createSave = createSaved.findChild<QShortcut *>(QStringLiteral("shortcutSaveReference"));
    if (!savedName || !savedDescription || !savedSource || !savedTag
        || !createSave || createSave->key() != QKeySequence::Save) return 8;
    savedName->setText(QStringLiteral("S07 资料"));
    savedDescription->setPlainText(QStringLiteral("S07 创建说明"));
    savedSource->setCurrentText(QStringLiteral("S07 来源"));
    savedTag->setCurrentText(QStringLiteral("S07 标签"));
    if (!QMetaObject::invokeMethod(createSave, "activated", Qt::DirectConnection)) return 9;

    ReferenceMaterialDao dao(DatabaseManager::instance().database());
    auto refs = dao.findAll();
    if (refs.size() != 1 || refs.first().name != QStringLiteral("S07 资料")
        || refs.first().folderPath.isEmpty() || !QDir(refs.first().folderPath).exists()) return 10;
    const int refId = refs.first().id;
    const QString folderPath = refs.first().folderPath;
    const auto savedInfo = ReferenceInfoFile::read(folderPath);
    if (!savedInfo || savedInfo->name != refs.first().name
        || savedInfo->source != refs.first().source
        || savedInfo->tag != refs.first().tag
        || savedInfo->description != refs.first().description
        || !QFileInfo(ReferenceInfoFile::path(folderPath)).isHidden()) return 26;

    view.refresh();
    app.processEvents();
    if (table->model()->rowCount() != 1 || empty->isVisible()
        || table->model()->index(0, 1).data().toString() != QStringLiteral("S07 资料")) return 11;
    auto *search = view.findChild<QLineEdit *>(QStringLiteral("referenceSearchEdit"));
    if (!search) return 12;
    search->setText(QStringLiteral("不会命中"));
    if (!QMetaObject::invokeMethod(&view, "onSearchClicked", Qt::DirectConnection)
        || table->model()->rowCount() != 0 || !empty->isVisible()) return 13;
    search->setText(QStringLiteral("S07 资料"));
    if (!QMetaObject::invokeMethod(&view, "onSearchClicked", Qt::DirectConnection)
        || table->model()->rowCount() != 1) return 14;
    search->setText(QStringLiteral("不会命中"));
    if (!QMetaObject::invokeMethod(&view, "onSearchClicked", Qt::DirectConnection)) return 24;
    view.showCreatedReference();
    if (!search->text().isEmpty() || table->model()->rowCount() != 1) return 25;

    table->setFixedWidth(300);
    app.processEvents();
    QScrollBar *horizontal = table->horizontalScrollBar();
    if (horizontal->maximum() <= horizontal->minimum()) return 15;
    horizontal->setValue(horizontal->maximum());
    const int beforeWheel = horizontal->value();
    QWheelEvent wheel(QPointF(10, 10), QPointF(10, 10), QPoint(), QPoint(0, 120),
                      Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
    QApplication::sendEvent(table->viewport(), &wheel);
    if (horizontal->value() >= beforeWheel) return 16;

    ReferenceEditDialog edit(refId);
    auto *editName = edit.findChild<QLineEdit *>(QStringLiteral("referenceNameEdit"));
    auto *editDescription = edit.findChild<QTextEdit *>(QStringLiteral("referenceDescriptionEdit"));
    auto *editSave = edit.findChild<QShortcut *>(QStringLiteral("shortcutSaveReference"));
    if (!editName || !editDescription || editName->text() != QStringLiteral("S07 资料")
        || !editSave || editSave->key() != QKeySequence::Save) return 17;
    editName->setText(QStringLiteral("S07 编辑资料"));
    editDescription->setPlainText(QStringLiteral("S07 编辑说明"));
    if (!QMetaObject::invokeMethod(editSave, "activated", Qt::DirectConnection)) return 18;
    const auto updatedRef = dao.findById(refId);
    if (!updatedRef.has_value() || updatedRef->name != QStringLiteral("S07 编辑资料")
        || updatedRef->folderPath == folderPath || !QDir(updatedRef->folderPath).exists()
        || !ReferenceInfoFile::read(updatedRef->folderPath)
        || ReferenceInfoFile::read(updatedRef->folderPath)->name != updatedRef->name) return 19;

    ReferenceDetailDialog detail(refId);
    auto *detailTitle = detail.findChild<QLabel *>(QStringLiteral("appUiReferenceDetailTitle"));
    if (!detailTitle || detailTitle->text() != QStringLiteral("S07 编辑资料")
        || !detail.findChild<QWidget *>(QStringLiteral("basicInfoSection"))) return 20;
    detail.show();
    closeConfirmation(false);
    if (!QMetaObject::invokeMethod(&detail, "onDeleteClicked", Qt::DirectConnection)
        || !dao.findById(refId).has_value() || !QDir(updatedRef->folderPath).exists()) return 21;
    closeConfirmation(true);
    if (!QMetaObject::invokeMethod(&detail, "onDeleteClicked", Qt::DirectConnection)
        || dao.findById(refId).has_value() || QDir(updatedRef->folderPath).exists()) return 22;
    // Finish the previous modal window's native close/activation events before opening a popup.
    app.processEvents();
    if (QApplication::activeModalWidget()) return 31;
    view.show();
    view.raise();
    view.activateWindow();
    app.processEvents();
    if (QApplication::activeModalWidget()) return 32;

    ReferenceMaterial first;
    first.refNo = QStringLiteral("20200101-001");
    first.name = QStringLiteral("较早编号");
    ReferenceMaterial later = first;
    later.refNo = QStringLiteral("20200101-002");
    later.name = QStringLiteral("较新编号");
    if (dao.insert(first) <= 0 || dao.insert(later) <= 0
        || !query.exec("UPDATE reference_material SET create_time='2030-01-01 00:00:00' WHERE ref_no='20200101-001'"))
        return 27;
    const auto ordered = dao.findAll();
    if (ordered.size() != 2 || ordered.first().refNo != later.refNo) return 28;
    auto *year = view.findChild<QComboBox *>(QStringLiteral("referenceYearCombo"));
    if (!year || year->findText(QStringLiteral("全部年度")) < 0) return 30;
    year->setCurrentText(QStringLiteral("全部年度"));
    QMetaObject::invokeMethod(&view, "onSearchClicked", Qt::DirectConnection);
    view.refresh();
    if (table->model()->index(0, 0).data().toString() != later.refNo) return 29;
    view.resize(1200, 800);
    app.processEvents();
    if (!verifyCellSelection(table, 1)) return 180;
    const int filterResult = verifyFilterContract(view, table);
    if (filterResult != 0) return filterResult;

    DatabaseManager::instance().close();
    return 0;
}
