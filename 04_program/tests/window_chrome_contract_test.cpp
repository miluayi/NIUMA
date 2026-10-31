#include <QApplication>
#include <QDialog>
#include <QDir>
#include <QImage>
#include <QLabel>
#include <QPushButton>
#include <QStyleFactory>
#include <QStyle>
#include <QVBoxLayout>
#include <QWindow>
#include <cstdio>
#include "widgets/RoundedCornerFilter.h"
#include "widgets/TitleBar.h"

static bool smoothCorners(QWidget &window,const QString &path) {
    const QImage image=window.grab().toImage().convertToFormat(QImage::Format_ARGB32);
    if (!image.save(path)) return false;
    for (QPoint corner : {QPoint(0,0),QPoint(image.width()-1,0),
                         QPoint(0,image.height()-1),QPoint(image.width()-1,image.height()-1)}) {
        if (qAlpha(image.pixel(corner))!=0) return false;
        int partial=0;
        const int extent=qRound(16*window.devicePixelRatioF());
        for (int y=0;y<extent;++y) for (int x=0;x<extent;++x) {
            const int px=corner.x() ? image.width()-1-x : x;
            const int py=corner.y() ? image.height()-1-y : y;
            const int alpha=qAlpha(image.pixel(px,py));
            if (alpha>0 && alpha<255) ++partial;
        }
        if (partial<6) return false;
    }
    return qAlpha(image.pixel(image.width()/2,image.height()/2))==255;
}

int main(int argc,char **argv) {
    QApplication app(argc,argv);
    app.setProperty("niumaLegacyWindowChrome",true);
    const QString evidence=qEnvironmentVariable("NIUMA_SCREENSHOT_DIR");
    if (evidence.isEmpty() || !QDir().mkpath(evidence)) return 1;
    QDialog window(nullptr,Qt::Dialog|Qt::FramelessWindowHint);
    window.setObjectName("legacyChromeFixture");
    window.setStyleSheet("QDialog#legacyChromeFixture { background:#F5F6FA; }");
    window.installEventFilter(new RoundedCornerFilter(12,&window));
    auto *layout=new QVBoxLayout(&window);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);
    auto *title=new TitleBar(&window);
    title->setFixedHeight(70);
    layout->addWidget(title);
    // An opaque child covering the bottom corners must also be clipped smoothly.
    auto *content=new QLabel(QStringLiteral("Windows 7 兼容圆角 / NIUMA 0123456789"),&window);
    content->setStyleSheet("background:white;");
    layout->addWidget(content,1);
    window.resize(940,580);
    window.show();
    app.processEvents();
    if (!window.testAttribute(Qt::WA_TranslucentBackground) || !window.mask().isEmpty()
        || !window.property("appUiLegacyCorners").toBool()
        || window.property("appUiNativeCorners").toBool()) return 2;
#ifdef Q_OS_WIN
    const HWND hwnd=reinterpret_cast<HWND>(window.winId());
    if (GetWindowLongPtrW(hwnd,GWL_STYLE)&WS_CAPTION) return 3;
    if (!window.windowHandle()->frameMargins().isNull()) return 17;
    if (SendMessageW(hwnd,WM_NCACTIVATE,FALSE,0)!=TRUE
        || SendMessageW(hwnd,WM_NCACTIVATE,TRUE,0)!=TRUE) return 4;
#endif
    for (int width : {940,1100,1300}) {
        window.resize(width,580);
        app.processEvents();
#ifdef Q_OS_WIN
        RECT client,bounds;
        GetClientRect(hwnd,&client);
        GetWindowRect(hwnd,&bounds);
        if (client.right!=bounds.right-bounds.left || client.bottom!=bounds.bottom-bounds.top
            || client.right!=qRound(window.width()*window.devicePixelRatioF())
            || client.bottom!=qRound(window.height()*window.devicePixelRatioF())) return 18;
#endif
        if (!smoothCorners(window,QDir(evidence).filePath(QString("legacy-%1.png").arg(width)))) return 5;
        const QImage surface=window.grab().toImage();
        const qreal scale=window.devicePixelRatioF();
        if (surface.pixelColor(qRound(200*scale),qRound(60*scale))!=QColor("#F5F6FA")) return 16;
    }
    const auto edges=window.findChildren<QWidget *>("legacyFrameEdge",Qt::FindDirectChildrenOnly);
    if (edges.size()!=8) return 6;
    for (const auto *edge : edges) {
        if (!edge->testAttribute(Qt::WA_TransparentForMouseEvents) || edge->internalWinId()
            || (edge->width()>16 && edge->height()>16)) return 7;
    }
    int minimizeClicks=0,maximizeClicks=0,closeClicks=0;
    QObject::connect(title,&TitleBar::minimizeClicked,[&] { ++minimizeClicks; });
    QObject::connect(title,&TitleBar::maximizeClicked,[&] { ++maximizeClicks; });
    QObject::connect(title,&TitleBar::closeClicked,[&] { ++closeClicks; });
    for (const QString &name : {QString("btnMinimize"),QString("btnMaximize"),QString("btnClose")}) {
        auto *button=title->findChild<QPushButton *>(name);
        if (!button) return 8;
        QImage reference;
        for (const QString &styleName : {QString("Windows"),QString("Fusion")}) {
            QStyle *style=QStyleFactory::create(styleName);
            if (!style) return 9;
            style->setParent(button);
            button->setStyle(style);
            button->setDown(true);
            const QImage pressed=button->grab().toImage().convertToFormat(QImage::Format_ARGB32);
            if (!pressed.save(QDir(evidence).filePath(name+"-pressed-"+styleName+".png"))) return 10;
            const QColor expected=name=="btnClose" ? QColor("#D8DEE8") : QColor("#E1E7F0");
            const int scale=qRound(button->devicePixelRatioF());
            if (pressed.pixelColor(6*scale,6*scale)!=expected) return 11;
            if (!reference.isNull() && pressed!=reference) return 12;
            reference=pressed;
            button->setDown(false);
        }
        button->click();
    }
    if (minimizeClicks!=1 || maximizeClicks!=1 || closeClicks!=1) return 13;
    window.showMaximized();
    app.processEvents();
    const QImage maximized=window.grab().toImage();
    if (qAlpha(maximized.pixel(0,0))==0) return 14;
    window.showNormal();
    app.processEvents();
    if (!smoothCorners(window,QDir(evidence).filePath("legacy-restored.png"))) return 15;
    fprintf(stdout,"Legacy alpha corners, resize/restore, input-transparent edges and theme-independent pressed controls passed\n");
    return 0;
}
