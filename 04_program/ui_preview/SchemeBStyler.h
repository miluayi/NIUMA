#pragma once

#include <QObject>
#include <QAbstractNativeEventFilter>

class QApplication;
class MainWindow;
class QEvent;
class QStackedWidget;

// Visual adjustments for the source-level V2 preview. The production V2
// sources remain untouched; newly created home rows are styled after refresh.
class SchemeBStyler final : public QObject, public QAbstractNativeEventFilter
{
public:
    SchemeBStyler(QApplication &app, MainWindow &window);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    bool nativeEventFilter(const QByteArray &eventType, void *message, long *result) override;

private:
    void scheduleRefresh();
    void refresh();
    void updateContentCorner(QStackedWidget *stack);

    QApplication &app_;
    MainWindow &window_;
    bool refreshPending_ = false;
    bool refreshing_ = false;
};
