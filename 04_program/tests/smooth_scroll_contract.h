#pragma once
#include <QElapsedTimer>
#include <QThread>
#include <QWheelEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QSet>

inline void scrollWheel(QWidget *viewport, int angle, int pixel = 0)
{
    const QPoint point(20,20);
    QWheelEvent event(QPointF(point),QPointF(viewport->mapToGlobal(point)),QPoint(0,pixel),
                     QPoint(0,angle),Qt::NoButton,Qt::NoModifier,Qt::NoScrollPhase,false);
    QApplication::sendEvent(viewport,&event);
}

inline QSet<int> observeScroll(QScrollBar *bar)
{
    QSet<int> values;
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed()<240) {
        QApplication::processEvents();
        values.insert(bar->value());
        QThread::msleep(5);
    }
    return values;
}

inline int verifySmoothScrolling(MainWindow &window, QListWidget *sidebar)
{
    sidebar->setCurrentRow(8);
    QApplication::processEvents();
    auto *list=window.findChild<QListWidget *>("settingsSourceList");
    if (!list || list->verticalScrollMode()!=QAbstractItemView::ScrollPerPixel) return 290;
    const int originalCount=list->count();
    for (int index=0; index<60; ++index) list->addItem(QStringLiteral("滚动测试 %1").arg(index));
    QApplication::processEvents();
    auto *bar=list->verticalScrollBar();
    bar->setValue(bar->minimum());
    scrollWheel(list->viewport(),-120);
    const auto frames=observeScroll(bar);
    const int listDistance=bar->value()-bar->minimum();
    if (frames.size()<3 || listDistance<30 || listDistance>112) return 291;
    const int beforePixel=bar->value();
    scrollWheel(list->viewport(),0,-7);
    if (bar->value()!=beforePixel+7) return 292;
    const int beforeHalf=bar->value();
    scrollWheel(list->viewport(),-60);
    observeScroll(bar);
    if (qAbs((bar->value()-beforeHalf)*2-listDistance)>2) return 293;
    QScrollArea *page=nullptr;
    for (QWidget *parent=list->parentWidget();parent;parent=parent->parentWidget())
        if ((page=qobject_cast<QScrollArea *>(parent))) break;
    if (!page || page->verticalScrollBar()->maximum()==0) return 294;
    auto *pageBar=page->verticalScrollBar();
    pageBar->setValue(pageBar->minimum());
    scrollWheel(page->viewport(),-120);
    if (observeScroll(pageBar).size()<3 || pageBar->value()<=listDistance) return 295;
    pageBar->setValue(pageBar->minimum());
    bar->setValue(bar->maximum());
    scrollWheel(list->viewport(),-120);
    observeScroll(pageBar);
    if (pageBar->value()==pageBar->minimum() || bar->value()!=bar->maximum()) return 296;
    while (list->count()>originalCount) delete list->takeItem(list->count()-1);
    sidebar->setCurrentRow(3);
    QApplication::processEvents();
    return 0;
}
