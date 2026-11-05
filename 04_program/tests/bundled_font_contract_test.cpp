#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFontDatabase>
#include <QFontInfo>
#include <QGlyphRun>
#include <QImage>
#include <QPainter>
#include <QRawFont>
#include <QTextLayout>
#include <QVariant>
#include <QtEndian>
#include <cstring>
#include <cstdio>
#include "BundledFonts.h"

static QByteArray fontTable(const QByteArray &data,int face,const char *tag)
{
    if (data.size()<12 || face<0) return {};
    const auto *bytes=reinterpret_cast<const uchar *>(data.constData());
    if (data.startsWith("ttcf") && (face>=int(qFromBigEndian<quint32>(bytes+8)) || data.size()<16+face*4)) return {};
    const quint32 start=data.startsWith("ttcf") ? qFromBigEndian<quint32>(bytes+12+face*4) : 0;
    if (start>quint32(data.size()) || quint32(data.size())-start<12) return {};
    const int count=qFromBigEndian<quint16>(bytes+start+4);
    for (int index=0;index<count;++index) {
        const quint32 record=start+12+index*16;
        if (record+16>quint32(data.size())) return {};
        if (std::memcmp(bytes+record,tag,4)!=0) continue;
        const quint32 offset=qFromBigEndian<quint32>(bytes+record+8);
        const quint32 size=qFromBigEndian<quint32>(bytes+record+12);
        if (offset>quint32(data.size()) || size>quint32(data.size())-offset) return {};
        return data.mid(offset,size);
    }
    return {};
}

int main(int argc,char **argv)
{
    const QString root=qEnvironmentVariable("NIUMA_TEST_ROOT");
    if (root.isEmpty() || !QDir().mkpath(root)) return 1;
    bool isolated=false;
    for (int index=1;index<argc;++index)
        if (QByteArray(argv[index])=="--isolated") isolated=true;
    if (isolated) {
        const QString empty=QDir(root).filePath("empty-font-directory");
        if (!QDir().mkpath(empty) || !QDir(empty).entryList(QDir::Files).isEmpty()) return 2;
        qputenv("QT_QPA_PLATFORM","offscreen");
        qputenv("QT_QPA_FONTDIR",empty.toUtf8());
    }
    QApplication app(argc,argv);
    if (isolated && (QFontDatabase().families().contains("Segoe UI")
        || QFontDatabase().families().contains("Microsoft YaHei"))) return 3;
    QString error;
    if (!BundledFonts::load(&error)) {
        fprintf(stderr,"Cannot load bundled fonts: %s\n",qPrintable(error));
        return 4;
    }
    const QVariantList ids=app.property("niumaBundledFontIds").toList();
    if (ids.size()!=6 || !BundledFonts::load(&error)
        || ids!=app.property("niumaBundledFontIds").toList()) return 5;
    for (const QVariant &id : ids)
        if (QFontDatabase::applicationFontFamilies(id.toInt()).isEmpty()) return 6;
    for (const QString &name : {QString("Segoe UI"),QString("Microsoft YaHei UI"),
                                QString("Microsoft YaHei"),QString("Segoe UI Symbol")})
        if (!QFontDatabase().families().contains(name)) {
            fprintf(stderr,"Missing registered family: %s; available: %s\n",qPrintable(name),qPrintable(QFontDatabase().families().join(",")));
            return 7;
        }
    const QStringList files={"segoeui.ttf","seguisb.ttf","segoeuib.ttf","msyh.ttc","msyhbd.ttc","seguisym.ttf"};
    const QStringList families={"Segoe UI","Segoe UI","Segoe UI","Microsoft YaHei UI","Microsoft YaHei UI","Segoe UI Symbol"};
    const QStringList styles={"Regular","Semibold","Bold","Regular","Bold","Regular"};
    const QList<QFont::Weight> weights={QFont::Normal,QFont::DemiBold,QFont::Bold,QFont::Normal,QFont::Bold,QFont::Normal};
    for (int index=0;index<files.size();++index) {
        QFile file(":/fonts/"+files[index]);
        if (!file.open(QIODevice::ReadOnly)) return 8;
        const QByteArray data=file.readAll();
        QFont selected(families[index]);
        selected.setPixelSize(14);
        selected.setStyleName(styles[index]);
        selected.setWeight(weights[index]);
        const QRawFont resolved=QRawFont::fromFont(selected);
        if (!resolved.isValid()) return 9;
        // These tables identify the actual supplied font version and glyph map,
        // so a matching family name alone cannot hide an older system font.
        for (const char *table : {"head","OS/2","cmap"}) {
            const QByteArray expected=fontTable(data,data.startsWith("ttcf") ? 1 : 0,table);
            if (expected.isEmpty() || resolved.fontTable(table)!=expected) {
                fprintf(stderr,"Resolved font differs from bundled %s (%s)\n",qPrintable(files[index]),table);
                return 10;
            }
        }
    }
    QFont mixedFont;
    mixedFont.setFamilies({"Segoe UI","Microsoft YaHei UI","Microsoft YaHei","Segoe UI Symbol"});
    mixedFont.setPixelSize(14);
    QTextLayout text(QStringLiteral("NIUMA 0123456789 中文任务 ，。￥+-() ✓→"),mixedFont);
    text.beginLayout(); text.createLine().setLineWidth(600); text.endLayout();
    bool latin=false,chinese=false;
    QFile latinResource(":/fonts/segoeui.ttf");
    if (!latinResource.open(QIODevice::ReadOnly)) return 8;
    const QByteArray latinMap=fontTable(latinResource.readAll(),0,"cmap");
    for (const QGlyphRun &run : text.glyphRuns()) {
        fprintf(stdout,"Mixed glyph family: %s cmap=%d\n",qPrintable(run.rawFont().familyName()),run.rawFont().fontTable("cmap").size());
        if (run.glyphIndexes().contains(0)) return 11;
        latin=latin || run.rawFont().fontTable("cmap")==latinMap;
        chinese=chinese || run.rawFont().familyName().startsWith("Microsoft YaHei");
    }
    QImage image(620,50,QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);
    QPainter painter(&image); text.draw(&painter,QPointF(8,8)); painter.end();
    const bool saved=image.save(QDir(root).filePath("bundled-fonts.png"));
    if (!latin || !chinese || !saved) {
        fprintf(stderr,"Mixed render incomplete: latin=%d chinese=%d saved=%d\n",latin,chinese,saved);
        return 12;
    }
    fprintf(stdout,"Bundled fonts load once; font tables and mixed-language rendering match%s\n",
            isolated ? " without system fonts" : " on the native Windows backend");
    return 0;
}
