#pragma once
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QLabel>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMouseEvent>
#include <QTableView>
#include "widgets/CopyableTableView.h"

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
    if(!table->grab().save(QDir(qEnvironmentVariable("NIUMA_TEST_ROOT")).filePath("text-selection-before.png"))) return false;
    const auto cursor=table->viewport()->cursor().shape();
    const QString original=index.data().toString();
    const QPoint start(rect.left()+9, rect.center().y());
    const QPoint end(qMin(rect.right()-3, rect.left()+85), rect.center().y());
    int clicks = 0;
    const auto connection = QObject::connect(table, &QAbstractItemView::clicked,
        table, [&clicks]() { ++clicks; });
    selectionMouse(table->viewport(), QEvent::MouseButtonPress, start);
    selectionMouse(table->viewport(), QEvent::MouseMove, end);
    selectionMouse(table->viewport(), QEvent::MouseButtonRelease, end);
    const auto *view=dynamic_cast<CopyableTableView *>(table);
    const auto *list=dynamic_cast<CopyableListWidget *>(table);
    const QString text=view ? view->selectedText() : list ? list->selectedText() : QString();
    if (text.isEmpty() || clicks!=0) {QObject::disconnect(connection);return false;}
    QApplication::clipboard()->clear();
    selectionCopy(table);
    const bool copied = QApplication::clipboard()->text() == text
        && original.contains(text) && index.data().toString()==original
        && table->visualRect(index)==rect && table->viewport()->cursor().shape()==cursor
        && !table->findChild<QLineEdit *>(QStringLiteral("cellCopyText"));
    const QImage highlight=table->viewport()->grab(rect).toImage();
    int blue=0;
    for(int y=0;y<highlight.height();++y) for(int x=0;x<highlight.width();++x) {
        const QColor color=highlight.pixelColor(x,y);
        if(color.red()<70 && color.green()>70 && color.green()<140 && color.blue()>200) ++blue;
    }
    if(!table->grab().save(QDir(qEnvironmentVariable("NIUMA_TEST_ROOT")).filePath("text-selection.png"))) {
        QObject::disconnect(connection);return false;
    }
    selectionMouse(table->viewport(),QEvent::MouseButtonPress,start);
    selectionMouse(table->viewport(),QEvent::MouseButtonRelease,start);
    QObject::disconnect(connection);
    return copied && clicks==1 && blue>4;
}

inline bool verifyLabelSelection(QLabel *label)
{
    if (!label || !(label->textInteractionFlags() & Qt::TextSelectableByMouse)) return false;
    const auto cursor=label->cursor().shape();
    const QRect geometry=label->geometry();
    const QPoint start(8, label->height()/2);
    const QPoint end(label->width()-8, label->height()/2);
    selectionMouse(label, QEvent::MouseButtonPress, start);
    selectionMouse(label, QEvent::MouseMove, end);
    selectionMouse(label, QEvent::MouseButtonRelease, end);
    if (label->selectedText().isEmpty()) return false;
    QApplication::clipboard()->clear();
    selectionCopy(label);
    return QApplication::clipboard()->text() == label->selectedText()
        && label->cursor().shape()==cursor && label->geometry()==geometry;
}
