#pragma once

#include <QStringList>

class QApplication;
class QWidget;

class AppTheme final
{
public:
    static QString preferredFontFamily(const QStringList &availableFamilies);
    static void apply(QApplication &app, QWidget &window);
    static void applyTypography(QApplication &app, QWidget &window, int pixelSize, int weight);
};
