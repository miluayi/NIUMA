#pragma once
#include <QImage>
#include <QPainter>
#include <QDir>
#include "widgets/TaskItemDelegate.h"

inline bool verifyCategoryPixels(const QImage &actual,const QRect &cell,const QString &category) {
    QImage expected(actual.size(),QImage::Format_ARGB32_Premultiplied);
    const qreal scale=actual.devicePixelRatio();
    expected.setDevicePixelRatio(scale);
    expected.fill(Qt::white);
    {QPainter painter(&expected);TaskItemDelegate::paintCategoryBadge(&painter,cell,category);}
    QRectF badge(0,0,TaskItemDelegate::categoryBadgeWidth(category),24);
    badge.moveCenter(QRectF(cell).center());
    badge.adjust(2,2,-2,-2);
    for(int y=qCeil(badge.top()*scale);y<qFloor(badge.bottom()*scale);++y)
        for(int x=qCeil(badge.left()*scale);x<qFloor(badge.right()*scale);++x) {
            if(actual.pixelColor(x,y).rgb()!=expected.pixelColor(x,y).rgb()) {
                fprintf(stderr,"Category surface differs from task-list standard at %d,%d\n",x,y);return false;
            }
        }
    return true;
}

inline bool verifyCategoryWidget(QWidget *widget,const QString &category,const QString &path) {
    if (!widget) return false;
    const QImage image=widget->grab().toImage();
    return image.save(path) && verifyCategoryPixels(image,widget->rect(),category);
}

inline bool verifyCategoryCentering(const QString &directory,qreal scale) {
    for (const QString &category : {QStringLiteral("催"),QStringLiteral("查"),QStringLiteral("报"),QStringLiteral("办")}) {
        QImage image(qRound(50*scale),qRound(40*scale),QImage::Format_ARGB32_Premultiplied);
        image.setDevicePixelRatio(scale);
        image.fill(Qt::white);
        {QPainter painter(&image);TaskItemDelegate::paintCategoryBadge(&painter,QRect(0,0,50,40),category);}
        QRect ink;
        QRect body;
        const QColor background=category==QStringLiteral("催") ? QColor("#C586C0")
            : category==QStringLiteral("查") ? QColor("#DCDCAA")
            : category==QStringLiteral("报") ? QColor("#569CD6") : QColor("#6A9955");
        for(int y=0;y<image.height();++y) for(int x=0;x<image.width();++x) {
            const QColor color=image.pixelColor(x,y);
            if(color.red()<70 && color.green()<70 && color.blue()<70)
                ink=ink.united(QRect(x,y,1,1));
            if(color.rgb()==background.rgb()) body=body.united(QRect(x,y,1,1));
        }
        if (!image.save(QDir(directory).filePath(QString("category-%1.png").arg(category)))) return false;
        const QPointF center(ink.x()+ink.width()/2.0,ink.y()+ink.height()/2.0);
        const QPointF bodyCenter(body.x()+body.width()/2.0,body.y()+body.height()/2.0);
        const qreal tolerance=qMin(1.0,0.75*scale);
        if (ink.isEmpty() || body.isEmpty() || qAbs(center.x()-bodyCenter.x())>tolerance
            || qAbs(center.y()-bodyCenter.y())>tolerance || qAbs(bodyCenter.x()-25*scale)>tolerance
            || qAbs(bodyCenter.y()-20*scale)>tolerance) {
            fprintf(stderr,"Category %s ink center=%.2f,%.2f expected=%.2f,%.2f\n",qPrintable(category),
                center.x(),center.y(),25*scale,20*scale);return false;
        }
    }
    return true;
}
