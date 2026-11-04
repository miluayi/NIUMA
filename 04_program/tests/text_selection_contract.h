#pragma once
#include <QApplication>
#include <QClipboard>
#include <QLabel>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMouseEvent>
#include <QTableView>

inline void selectionMouse(QWidget *widget, QEvent::Type type, QPoint point)
{
    QMouseEvent event(type, point, widget->mapToGlobal(point),
        type == QEvent::MouseMove ? Qt::NoButton : Qt::LeftButton,
        type == QEvent::MouseButtonRelease ? Qt::NoButton : Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(widget, &event);
}

inline void selectionCopy(QWidget *widget)
{
    QKeyEvent press(QEvent::KeyPress, Qt::Key_C, Qt::ControlModifier);
    QApplication::sendEvent(widget, &press);
    QKeyEvent release(QEvent::KeyRelease, Qt::Key_C, Qt::ControlModifier);
    QApplication::sendEvent(widget, &release);
}

inline bool verifyCellSelection(QAbstractItemView *table, int column)
{
    const QModelIndex index = table->model()->index(0, column);
    if (!index.isValid()) return false;
    table->scrollTo(index);
    QApplication::processEvents();
    const QRect rect = table->visualRect(index);
    const QPoint start(rect.left()+9, rect.center().y());
    const QPoint end(qMin(rect.right()-3, rect.left()+85), rect.center().y());
    int clicks = 0;
    const auto connection = QObject::connect(table, &QAbstractItemView::clicked,
        table, [&clicks]() { ++clicks; });
    selectionMouse(table->viewport(), QEvent::MouseButtonPress, start);
    selectionMouse(table->viewport(), QEvent::MouseMove, end);
    selectionMouse(table->viewport(), QEvent::MouseButtonRelease, end);
    QObject::disconnect(connection);
    auto *editor = table->findChild<QLineEdit *>(QStringLiteral("cellCopyText"));
    if (!editor || !editor->isVisible() || !editor->isReadOnly()
        || editor->selectedText().isEmpty() || clicks != 0) return false;
    const QString text = editor->selectedText();
    QApplication::clipboard()->clear();
    selectionCopy(editor);
    const bool copied = QApplication::clipboard()->text() == text
        && index.data().toString().contains(text);
    editor->clearFocus();
    return copied;
}

inline bool verifyLabelSelection(QLabel *label)
{
    if (!label || !(label->textInteractionFlags() & Qt::TextSelectableByMouse)) return false;
    const QPoint start(8, label->height()/2);
    const QPoint end(label->width()-8, label->height()/2);
    selectionMouse(label, QEvent::MouseButtonPress, start);
    selectionMouse(label, QEvent::MouseMove, end);
    selectionMouse(label, QEvent::MouseButtonRelease, end);
    if (label->selectedText().isEmpty()) return false;
    QApplication::clipboard()->clear();
    selectionCopy(label);
    return QApplication::clipboard()->text() == label->selectedText();
}
