#include <QApplication>
#include <QDialog>
#include <QKeyEvent>
#include <QPushButton>
#include <QTimer>

#include "widgets/StyledMessageBox.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    bool inspected = false;
    QTimer::singleShot(0, [&inspected]() {
        auto *dialog = qobject_cast<StyledMessageBox *>(QApplication::activeModalWidget());
        if (!dialog) return;
        const auto buttons = dialog->findChildren<QPushButton *>();
        for (QPushButton *button : buttons) {
            if (button->text() == QStringLiteral("确认")) {
                inspected = button->isDefault()
                    && button->focusPolicy() == Qt::NoFocus
                    && dialog->minimumWidth() == 420 && dialog->maximumWidth() == 520
                    && dialog->windowFlags().testFlag(Qt::FramelessWindowHint) && dialog->isModal()
                    && dialog->mask().isEmpty() && dialog->property("nativePopupHandle").toULongLong();
                button->click();
                return;
            }
        }
    });
    if (StyledMessageBox::question(nullptr, QStringLiteral("确认删除"), QStringLiteral("隔离测试"))
            != StyledMessageBox::Yes) return 1;
    if (!inspected) return 2;

    bool cancelled = false;
    QTimer::singleShot(0, [&cancelled]() {
        auto *dialog = qobject_cast<StyledMessageBox *>(QApplication::activeModalWidget());
        if (!dialog) return;
        for (QPushButton *button : dialog->findChildren<QPushButton *>()) {
            if (button->text() == QStringLiteral("取消")) {
                cancelled = true;
                button->click();
                return;
            }
        }
    });
    if (StyledMessageBox::question(nullptr, QStringLiteral("确认删除"), QStringLiteral("隔离测试"))
            != StyledMessageBox::No || !cancelled) return 2;

    bool escaped = false;
    QTimer::singleShot(0, [&escaped]() {
        auto *dialog = qobject_cast<StyledMessageBox *>(QApplication::activeModalWidget());
        if (!dialog) return;
        escaped = true;
        QKeyEvent escape(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
        QApplication::sendEvent(dialog, &escape);
    });
    if (StyledMessageBox::question(nullptr, QStringLiteral("确认删除"), QStringLiteral("隔离测试"))
            != StyledMessageBox::No || !escaped) return 3;

    bool informationClosed = false;
    QTimer::singleShot(0, [&informationClosed]() {
        auto *dialog = qobject_cast<StyledMessageBox *>(QApplication::activeModalWidget());
        if (!dialog) return;
        for (QPushButton *button : dialog->findChildren<QPushButton *>()) {
            if (button->text() == QStringLiteral("我知道了")) {
                informationClosed = true;
                button->click();
                return;
            }
        }
    });
    StyledMessageBox::information(nullptr, QStringLiteral("提示"), QStringLiteral("隔离测试"));
    if (!informationClosed) return 4;

    bool warningClosed = false;
    QTimer::singleShot(0, [&warningClosed]() {
        auto *dialog = qobject_cast<StyledMessageBox *>(QApplication::activeModalWidget());
        if (!dialog) return;
        for (QPushButton *button : dialog->findChildren<QPushButton *>()) {
            if (button->text() == QStringLiteral("我知道了")) {
                warningClosed = true;
                button->click();
                return;
            }
        }
    });
    StyledMessageBox::warning(nullptr, QStringLiteral("警告"), QStringLiteral("隔离测试"));
    return warningClosed ? 0 : 5;
}
