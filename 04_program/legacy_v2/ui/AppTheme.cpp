#include "AppTheme.h"

#include <QApplication>
#include <QCursor>
#include <QLayout>
#include <QAbstractButton>
#include <QAbstractScrollArea>
#include <QAbstractItemView>
#include <QCalendarWidget>
#include <QCheckBox>
#include <QComboBox>
#include <QDate>
#include <QDateEdit>
#include <QEvent>
#include <QFile>
#include <QFrame>
#include <QFont>
#include <QFontDatabase>
#include <QHash>
#include <QHBoxLayout>
#include <QGuiApplication>
#include <QGridLayout>
#include <QListWidget>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPointer>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QStackedWidget>
#include <QStatusBar>
#include <QStyledItemDelegate>
#include <QLabel>
#include <QPen>
#include <QPropertyAnimation>
#include <QPixmap>
#include <QRegion>
#include <QRegularExpression>
#include <QTimer>
#include <QScrollBar>
#include <QEasingCurve>
#include <QWheelEvent>

#include "application/settings/SettingsUseCase.h"
#include "widgets/RoundedCornerFilter.h"

namespace {

class AppSidebarDelegate final : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        Q_UNUSED(option)
        if (index.row() == 7) return QSize(0, 0);
        return QSize(184, index.row() == 3 || index.row() == 5 ? 60 : 48);
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        QStyledItemDelegate::paint(painter, option, index);
        if (index.row() == 3 || index.row() == 5) {
            painter->save();
            painter->setPen(QPen(QColor("#DCE3EF"), 1));
            painter->drawLine(option.rect.left() + 16, option.rect.bottom() - 6,
                              option.rect.right() - 16, option.rect.bottom() - 6);
            painter->restore();
        }
    }
};

QString readResource(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }
    return QString::fromUtf8(file.readAll());
}

QFont::Weight selectedWeight(int index)
{
    if (index == static_cast<int>(UiFontWeight::Medium)) return QFont::Medium;
    if (index == static_cast<int>(UiFontWeight::Semibold)) return QFont::DemiBold;
    return QFont::Normal;
}

QString selectedFontStyle(QFont::Weight weight)
{
    if (weight == QFont::Medium) return QStringLiteral("Medium");
    if (weight == QFont::DemiBold) return QStringLiteral("Semibold");
    return QStringLiteral("Regular");
}

QPixmap appUiArrowPixmap()
{
    QPixmap arrow(14, 14);
    arrow.fill(Qt::transparent);
    QPainter painter(&arrow);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor("#667085"), 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawLine(QPointF(3, 5), QPointF(7, 9));
    painter.drawLine(QPointF(7, 9), QPointF(11, 5));
    return arrow;
}

void styleDateEditor(QDateEdit *edit);
void roundComboPopup(QWidget *popup);
void prepareComboPopup(QWidget *popup);

void styleComboBox(QComboBox *combo)
{
    if (!combo) return;
    if (!combo->styleSheet().contains(QStringLiteral("/*appUiComboArrow*/"))) {
        combo->setStyleSheet(combo->styleSheet() + QStringLiteral(
            " QComboBox::drop-down { border:none; width:30px; }"
            " QComboBox::down-arrow { image:none; width:0; height:0; }"
            " /*appUiComboArrow*/"));
    }
    QWidget *popup = combo->view()->window();
    prepareComboPopup(popup);
    auto *arrow = combo->findChild<QLabel *>(QStringLiteral("appUiComboArrow"));
    if (!arrow) {
        arrow = new QLabel(combo);
        arrow->setObjectName(QStringLiteral("appUiComboArrow"));
        arrow->setProperty("appUiNoLabelReset", true);
        arrow->setAttribute(Qt::WA_TransparentForMouseEvents);
        arrow->setStyleSheet(QStringLiteral("background:transparent; border:none; padding:0;"));
        arrow->setFixedSize(14, 14);
        arrow->setPixmap(appUiArrowPixmap());
        arrow->show();
    }
    arrow->move(combo->width() - arrow->width() - 9,
                (combo->height() - arrow->height()) / 2);
    arrow->raise();
}

void styleCheckBox(QCheckBox *box)
{
    if (!box || box->property("appUiCheckStyled").toBool()) return;
    box->setStyleSheet(box->styleSheet() + QStringLiteral(
        " QCheckBox::indicator { width:17px; height:17px; border:1px solid #BAC6D8;"
        " border-radius:4px; background:#FFFFFF; }"
        " QCheckBox::indicator:checked { border:1px solid #2463EB; background:#2463EB;"
        " image:url(:/icons/app_check.svg); }"));
    box->setProperty("appUiCheckStyled", true);
}

void roundComboPopup(QWidget *popup)
{
    if (!popup || popup->width() <= 0 || popup->height() <= 0) return;
#ifdef Q_OS_WIN
    NativePopupFrame::apply(popup);
    return;
#endif
    popup->setAttribute(Qt::WA_TranslucentBackground, false);
    popup->setAttribute(Qt::WA_StyledBackground, true);
    popup->setAutoFillBackground(true);
    popup->setProperty("appUiTransparentCorners", true);
    QPainterPath shape;
    shape.addRoundedRect(QRectF(popup->rect()).adjusted(0.5, 0.5, -0.5, -0.5), 8, 8);
    popup->setMask(QRegion(shape.toFillPolygon().toPolygon()));
}

class ComboPopupRounder final : public QObject
{
public:
    explicit ComboPopupRounder(QWidget *popup) : QObject(popup), popup_(popup) {}

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (watched == popup_ && (event->type() == QEvent::Show || event->type() == QEvent::Resize)) {
            roundComboPopup(popup_);
            QTimer::singleShot(0, this, [this]() { roundComboPopup(popup_); });
        }
        return QObject::eventFilter(watched, event);
    }

private:
    QPointer<QWidget> popup_;
};

void prepareComboPopup(QWidget *popup)
{
    if (!popup) return;
#ifdef Q_OS_WIN
    WindowsChrome::prepare(popup);
#else
    popup->setAttribute(Qt::WA_TranslucentBackground, false);
#endif
    popup->setAttribute(Qt::WA_StyledBackground, true);
    popup->setAutoFillBackground(true);
    popup->setObjectName(QStringLiteral("appUiComboPopup"));
    popup->setStyleSheet(QStringLiteral(
        "QFrame#appUiComboPopup { background:#FFFFFF; border:1px solid #DCE3EF; border-radius:8px; }"));
    if (!popup->property("appUiPopupRounderInstalled").toBool()) {
        popup->installEventFilter(new ComboPopupRounder(popup));
        popup->setProperty("appUiPopupRounderInstalled", true);
    }
    roundComboPopup(popup);
}

QString removeBoldStyles(QString style)
{
    static const QRegularExpression weightRule(
        QStringLiteral("font-weight\\s*:\\s*(?:bold|[6-9]00)\\s*;?"),
        QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression shorthandRule(
        QStringLiteral("font\\s*:\\s*bold\\s+"),
        QRegularExpression::CaseInsensitiveOption);
    style.remove(weightRule);
    style.replace(shorthandRule, QStringLiteral("font: normal "));
    return style;
}

class SmoothWheelFilter final : public QObject
{
public:
    explicit SmoothWheelFilter(QObject *parent) : QObject(parent) {}
protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (event->type() != QEvent::Wheel) return false;
        auto *wheel = static_cast<QWheelEvent *>(event);
        auto *widget = qobject_cast<QWidget *>(watched);
        if (!widget) return false;
        const bool horizontal = (wheel->modifiers() & Qt::ShiftModifier)
            || (wheel->angleDelta().x() && !wheel->angleDelta().y());
        const int pixel = horizontal && wheel->pixelDelta().x()
            ? wheel->pixelDelta().x() : wheel->pixelDelta().y();
        const int angle = horizontal && wheel->angleDelta().x()
            ? wheel->angleDelta().x() : wheel->angleDelta().y();
        const int direction = pixel ? pixel : angle;
        if (!direction) return false;
        QAbstractScrollArea *area = nullptr;
        QScrollBar *bar = nullptr;
        // At a nested list's limit, continue scrolling its enclosing page.
        for (QWidget *parent = widget; parent; parent = parent->parentWidget()) {
            if (auto *candidate = qobject_cast<QAbstractScrollArea *>(parent)) {
                auto *candidateBar = horizontal ? candidate->horizontalScrollBar() : candidate->verticalScrollBar();
                if (candidateBar && candidateBar->maximum() > candidateBar->minimum()
                    && (direction > 0 ? candidateBar->value() > candidateBar->minimum()
                                      : candidateBar->value() < candidateBar->maximum())) {
                    area = candidate; bar = candidateBar; break;
                }
            }
            if (parent->isWindow()) break;
        }
        if (!area || !bar) return false;
        auto animation = animations_.value(bar);
        if (pixel) {
            // Touchpads already supply continuous pixel movement and momentum.
            if (animation) animation->stop();
            bar->setValue(qBound(bar->minimum(),bar->value()-pixel,bar->maximum()));
            wheel->accept();
            return true;
        }
        if (!fractional_.contains(bar)) {
            fractional_.insert(bar,0);
            connect(bar,&QObject::destroyed,this,[this,bar]() { fractional_.remove(bar); animations_.remove(bar); });
        }
        const int viewportLength = horizontal ? area->viewport()->width() : area->viewport()->height();
        const qreal distance = qBound(36.0,viewportLength*0.14,112.0);
        fractional_[bar] += -angle/120.0*distance;
        const int pixels = qRound(fractional_[bar]);
        fractional_[bar] -= pixels;
        if (!pixels) { wheel->accept(); return true; }
        int target = bar->value()+pixels;
        if (animation && animation->state() == QAbstractAnimation::Running) {
            const int remaining = animation->endValue().toInt()-bar->value();
            if (remaining*pixels > 0) target += remaining;
        }
        target = qBound(bar->minimum(),target,bar->maximum());
        if (!animation) {
            animation = new QPropertyAnimation(bar,"value",this);
            animations_.insert(bar,animation);
        }
        animation->stop();
        animation->setDuration(180);
        animation->setEasingCurve(QEasingCurve::OutCubic);
        animation->setStartValue(bar->value());
        animation->setEndValue(target);
        animation->start();
        wheel->accept();
        return true;
    }
private:
    QHash<QScrollBar *, QPointer<QPropertyAnimation>> animations_;
    QHash<QScrollBar *, qreal> fractional_;
};

void styleControls(QWidget *root)
{
    if (!root) return;
    auto labels = root->findChildren<QLabel *>();
    if (auto *label = qobject_cast<QLabel *>(root)) labels.append(label);
    for (auto *label : labels) {
        if (label->text().isEmpty() || label->property("appUiNoTextSelection").toBool()) continue;
        bool decoration = false;
        for (QWidget *parent = label; parent; parent = parent->parentWidget()) {
            if (parent->objectName() == QStringLiteral("titlebar")
                || qobject_cast<QAbstractButton *>(parent)
                || qobject_cast<QComboBox *>(parent)
                || qobject_cast<QDateEdit *>(parent)
                || qobject_cast<QCalendarWidget *>(parent)) { decoration = true; break; }
        }
        if (!decoration) {
            if (!label->property("appUiCopyCursor").isValid())
                label->setProperty("appUiCopyCursor",QVariant::fromValue(label->cursor()));
            label->setTextInteractionFlags(label->textInteractionFlags()
                | Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
            label->setFocusPolicy(Qt::ClickFocus);
            label->setCursor(label->property("appUiCopyCursor").value<QCursor>());
            QPalette palette=label->palette();
            palette.setColor(QPalette::Highlight,QColor("#2463EB"));
            palette.setColor(QPalette::HighlightedText,Qt::white);
            label->setPalette(palette);
        }
    }
    auto views = root->findChildren<QAbstractItemView *>();
    if (auto *view = qobject_cast<QAbstractItemView *>(root)) views.append(view);
    for (auto *view : views) {
        if (view->objectName() == QStringLiteral("sidebar")) continue;
        if (view->verticalScrollMode() != QAbstractItemView::ScrollPerPixel)
            view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        if (view->horizontalScrollMode() != QAbstractItemView::ScrollPerPixel)
            view->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    }
    if (auto *combo = qobject_cast<QComboBox *>(root)) styleComboBox(combo);
    if (auto *checkbox = qobject_cast<QCheckBox *>(root)) styleCheckBox(checkbox);
    if (auto *dateEdit = qobject_cast<QDateEdit *>(root)) styleDateEditor(dateEdit);
    const auto combos = root->findChildren<QComboBox *>();
    for (QComboBox *combo : combos) styleComboBox(combo);
    const auto checkboxes = root->findChildren<QCheckBox *>();
    for (QCheckBox *checkbox : checkboxes) styleCheckBox(checkbox);
    const auto dateEdits = root->findChildren<QDateEdit *>();
    for (QDateEdit *edit : dateEdits) styleDateEditor(edit);
}

void normalizeLabel(QLabel *label)
{
    if (!label || label->property("appUiNoLabelReset").toBool()) return;
    static const QRegularExpression semanticText(
        QStringLiteral("\\bcolor\\s*:\\s*#(?:FA423E|40C977|F5B400)\\b"),
        QRegularExpression::CaseInsensitiveOption);
    const QString style = label->styleSheet();
    if (!style.contains(QStringLiteral("/*appUiText*/"))
        && !semanticText.match(style).hasMatch()) {
        label->setStyleSheet(style + QStringLiteral(
            "; color:#000000; background:transparent; border:none; padding-left:6px;"
            " padding-right:6px; /*appUiText*/"));
    }
    label->setAutoFillBackground(false);
}

void styleDateEditor(QDateEdit *edit)
{
    if (!edit) return;
    edit->setCalendarPopup(false);
    edit->setButtonSymbols(QAbstractSpinBox::NoButtons);
    edit->setMinimumWidth(156);
    if (!edit->property("appUiDateStyled").toBool()) {
        edit->setStyleSheet(QStringLiteral(
            "QDateEdit { background:#FFFFFF; border:1px solid #DCE3EF; border-radius:7px; "
            "color:#172033; padding:6px 30px 6px 28px; font-size:14px; }"
            "QDateEdit:focus { border-color:#2B72EF; }"
            "QDateEdit::up-button, QDateEdit::down-button { width:0; height:0; border:none; }"
            "QDateEdit::up-arrow, QDateEdit::down-arrow { image:none; width:0; height:0; }"));
        edit->setProperty("appUiDateStyled", true);
    }
    if (!edit->findChild<QLabel *>(QStringLiteral("appUiDateIcon"))) {
        QPixmap iconImage(16, 16);
        iconImage.fill(Qt::transparent);
        QPainter painter(&iconImage);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(QPen(QColor("#667085"), 1.3));
        painter.drawRoundedRect(QRectF(1.5, 3.5, 13, 11), 1.5, 1.5);
        painter.drawLine(QPointF(1.5, 7), QPointF(14.5, 7));
        painter.drawLine(QPointF(5, 1.5), QPointF(5, 5));
        painter.drawLine(QPointF(11, 1.5), QPointF(11, 5));
        auto *icon = new QLabel(edit);
        icon->setObjectName(QStringLiteral("appUiDateIcon"));
        icon->setProperty("appUiNoLabelReset", true);
        icon->setAttribute(Qt::WA_TransparentForMouseEvents);
        icon->setStyleSheet(QStringLiteral("background:transparent; border:none; padding:0;"));
        icon->setFixedSize(16, 16);
        icon->setPixmap(iconImage);
        icon->show();
    }
    if (auto *icon = edit->findChild<QLabel *>(QStringLiteral("appUiDateIcon"))) {
        icon->move(9, (edit->height() - icon->height()) / 2);
    }
    auto *arrow = edit->findChild<QLabel *>(QStringLiteral("appUiDateArrow"));
    if (!arrow) {
        arrow = new QLabel(edit);
        arrow->setObjectName(QStringLiteral("appUiDateArrow"));
        arrow->setProperty("appUiNoLabelReset", true);
        arrow->setAttribute(Qt::WA_TransparentForMouseEvents);
        arrow->setStyleSheet(QStringLiteral("background:transparent; border:none; padding:0;"));
        arrow->setFixedSize(14, 14);
        arrow->setPixmap(appUiArrowPixmap());
        arrow->show();
    }
    arrow->move(edit->width() - arrow->width() - 9,
                (edit->height() - arrow->height()) / 2);
}

QString scaleFontSizes(const QString &style, qreal scale)
{
    static const QRegularExpression sizeRule(
        QStringLiteral("font-size\\s*:\\s*(\\d+(?:\\.\\d+)?)(px|pt)"),
        QRegularExpression::CaseInsensitiveOption);
    QString result = style;
    auto matches = sizeRule.globalMatch(style);
    struct Replacement { int start; int length; QString text; };
    QList<Replacement> replacements;
    while (matches.hasNext()) {
        const auto match = matches.next();
        const qreal size = match.captured(1).toDouble();
        replacements.append({match.capturedStart(1), match.capturedLength(1),
                             QString::number(qMax(1, qRound(size * scale)))});
    }
    for (auto it = replacements.crbegin(); it != replacements.crend(); ++it) {
        result.replace(it->start, it->length, it->text);
    }
    return result;
}

QString normalizeStatusColors(QString style)
{
    style.replace(QStringLiteral("#DC2626"), QStringLiteral("#FA423E"), Qt::CaseInsensitive);
    style.replace(QStringLiteral("#EF4444"), QStringLiteral("#FA423E"), Qt::CaseInsensitive);
    style.replace(QStringLiteral("#16A34A"), QStringLiteral("#40C977"), Qt::CaseInsensitive);
    style.replace(QStringLiteral("#059669"), QStringLiteral("#40C977"), Qt::CaseInsensitive);
    style.replace(QStringLiteral("#F59E0B"), QStringLiteral("#F5B400"), Qt::CaseInsensitive);
    style.replace(QStringLiteral("#A16207"), QStringLiteral("#F5B400"), Qt::CaseInsensitive);
    style.replace(QStringLiteral("#D97706"), QStringLiteral("#F5B400"), Qt::CaseInsensitive);
    return style;
}

class AppDatePopup final : public QWidget
{
public:
    explicit AppDatePopup(QDateEdit *edit) : QWidget(edit, Qt::Popup), edit_(edit)
    {
        setObjectName(QStringLiteral("appUiDatePopup"));
        setFixedSize(450, 350);
        setStyleSheet(QStringLiteral(
            "QWidget#appUiDatePopup { background:#FFFFFF; border:1px solid #D9DFE9; border-radius:5px; }"
            "QWidget#appUiDateQuickBar { background:#FFFFFF; border-right:1px solid #E3E7EF; }"
            "QPushButton#appUiDateQuick { background:transparent; border:none; color:#475467; "
            "text-align:left; padding:7px 12px; font-size:14px; }"
            "QPushButton#appUiDateQuick:hover { color:#2164E5; background:#F2F6FF; }"
            "QPushButton#appUiDateNav { background:transparent; border:none; color:#475467; "
            "font-size:19px; padding:0; }"
            "QPushButton#appUiDateNav:hover { color:#2164E5; background:#F2F6FF; border-radius:4px; }"
            "QLabel#appUiDateHeading { color:#344054; font-size:15px; font-weight:700; }"
            "QLabel#appUiDateWeekday { color:#667085; font-size:12px; }"
            "QPushButton#appUiDateDay { background:transparent; border:none; border-radius:17px; "
            "color:#344054; font-size:13px; padding:0; }"
            "QPushButton#appUiDateDay:hover { background:#EEF3FB; }"
            "QPushButton#appUiDateDay[adjacent='true'] { color:#98A2B3; }"
            "QPushButton#appUiDateDay[selected='true'] { background:#2B72EF; color:#FFFFFF; font-weight:700; }"
            "QPushButton#appUiDateDay:disabled { color:#C8CDD7; background:transparent; }"));
        installEventFilter(new RoundedCornerFilter(12, this));

        auto *root = new QHBoxLayout(this);
        root->setContentsMargins(0, 0, 0, 0);
        root->setSpacing(0);
        auto *quickBar = new QWidget(this);
        quickBar->setObjectName(QStringLiteral("appUiDateQuickBar"));
        quickBar->setFixedWidth(110);
        auto *quickLayout = new QVBoxLayout(quickBar);
        quickLayout->setContentsMargins(0, 15, 0, 8);
        quickLayout->setSpacing(1);
        const QList<QPair<QString, int>> presets = {
            {QStringLiteral("今天"), 0}, {QStringLiteral("昨天"), -1}, {QStringLiteral("一周前"), -7}
        };
        for (const auto &preset : presets) {
            auto *button = new QPushButton(preset.first, quickBar);
            button->setObjectName(QStringLiteral("appUiDateQuick"));
            button->setCursor(Qt::PointingHandCursor);
            quickLayout->addWidget(button);
            connect(button, &QPushButton::clicked, this, [this, offset = preset.second]() {
                selectDate(QDate::currentDate().addDays(offset));
            });
        }
        quickLayout->addStretch();
        root->addWidget(quickBar);

        auto *calendar = new QWidget(this);
        auto *calendarLayout = new QVBoxLayout(calendar);
        calendarLayout->setContentsMargins(14, 8, 14, 12);
        calendarLayout->setSpacing(4);
        auto *navigation = new QHBoxLayout();
        navigation->setContentsMargins(0, 0, 0, 0);
        navigation->setSpacing(3);
        auto addNavigation = [this, navigation](const QString &text, int months) {
            auto *button = new QPushButton(text, this);
            button->setObjectName(QStringLiteral("appUiDateNav"));
            button->setFixedSize(26, 30);
            button->setCursor(Qt::PointingHandCursor);
            navigation->addWidget(button);
            connect(button, &QPushButton::clicked, this, [this, months]() {
                visibleMonth_ = visibleMonth_.addMonths(months);
                updateDays();
            });
        };
        addNavigation(QStringLiteral("«"), -12);
        addNavigation(QStringLiteral("‹"), -1);
        navigation->addStretch();
        heading_ = new QLabel(calendar);
        heading_->setObjectName(QStringLiteral("appUiDateHeading"));
        heading_->setAlignment(Qt::AlignCenter);
        navigation->addWidget(heading_);
        navigation->addStretch();
        addNavigation(QStringLiteral("›"), 1);
        addNavigation(QStringLiteral("»"), 12);
        calendarLayout->addLayout(navigation);

        auto *grid = new QGridLayout();
        grid->setContentsMargins(0, 0, 0, 0);
        grid->setHorizontalSpacing(4);
        grid->setVerticalSpacing(7);
        const QStringList weekdays = {QStringLiteral("周日"), QStringLiteral("周一"),
            QStringLiteral("周二"), QStringLiteral("周三"), QStringLiteral("周四"),
            QStringLiteral("周五"), QStringLiteral("周六")};
        for (int col = 0; col < weekdays.size(); ++col) {
            auto *label = new QLabel(weekdays.at(col), calendar);
            label->setObjectName(QStringLiteral("appUiDateWeekday"));
            label->setAlignment(Qt::AlignCenter);
            label->setFixedHeight(26);
            grid->addWidget(label, 0, col);
        }
        for (int week = 0; week < 6; ++week) {
            for (int col = 0; col < 7; ++col) {
                auto *day = new QPushButton(calendar);
                day->setObjectName(QStringLiteral("appUiDateDay"));
                day->setFixedSize(34, 34);
                day->setCursor(Qt::PointingHandCursor);
                grid->addWidget(day, week + 1, col, Qt::AlignCenter);
                days_.append(day);
                connect(day, &QPushButton::clicked, this, [this, day]() {
                    selectDate(day->property("date").toDate());
                });
            }
        }
        calendarLayout->addLayout(grid);
        calendarLayout->addStretch();
        root->addWidget(calendar, 1);
    }

    void openAt(QDateEdit *edit)
    {
        edit_ = edit;
        selectedDate_ = edit->date() == edit->minimumDate() && !edit->specialValueText().isEmpty()
            ? QDate::currentDate() : edit->date();
        visibleMonth_ = QDate(selectedDate_.year(), selectedDate_.month(), 1);
        updateDays();
        QPoint position = edit->mapToGlobal(QPoint(0, edit->height() + 4));
        if (QScreen *screen = QGuiApplication::screenAt(position)) {
            const QRect available = screen->availableGeometry();
            if (position.x() + width() > available.right()) position.setX(available.right() - width());
            if (position.y() + height() > available.bottom()) {
                position.setY(edit->mapToGlobal(QPoint(0, 0)).y() - height() - 4);
            }
        }
        move(position);
        show();
        raise();
    }

private:
    void selectDate(const QDate &date)
    {
        if (!edit_ || !date.isValid() || date < edit_->minimumDate() || date > edit_->maximumDate()) return;
        edit_->setDate(date);
        hide();
    }

    void updateDays()
    {
        if (!edit_) return;
        heading_->setText(QStringLiteral("%1年%2月").arg(visibleMonth_.year()).arg(visibleMonth_.month()));
        const QDate first = visibleMonth_.addDays(-(visibleMonth_.dayOfWeek() % 7));
        for (int index = 0; index < days_.size(); ++index) {
            auto *day = days_.at(index);
            const QDate date = first.addDays(index);
            day->setText(QString::number(date.day()));
            day->setProperty("date", date);
            day->setProperty("adjacent", date.month() != visibleMonth_.month());
            day->setProperty("selected", date == selectedDate_);
            day->setEnabled(date >= edit_->minimumDate() && date <= edit_->maximumDate());
            day->style()->unpolish(day);
            day->style()->polish(day);
        }
    }

    QPointer<QDateEdit> edit_;
    QDate visibleMonth_;
    QDate selectedDate_;
    QLabel *heading_ = nullptr;
    QList<QPushButton *> days_;
};

void applyWidgetTypography(QWidget *widget, int pixelSize, QFont::Weight weight, int version)
{
    if (!widget) return;
    QFont baseFont;
    if (!widget->property("appUiBaseFont").isValid()) {
        baseFont = widget->font();
        widget->setProperty("appUiBaseFont", baseFont);
    } else {
        baseFont = widget->property("appUiBaseFont").value<QFont>();
    }
    QFont font = baseFont;
    font.setFamilies({QStringLiteral("Segoe UI"), QStringLiteral("Microsoft YaHei UI"), QStringLiteral("Microsoft YaHei")});
    const qreal scale = pixelSize / 14.0;
    if (font.pixelSize() > 0) font.setPixelSize(qMax(1, qRound(font.pixelSize() * scale)));
    else if (font.pointSizeF() > 0) font.setPointSizeF(qMax(1.0, font.pointSizeF() * scale));
    font.setWeight(weight);
    font.setStyleName(selectedFontStyle(weight));
    widget->setFont(font);

    const QString currentStyle = widget->styleSheet();
    const QString previouslyApplied = widget->property("appUiAppliedFontStyle").toString();
    if (!widget->property("appUiBaseFontStyle").isValid() || currentStyle != previouslyApplied) {
        widget->setProperty("appUiBaseFontStyle", currentStyle);
    }
    QString scaledStyle = scaleFontSizes(widget->property("appUiBaseFontStyle").toString(), scale);
    scaledStyle = normalizeStatusColors(removeBoldStyles(scaledStyle));
    if (scaledStyle != currentStyle) widget->setStyleSheet(scaledStyle);
    widget->setProperty("appUiAppliedFontStyle", scaledStyle);
    widget->setProperty("appUiTypographyVersion", version);
    if (auto *label = qobject_cast<QLabel *>(widget)) normalizeLabel(label);
    widget->setProperty("appUiAppliedFontStyle", widget->styleSheet());
}

class AppThemeWatcher final : public QObject
{
public:
    AppThemeWatcher(QApplication &app, QWidget &root)
        : QObject(&root), app_(app), root_(root) {}

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        auto *widget = qobject_cast<QWidget *>(watched);
        // Resize is a hot path: never scan descendants or repolish styles here.
        if (!widget) return QObject::eventFilter(watched, event);
        if (event->type()==QEvent::CursorChange) {
            if (auto *label=qobject_cast<QLabel *>(widget)) {
                const QVariant saved=label->property("appUiCopyCursor");
                if (saved.isValid() && label->cursor().shape()!=saved.value<QCursor>().shape())
                    label->setCursor(saved.value<QCursor>());
            }
            return QObject::eventFilter(watched,event);
        }
        if (event->type() == QEvent::Resize) {
            if (qobject_cast<QComboBox *>(widget)) {
                if (auto *arrow = widget->findChild<QLabel *>(QStringLiteral("appUiComboArrow"), Qt::FindDirectChildrenOnly))
                    arrow->move(widget->width()-arrow->width()-9, (widget->height()-arrow->height())/2);
            }
            if (qobject_cast<QDateEdit *>(widget)) {
                if (auto *icon = widget->findChild<QLabel *>(QStringLiteral("appUiDateIcon"), Qt::FindDirectChildrenOnly))
                    icon->move(9, (widget->height()-icon->height())/2);
                if (auto *arrow = widget->findChild<QLabel *>(QStringLiteral("appUiDateArrow"), Qt::FindDirectChildrenOnly))
                    arrow->move(widget->width()-arrow->width()-9, (widget->height()-arrow->height())/2);
            }
            return QObject::eventFilter(watched, event);
        }
        if (event->type() != QEvent::Show && event->type() != QEvent::MouseButtonPress)
            return QObject::eventFilter(watched, event);
        if (event->type() == QEvent::Show) {
            styleControls(widget);
            if (auto *combo = qobject_cast<QComboBox *>(widget)) {
                roundComboPopup(combo->view()->window());
            }
        }
        QDateEdit *dateEdit = nullptr;
        for (QWidget *ancestor = widget; ancestor; ancestor = ancestor->parentWidget()) {
            if (ancestor->objectName() == QStringLiteral("appUiDatePopup")) break;
            if ((dateEdit = qobject_cast<QDateEdit *>(ancestor))) break;
        }
        if (dateEdit && (event->type() == QEvent::Show
                         || event->type() == QEvent::MouseButtonPress)) {
            styleDateEditor(dateEdit);
        }
        if (dateEdit && event->type() == QEvent::MouseButtonPress
            && static_cast<QMouseEvent *>(event)->button() == Qt::LeftButton) {
            if (dateEdit->calendarPopup()) dateEdit->setCalendarPopup(false);
            auto *popupWidget = dateEdit->findChild<QWidget *>(QStringLiteral("appUiDatePopup"));
            auto *popup = popupWidget ? static_cast<AppDatePopup *>(popupWidget)
                                      : new AppDatePopup(dateEdit);
            popup->openAt(dateEdit);
            return true;
        }
        if (!widget || event->type() != QEvent::Show
            || widget->property("appUiTypographyVersion").toInt()
                == app_.property("appUiTypographyVersion").toInt()) {
            return QObject::eventFilter(watched, event);
        }
        AppTheme::prepareWindow(*widget);
        return QObject::eventFilter(watched, event);
    }

private:
    QApplication &app_;
    QWidget &root_;
};

} // namespace

void AppTheme::prepareWindow(QWidget &window)
{
    styleControls(&window);
    const int version=qApp->property("appUiTypographyVersion").toInt();
    auto widgets=window.findChildren<QWidget *>();
    widgets.prepend(&window);
    if (version>0) {
        for(auto *widget:widgets) {
            if(widget->property("appUiTypographyVersion").toInt()!=version)
                applyWidgetTypography(widget,qApp->property("appUiFontPixelSize").toInt(),
                    selectedWeight(qApp->property("appUiFontWeight").toInt()),version);
        }
    }
    // Resolve font metrics and nested layouts before a dialog can paint.
    for(auto *widget:widgets) {
        widget->ensurePolished();
        if(widget->layout()) widget->layout()->activate();
        if(auto *view=qobject_cast<QAbstractItemView *>(widget)) view->doItemsLayout();
    }
    for(auto it=widgets.crbegin();it!=widgets.crend();++it) {
        (*it)->ensurePolished();
        if((*it)->layout()) (*it)->layout()->activate();
    }
}

QString AppTheme::preferredFontFamily(const QStringList &availableFamilies)
{
    return availableFamilies.contains(QStringLiteral("Segoe UI"))
        ? QStringLiteral("Segoe UI") : QStringLiteral("Microsoft YaHei");
}

void AppTheme::apply(QApplication &app, QWidget &window)
{
    const QString fontFamily = preferredFontFamily(QFontDatabase().families());
    app.setProperty("appUiFontFallback", fontFamily != QStringLiteral("Segoe UI"));
    QFont::insertSubstitutions(QStringLiteral("Segoe UI"),
        {QStringLiteral("Microsoft YaHei UI"), QStringLiteral("Microsoft YaHei")});
    QFont baseFont(fontFamily);
    baseFont.setPixelSize(14);
    baseFont.setStyleName(QStringLiteral("Regular"));
    app.setFont(baseFont);

    const QString style = readResource(QStringLiteral(":/styles/neutral_modern.qss"))
        + QLatin1Char('\n') + readResource(QStringLiteral(":/styles/app.qss"));
    app.setProperty("appUiBaseApplicationStyle", style);
    app.setStyleSheet(style);
    if (!window.property("appUiThemeWatcherInstalled").toBool()) {
        auto *watcher = new AppThemeWatcher(app, window);
        app.installEventFilter(watcher);
        app.installEventFilter(new SmoothWheelFilter(watcher));
        window.setProperty("appUiThemeWatcherInstalled", true);
    }

    if (auto *titleBar = window.findChild<QWidget *>(QStringLiteral("titlebar"))) {
        titleBar->setFixedHeight(70);
    }
    if (auto *sidebar = window.findChild<QListWidget *>(QStringLiteral("sidebar"))) {
        sidebar->setFixedWidth(184);
        sidebar->setItemDelegate(new AppSidebarDelegate(sidebar));
    }
    if (auto *workspace = window.findChild<QStackedWidget *>()) {
        workspace->setObjectName(QStringLiteral("appUiWorkspace"));
        if (auto *layout = qobject_cast<QHBoxLayout *>(workspace->parentWidget()->layout())) {
            layout->setContentsMargins(0, 0, 8, 0);
        }
    }
    if (auto *statusBar = window.findChild<QStatusBar *>()) {
        statusBar->setFixedHeight(36);
    }
    styleControls(&window);
}

void AppTheme::applyTypography(QApplication &app, QWidget &window, int pixelSize, int weight)
{
    const int version = app.property("appUiTypographyVersion").toInt() + 1;
    const qreal scale = pixelSize / 14.0;
    app.setProperty("appUiFontPixelSize", pixelSize);
    app.setProperty("appUiFontWeight", weight);
    app.setProperty("appUiTypographyVersion", version);
    QFont appFont;
    appFont.setFamilies({QStringLiteral("Segoe UI"), QStringLiteral("Microsoft YaHei UI"), QStringLiteral("Microsoft YaHei")});
    appFont.setPixelSize(pixelSize);
    appFont.setWeight(selectedWeight(weight));
    appFont.setStyleName(selectedFontStyle(selectedWeight(weight)));
    app.setFont(appFont);

    const QString baseStyle = app.property("appUiBaseApplicationStyle").toString();
    QString scaledApplicationStyle = scaleFontSizes(baseStyle, scale);
    app.setStyleSheet(normalizeStatusColors(removeBoldStyles(scaledApplicationStyle)));
    applyWidgetTypography(&window, pixelSize, selectedWeight(weight), version);
    for (QWidget *widget : window.findChildren<QWidget *>()) {
        applyWidgetTypography(widget, pixelSize, selectedWeight(weight), version);
    }
    styleControls(&window);
    if (auto *inProgressCard = window.findChild<QFrame *>(QStringLiteral("InProgressCard"))) {
        inProgressCard->setStyleSheet(QStringLiteral(
            "QFrame#InProgressCard { background:#FFFFFF; border:1px solid #DCE3EF; border-radius:12px; }"));
    }
}
