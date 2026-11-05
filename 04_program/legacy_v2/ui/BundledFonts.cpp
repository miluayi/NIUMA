#include "BundledFonts.h"
#include <QCoreApplication>
#include <QFontDatabase>
#include <QResource>
#include <QVariant>

// Keep the static resource initializer in the global namespace so linking the
// font library also pulls its resource object into every consuming executable.
static void initializeFontResources()
{
    Q_INIT_RESOURCE(embedded_fonts);
}

bool BundledFonts::load(QString *error)
{
    if (error) error->clear();
    if (qApp->property("niumaBundledFontIds").isValid()) return true;
    initializeFontResources();
    QVariantList ids;
    for (const QString &file : {QStringLiteral("segoeui.ttf"),QStringLiteral("seguisb.ttf"),
                               QStringLiteral("segoeuib.ttf"),QStringLiteral("msyh.ttc"),
                               QStringLiteral("msyhbd.ttc"),QStringLiteral("seguisym.ttf")}) {
        const int id=QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/")+file);
        if (id<0) {
            for (const QVariant &added : ids) QFontDatabase::removeApplicationFont(added.toInt());
            if (error) *error=QStringLiteral("内置字体加载失败：%1").arg(file);
            return false;
        }
        ids.append(id);
    }
    qApp->setProperty("niumaBundledFontIds",ids);
    return true;
}
