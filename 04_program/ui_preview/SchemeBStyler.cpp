#include "SchemeBStyler.h"

#include <QApplication>
#include <QAbstractItemView>
#include <QAbstractScrollArea>
#include <QChildEvent>
#include <QDate>
#include <QDateEdit>
#include <QDir>
#include <QEasingCurve>
#include <QCalendarWidget>
#include <QCheckBox>
#include <QComboBox>
#include <QCompleter>
#include <QGridLayout>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QPointer>
#include <QScreen>
#include <QEvent>
#include <QFile>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QIcon>
#include <QListWidget>
#include <QLineEdit>
#include <QScrollArea>
#include <QScrollBar>
#include <QStatusBar>
#include <memory>
#include <cmath>
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QRegion>
#include <QRegularExpression>
#include <QResizeEvent>
#include <QSqlQuery>
#include <QStackedWidget>
#include <QStyle>
#include <QStyleOptionViewItem>
#include <QStyleOptionButton>
#include <QStyledItemDelegate>
#include <QTableView>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextEdit>
#include <QToolButton>
#include <QTextCharFormat>
#include <QTimer>
#include <QToolTip>
#include <QVBoxLayout>
#include <QWheelEvent>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "models/Task.h"
#include "models/TaskTableModel.h"
#include "service/FocusService.h"
#include "ui/HomeView.h"
#include "ui/MainWindow.h"
#include "ui/TaskListView.h"
#include "ui/TaskDetailView.h"
#include "ui/PeriodicTaskView.h"
#include "ui/ReferenceView.h"
#include "ui/SettingsView.h"
#include "ui/TaskCreateDialog.h"
#include "ui/ReferenceCreateDialog.h"
#include "ui/widgets/DragDropArea.h"
#include "ui/widgets/TaskItemDelegate.h"
#include "ui/widgets/PaginationWidget.h"
#include "ui/widgets/ToastWidget.h"

namespace {

constexpr auto kText = "#000000";
constexpr auto kRed = "#FA423E";
constexpr auto kYellow = "#F5B400";
constexpr auto kGreen = "#40C977";

QPixmap downArrowPixmap()
{
    QPixmap arrow(14, 14);
    arrow.fill(Qt::transparent);
    QPainter painter(&arrow);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor("#667085"), 1.5, Qt::SolidLine,
                        Qt::RoundCap, Qt::RoundJoin));
    painter.drawLine(QPointF(3, 5), QPointF(7, 9));
    painter.drawLine(QPointF(7, 9), QPointF(11, 5));
    return arrow;
}

QIcon windowControlIcon(const QString &name)
{
    QPixmap icon(40, 40);
    icon.fill(Qt::transparent);
    icon.setDevicePixelRatio(2);
    QPainter painter(&icon);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor("#1F2937"), 1.2, Qt::SolidLine,
                        Qt::SquareCap, Qt::MiterJoin));
    if (name == QStringLiteral("btnMinimize")) {
        painter.drawLine(QPointF(5, 10), QPointF(15, 10));
    } else if (name == QStringLiteral("btnMaximize")) {
        painter.drawRect(QRectF(5.5, 5.5, 9, 9));
    } else {
        painter.drawLine(QPointF(5.5, 5.5), QPointF(14.5, 14.5));
        painter.drawLine(QPointF(14.5, 5.5), QPointF(5.5, 14.5));
    }
    return QIcon(icon);
}

void normalizeStatusColors(QString &style)
{
    style.replace(QStringLiteral("#DC2626"), QLatin1String(kRed), Qt::CaseInsensitive);
    style.replace(QStringLiteral("#EF4444"), QLatin1String(kRed), Qt::CaseInsensitive);
    style.replace(QStringLiteral("#16A34A"), QLatin1String(kGreen), Qt::CaseInsensitive);
    style.replace(QStringLiteral("#059669"), QLatin1String(kGreen), Qt::CaseInsensitive);
    style.replace(QStringLiteral("#F59E0B"), QLatin1String(kYellow), Qt::CaseInsensitive);
    style.replace(QStringLiteral("#A16207"), QLatin1String(kYellow), Qt::CaseInsensitive);
    style.replace(QStringLiteral("#D97706"), QLatin1String(kYellow), Qt::CaseInsensitive);
}

void positionCheckMark(QCheckBox *box)
{
    auto *mark = box->findChild<QLabel *>(QStringLiteral("schemeBCheckMark"));
    if (!mark) return;
    QStyleOptionButton option;
    option.initFrom(box);
    mark->setGeometry(box->style()->subElementRect(QStyle::SE_CheckBoxIndicator, &option, box));
}

class RoundedSidebarDelegate final : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        Q_UNUSED(option)
        // V2 updates the spacer's SizeHintRole on every resize. Scheme B keeps
        // the settings row beside the other navigation items instead.
        if (index.row() == 7) return QSize(0, 0);
        return QSize(184, 48);
    }
};

class SchemeBTaskDelegate final : public TaskItemDelegate
{
public:
    using TaskItemDelegate::TaskItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        const int column = index.column();
        painter->save();
        if (option.state & (QStyle::State_Selected | QStyle::State_MouseOver)) {
            painter->fillRect(option.rect, QColor("#F5F7FA"));
        }
        painter->setPen(QColor("#F1F5F9"));
        painter->drawLine(option.rect.bottomLeft(), option.rect.bottomRight());
        painter->restore();

        if (column == TaskTableModel::ColCategory || column == TaskTableModel::ColStatus) {
            const QString value = index.data(Qt::DisplayRole).toString();
            QColor foreground(kText), background("#F1F5F9");
            if (value == QStringLiteral("催")) { foreground = QColor(kYellow); background = QColor("#FFF8DB"); }
            else if (value == QStringLiteral("查")) { foreground = QColor("#1D4ED8"); background = QColor("#EFF6FF"); }
            else if (value == QStringLiteral("报")) { foreground = QColor("#C2410C"); background = QColor("#FFF7ED"); }
            else if (value == QStringLiteral("办")) { foreground = QColor(kGreen); background = QColor("#E8FAEF"); }
            else if (value == QStringLiteral("进行中")) { foreground = QColor(kYellow); background = QColor("#FFF3C4"); }
            else if (value == QStringLiteral("已完成")) { foreground = QColor(kGreen); background = QColor("#DDF8E7"); }
            else if (value == QStringLiteral("已删除")) { foreground = QColor(kRed); background = QColor("#FFE7E6"); }
            QFont badgeFont(option.font);
            badgeFont.setFamily(QStringLiteral("MiSans"));
            badgeFont.setBold(false);
            const QFontMetrics metrics(badgeFont);
            const QRect badge(option.rect.center().x() - (metrics.horizontalAdvance(value) + 16) / 2,
                              option.rect.center().y() - 12, metrics.horizontalAdvance(value) + 16, 24);
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            painter->setPen(Qt::NoPen);
            painter->setBrush(background);
            painter->drawRoundedRect(badge, 6, 6);
            painter->setPen(foreground);
            painter->setFont(badgeFont);
            painter->drawText(badge, Qt::AlignCenter, value);
            painter->restore();
            return;
        }

        Task task;
        const QVariant value = index.data(Qt::UserRole);
        if (value.canConvert<Task>()) {
            task = value.value<Task>();
        }

        QColor color(kText);
        QString text = index.data(Qt::DisplayRole).toString();
        if (column == TaskTableModel::ColTitle) {
            text = task.title;
            if (task.isKeyTask) {
                color = QColor(kRed);
                if (!text.startsWith(QStringLiteral("【重点】"))) {
                    text.prepend(QStringLiteral("【重点】"));
                }
            }
        } else if (column == TaskTableModel::ColDeadline && task.deadline.isValid()) {
            const int days = QDate::currentDate().daysTo(task.deadline);
            if (days <= 0) color = QColor(kRed);
            else if (days < 3) color = QColor(kYellow);
        }

        painter->save();
        painter->setPen(color);
        QFont font(option.font);
        font.setFamily(QStringLiteral("MiSans"));
        font.setBold((column == TaskTableModel::ColTitle && task.isKeyTask)
                     || (column == TaskTableModel::ColDeadline && color != QColor(kText)));
        painter->setFont(font);
        const int alignment = column == TaskTableModel::ColTitle
            ? Qt::AlignLeft | Qt::AlignVCenter : Qt::AlignCenter;
        painter->drawText(option.rect.adjusted(12, 0, -12, 0), alignment, text);
        painter->restore();
    }
};

class SchemeBReferenceDelegate final : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

protected:
    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override
    {
        QStyledItemDelegate::initStyleOption(option, index);
        if (auto *table = qobject_cast<QTableView *>(parent())) {
            option->font = table->font();
        }
    }
};

class SchemeBWindowBackdrop final : public QWidget
{
public:
    using QWidget::QWidget;

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor("#F5F6FA"));
        painter.drawRoundedRect(QRectF(rect()), 16, 16);
    }
};

QString categoryColor(const QString &category)
{
    if (category == QStringLiteral("催")) return QLatin1String(kYellow);
    if (category == QStringLiteral("查")) return QStringLiteral("#1D4ED8");
    if (category == QStringLiteral("报")) return QStringLiteral("#C2410C");
    if (category == QStringLiteral("办")) return QLatin1String(kGreen);
    return QLatin1String(kText);
}

QString categoryBackground(const QString &category)
{
    if (category == QStringLiteral("催")) return QStringLiteral("#FFF8DB");
    if (category == QStringLiteral("查")) return QStringLiteral("#EFF6FF");
    if (category == QStringLiteral("报")) return QStringLiteral("#FFF7ED");
    if (category == QStringLiteral("办")) return QStringLiteral("#E8FAEF");
    return QStringLiteral("#F1F5F9");
}

QFrame *createSchemeBHomeRow(HomeView *home, QWidget *container)
{
    auto *row = new QFrame(container);
    row->setObjectName(QStringLiteral("InProgressItem"));
    row->setFixedHeight(48);
    row->setCursor(Qt::PointingHandCursor);
    row->setStyleSheet(QStringLiteral(
        "QFrame#InProgressItem { background:#FFFFFF; border:none; border-bottom:1px solid #F1F5F9; }"
        "QFrame#InProgressItem:hover { background:#F5F7FA; }"));
    row->installEventFilter(home);
    auto *layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 8, 0, 8);
    layout->setSpacing(10);

    auto *tag = new QFrame(row);
    tag->setFixedSize(32, 24);
    auto *tagLayout = new QHBoxLayout(tag);
    tagLayout->setContentsMargins(0, 0, 0, 0);
    auto *category = new QLabel(tag);
    category->setAlignment(Qt::AlignCenter);
    tagLayout->addWidget(category);
    layout->addWidget(tag);

    auto *title = new QLabel(row);
    title->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    layout->addWidget(title);
    auto *star = new QPushButton(QStringLiteral("☆"), row);
    star->setFixedSize(28, 28);
    star->setCursor(Qt::PointingHandCursor);
    star->setProperty("taskId", 0);
    star->setProperty("focused", false);
    layout->addWidget(star);
    QObject::connect(star, &QPushButton::clicked, home, [home, star]() {
        const int id = star->property("taskId").toInt();
        if (id <= 0) return;
        FocusService service;
        if (!service.setFocus(id, !star->property("focused").toBool()).ok) {
            QToolTip::showText(star->mapToGlobal(QPoint(0, -30)),
                               QStringLiteral("今日专注已达上限（3个）"), star);
            return;
        }
        QTimer::singleShot(0, home, [home]() { home->refresh(); });
    });
    layout->addStretch();
    auto *source = new QLabel(row);
    source->setFixedWidth(104);
    source->setAlignment(Qt::AlignCenter);
    layout->addWidget(source);
    auto *deadline = new QLabel(row);
    deadline->setFixedWidth(102);
    deadline->setAlignment(Qt::AlignCenter);
    layout->addWidget(deadline);
    return row;
}

class SchemeBDatePopup final : public QWidget
{
public:
    explicit SchemeBDatePopup(QDateEdit *edit) : QWidget(edit, Qt::Popup), edit_(edit)
    {
        setObjectName(QStringLiteral("schemeBDatePopup"));
        setFixedSize(450, 350);
        QPainterPath shape;
        shape.addRoundedRect(QRectF(rect()), 5, 5);
        setMask(QRegion(shape.toFillPolygon().toPolygon()));
        setStyleSheet(QStringLiteral(
            "QWidget#schemeBDatePopup { background:#FFFFFF; border:1px solid #D9DFE9; border-radius:5px; }"
            "QWidget#schemeBDateQuickBar { background:#FFFFFF; border-right:1px solid #E3E7EF; }"
            "QPushButton#schemeBDateQuick { background:transparent; border:none; color:#475467; "
            "text-align:left; padding:7px 12px; font-size:14px; }"
            "QPushButton#schemeBDateQuick:hover { color:#2164E5; background:#F2F6FF; }"
            "QPushButton#schemeBDateNav { background:transparent; border:none; color:#475467; "
            "font-size:19px; padding:0; }"
            "QPushButton#schemeBDateNav:hover { color:#2164E5; background:#F2F6FF; border-radius:4px; }"
            "QLabel#schemeBDateHeading { color:#344054; font-size:15px; font-weight:700; }"
            "QLabel#schemeBDateWeekday { color:#667085; font-size:12px; }"
            "QPushButton#schemeBDateDay { background:transparent; border:none; border-radius:17px; "
            "color:#344054; font-size:13px; padding:0; }"
            "QPushButton#schemeBDateDay:hover { background:#EEF3FB; }"
            "QPushButton#schemeBDateDay[adjacent='true'] { color:#98A2B3; }"
            "QPushButton#schemeBDateDay[selected='true'] { background:#2B72EF; color:#FFFFFF; font-weight:700; }"
            "QPushButton#schemeBDateDay:disabled { color:#C8CDD7; background:transparent; }"));

        auto *root = new QHBoxLayout(this);
        root->setContentsMargins(0, 0, 0, 0);
        root->setSpacing(0);
        auto *quickBar = new QWidget(this);
        quickBar->setObjectName(QStringLiteral("schemeBDateQuickBar"));
        quickBar->setFixedWidth(110);
        auto *quickLayout = new QVBoxLayout(quickBar);
        quickLayout->setContentsMargins(0, 15, 0, 8);
        quickLayout->setSpacing(1);
        for (const auto &preset : QList<QPair<QString, int>>{
                 {QStringLiteral("今天"), 0},
                 {QStringLiteral("昨天"), -1},
                 {QStringLiteral("一周前"), -7}}) {
            auto *button = new QPushButton(preset.first, quickBar);
            button->setObjectName(QStringLiteral("schemeBDateQuick"));
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
        auto makeNav = [this, navigation](const QString &text, int months) {
            auto *button = new QPushButton(text, this);
            button->setObjectName(QStringLiteral("schemeBDateNav"));
            button->setFixedSize(26, 30);
            button->setCursor(Qt::PointingHandCursor);
            navigation->addWidget(button);
            connect(button, &QPushButton::clicked, this, [this, months]() {
                visibleMonth_ = visibleMonth_.addMonths(months);
                updateDays();
            });
        };
        makeNav(QStringLiteral("«"), -12);
        makeNav(QStringLiteral("‹"), -1);
        navigation->addStretch();
        heading_ = new QLabel(calendar);
        heading_->setObjectName(QStringLiteral("schemeBDateHeading"));
        heading_->setAlignment(Qt::AlignCenter);
        navigation->addWidget(heading_);
        navigation->addStretch();
        makeNav(QStringLiteral("›"), 1);
        makeNav(QStringLiteral("»"), 12);
        calendarLayout->addLayout(navigation);

        auto *grid = new QGridLayout();
        grid->setContentsMargins(0, 0, 0, 0);
        grid->setHorizontalSpacing(4);
        grid->setVerticalSpacing(7);
        const QStringList weekdays = {QStringLiteral("周日"), QStringLiteral("周一"),
            QStringLiteral("周二"), QStringLiteral("周三"), QStringLiteral("周四"),
            QStringLiteral("周五"), QStringLiteral("周六")};
        for (int col = 0; col < 7; ++col) {
            auto *label = new QLabel(weekdays.at(col), calendar);
            label->setObjectName(QStringLiteral("schemeBDateWeekday"));
            label->setAlignment(Qt::AlignCenter);
            label->setFixedHeight(26);
            grid->addWidget(label, 0, col);
        }
        for (int week = 0; week < 6; ++week) {
            for (int col = 0; col < 7; ++col) {
                auto *day = new QPushButton(calendar);
                day->setObjectName(QStringLiteral("schemeBDateDay"));
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
        selectedDate_ = edit->date() == edit->minimumDate()
            && !edit->specialValueText().isEmpty() ? QDate::currentDate() : edit->date();
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
        heading_->setText(QStringLiteral("%1年%2月")
            .arg(visibleMonth_.year())
            .arg(visibleMonth_.month()));
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

} // namespace

SchemeBStyler::SchemeBStyler(QApplication &app, MainWindow &window)
    : QObject(&window), app_(app), window_(window)
{
    // The V2 polygon window mask produces visible diagonal facets at the
    // corners. Scheme B draws its own antialiased surface on a translucent
    // frameless window instead.
    window_.setAttribute(Qt::WA_TranslucentBackground);
    window_.setAutoFillBackground(false);
    window_.setStyleSheet(QStringLiteral("QMainWindow { background:transparent; border:none; }"));
    window_.clearMask();
    auto *backdrop = new SchemeBWindowBackdrop(&window_);
    backdrop->setObjectName(QStringLiteral("schemeBWindowBackdrop"));
    backdrop->setAttribute(Qt::WA_TransparentForMouseEvents);
    backdrop->setGeometry(window_.rect());
    backdrop->show();
    backdrop->lower();
    if (auto *status = window_.statusBar()) {
        status->setStyleSheet(QStringLiteral("QStatusBar { background:transparent; border:none; }"));
    }

    const QString userFontDir = QDir::homePath()
        + QStringLiteral("/AppData/Local/Microsoft/Windows/Fonts/");
    bool loadedOfficialFont = false;
    for (const QString &weight : {QStringLiteral("Regular"), QStringLiteral("Medium"),
                                  QStringLiteral("Semibold"), QStringLiteral("Bold")}) {
        loadedOfficialFont |= QFontDatabase::addApplicationFont(
            userFontDir + QStringLiteral("MiSans-Official-%1.ttf").arg(weight)) >= 0;
    }
    if (!loadedOfficialFont) {
        QFontDatabase::addApplicationFont(QStringLiteral("C:/Windows/Fonts/MiSans-Regular.otf"));
    }
    QFont defaultFont(QStringLiteral("MiSans"), 14);
    app_.setFont(defaultFont);

    QFile sourceStyle(QStringLiteral(":/styles/neutral_modern.qss"));
    QFile schemeStyle(QStringLiteral(":/styles/scheme_b.qss"));
    if (sourceStyle.open(QIODevice::ReadOnly | QIODevice::Text)
        && schemeStyle.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString style = QString::fromUtf8(sourceStyle.readAll());
        style.replace(QStringLiteral("Microsoft YaHei"), QStringLiteral("MiSans"));
        normalizeStatusColors(style);
        style += QLatin1Char('\n') + QString::fromUtf8(schemeStyle.readAll());
        app_.setStyleSheet(style);
    }

    if (auto *sidebar = window_.findChild<QListWidget *>(QStringLiteral("sidebar"))) {
        sidebar->setItemDelegate(new RoundedSidebarDelegate(sidebar));
        sidebar->setFixedWidth(184);
        sidebar->setIconSize(QSize(20, 20));
        // Keep V2's row indices, but not its expanding spacer that pins settings
        // to the bottom of the sidebar.
        sidebar->item(7)->setSizeHint(QSize(0, 0));
        for (int row = 0; row < sidebar->count(); ++row) {
            if (row != 8) {
                sidebar->item(row)->setIcon(QIcon());
                continue;
            }
            QPixmap pixmap(40, 40);
            pixmap.fill(Qt::transparent);
            QPainter icon(&pixmap);
            icon.setRenderHint(QPainter::Antialiasing);
            icon.scale(2, 2);
            icon.setPen(QPen(QColor("#384454"), 1.5));
            QPainterPath gear;
            constexpr int teeth = 8;
            for (int i = 0; i < teeth * 4; ++i) {
                const qreal angle = i * 3.141592653589793 / (teeth * 2) - 3.141592653589793 / 2;
                const qreal radius = (i % 4 == 1 || i % 4 == 2) ? 8.1 : 6.4;
                const QPointF point(10 + radius * std::cos(angle), 10 + radius * std::sin(angle));
                if (i == 0) gear.moveTo(point); else gear.lineTo(point);
            }
            gear.closeSubpath();
            icon.drawPath(gear);
            icon.drawEllipse(QRectF(7.5, 7.5, 5, 5));
            icon.end();
            pixmap.setDevicePixelRatio(2);
            sidebar->item(row)->setIcon(QIcon(pixmap));
        }
    }
    if (auto *titlebar = window_.findChild<QWidget *>(QStringLiteral("titlebar"))) {
        titlebar->setFixedHeight(70);
        titlebar->setStyleSheet(QStringLiteral("QWidget#titlebar { background:transparent; }"));
        const auto titleLabels = titlebar->findChildren<QLabel *>(QString(), Qt::FindDirectChildrenOnly);
        if (titleLabels.size() >= 2) {
            titleLabels.at(0)->setProperty("schemeBNoLabelReset", true);
            QFont titleFont = titleLabels.at(1)->font();
            titleFont.setFamily(QStringLiteral("MiSans"));
            titleFont.setBold(false);
            titleLabels.at(1)->setFont(titleFont);
            titleLabels.at(1)->setFixedWidth(126);
        }
        if (auto *bar = qobject_cast<QHBoxLayout *>(titlebar->layout())) {
            bar->setContentsMargins(20, 0, 8, 0);
            bar->setSpacing(8);
            auto *back = new QPushButton(QStringLiteral("‹"), titlebar);
            auto *forward = new QPushButton(QStringLiteral("›"), titlebar);
            auto *reload = new QPushButton(QStringLiteral("↻"), titlebar);
            for (auto *button : {back, forward, reload}) {
                button->setObjectName(QStringLiteral("shellNavigationButton"));
                button->setFixedSize(32, 36);
                button->setCursor(Qt::PointingHandCursor);
            }
            back->setToolTip(QStringLiteral("后退"));
            forward->setToolTip(QStringLiteral("前进"));
            reload->setToolTip(QStringLiteral("刷新当前页面"));
            auto *search = new QLineEdit(titlebar);
            search->setObjectName(QStringLiteral("shellSearch"));
            search->setPlaceholderText(QStringLiteral("搜索任务名称、编号，回车搜索"));
            search->setClearButtonEnabled(true);
            search->setFixedHeight(38);
            search->setMinimumWidth(210);
            search->setMaximumWidth(340);
            auto *create = new QPushButton(QStringLiteral("＋ 新建"), titlebar);
            create->setObjectName(QStringLiteral("shellCreate"));
            create->setFixedSize(82, 38);
            create->setCursor(Qt::PointingHandCursor);
            bar->insertWidget(2, back);
            bar->insertWidget(3, forward);
            bar->insertWidget(4, reload);
            bar->insertWidget(5, search, 1);
            bar->insertWidget(6, create);
            auto *sidebar = window_.findChild<QListWidget *>(QStringLiteral("sidebar"));
            struct History { QList<int> rows{0}; int position = 0; bool navigating = false; };
            auto history = std::make_shared<History>();
            auto updateButtons = [history, back, forward]() {
                back->setEnabled(history->position > 0);
                forward->setEnabled(history->position + 1 < history->rows.size());
            };
            updateButtons();
            connect(sidebar, &QListWidget::currentRowChanged, this, [history, updateButtons](int row) {
                if (row == 7) return;
                if (row == history->rows.at(history->position)) { updateButtons(); return; }
                if (!history->navigating) {
                    while (history->rows.size() > history->position + 1) history->rows.removeLast();
                    history->rows.append(row);
                    ++history->position;
                }
                updateButtons();
            });
            connect(back, &QPushButton::clicked, this, [history, sidebar, updateButtons]() {
                if (history->position == 0) return;
                history->navigating = true;
                sidebar->setCurrentRow(history->rows.at(--history->position));
                history->navigating = false;
                updateButtons();
            });
            connect(forward, &QPushButton::clicked, this, [history, sidebar, updateButtons]() {
                if (history->position + 1 >= history->rows.size()) return;
                history->navigating = true;
                sidebar->setCurrentRow(history->rows.at(++history->position));
                history->navigating = false;
                updateButtons();
            });
            connect(reload, &QPushButton::clicked, this, [this, sidebar]() {
                QMetaObject::invokeMethod(sidebar, "currentRowChanged", Q_ARG(int, sidebar->currentRow()));
                ToastWidget::show(&window_, ToastWidget::Success, QStringLiteral("刷新完成"));
                for (ToastWidget *toast : window_.findChildren<ToastWidget *>()) {
                    for (QLabel *label : toast->findChildren<QLabel *>()) {
                        label->setProperty("schemeBNoLabelReset", true);
                        label->setWordWrap(false);
                        label->setStyleSheet(QStringLiteral(
                            "color:#FFFFFF; background:transparent; border:none; "
                            "padding:0; font-family:MiSans; font-size:14px;"));
                    }
                    toast->setMinimumWidth(154);
                    toast->adjustSize();
                    toast->move((window_.width() - toast->width()) / 2,
                                window_.height() - 48 - toast->height());
                }
            });
            connect(create, &QPushButton::clicked, this, [this]() {
                if (auto *home = window_.findChild<HomeView *>()) {
                    if (auto *button = home->findChild<QPushButton *>(QStringLiteral("btnPrimary"))) button->click();
                }
            });
            connect(search, &QLineEdit::returnPressed, this, [this, sidebar, search]() {
                sidebar->setCurrentRow(1);
                if (auto *tasks = window_.findChild<TaskListView *>()) {
                    if (auto *input = tasks->findChild<QLineEdit *>()) {
                        input->setText(search->text());
                        QMetaObject::invokeMethod(tasks, "onSearchClicked");
                    }
                }
            });
            // The existing search entry accepts both title and task number.
        }
        for (const QString &buttonName : {QStringLiteral("btnMinimize"),
                                          QStringLiteral("btnMaximize"),
                                          QStringLiteral("btnClose")}) {
            if (auto *button = titlebar->findChild<QPushButton *>(buttonName)) {
                button->setText(QString());
                button->setIcon(windowControlIcon(buttonName));
                button->setIconSize(QSize(20, 20));
                button->setStyleSheet(QStringLiteral(
                    "QPushButton { background:transparent; color:#1F2937; border:none; "
                    "border-radius:0; padding:0; }"
                    "QPushButton:hover { background:#E9EEF5; color:#111827; }"
                    "QPushButton:pressed { background:#DCE5F2; }"));
            }
        }
    }
    if (auto *stack = window_.findChild<QStackedWidget *>()) {
        stack->setObjectName(QStringLiteral("schemeBWorkspace"));
        if (auto *layout = stack->parentWidget()->layout()) layout->setContentsMargins(0, 0, 8, 0);
        for (QScrollArea *scroll : stack->findChildren<QScrollArea *>()) {
            scroll->setStyleSheet(QStringLiteral("QScrollArea { background:white; border:none; }"));
            scroll->viewport()->setAutoFillBackground(true);
            QPalette white = scroll->viewport()->palette();
            white.setColor(QPalette::Window, Qt::white);
            scroll->viewport()->setPalette(white);
            if (auto *page = scroll->widget()) {
                page->setObjectName(QStringLiteral("schemeBPage"));
                page->setStyleSheet(QStringLiteral("QWidget#schemeBPage { background:#FFFFFF; }"));
            }
        }
        stack->installEventFilter(this);
        updateContentCorner(stack);
    }
    app_.installEventFilter(this);
    app_.installNativeEventFilter(this);
    // Qt's private combo popup clears QWidget::mask() when it opens. Restore
    // the native rounded region after that private show path has run.
    auto *popupMaskTimer = new QTimer(this);
    popupMaskTimer->setInterval(50);
    connect(popupMaskTimer, &QTimer::timeout, this, [this]() {
        auto *popup = app_.activePopupWidget();
        if (!popup || !popup->property("schemeBTransparentCorners").toBool()
            || !popup->mask().isEmpty()) return;
        QPainterPath shape;
        shape.addRoundedRect(QRectF(popup->rect()), 8, 8);
        popup->setMask(QRegion(shape.toFillPolygon().toPolygon()));
    });
    popupMaskTimer->start();
    scheduleRefresh();
}

bool SchemeBStyler::nativeEventFilter(const QByteArray &eventType, void *message, long *result)
{
#ifdef Q_OS_WIN
    if (eventType == QByteArrayLiteral("windows_generic_MSG")) {
        const auto *msg = static_cast<MSG *>(message);
        if (msg->hwnd == reinterpret_cast<HWND>(window_.winId())
            && msg->message == WM_NCHITTEST && !window_.isMaximized()) {
            RECT bounds;
            if (!GetWindowRect(msg->hwnd, &bounds)) return false;
            constexpr int edge = 8;
            const int x = static_cast<short>(LOWORD(msg->lParam));
            const int y = static_cast<short>(HIWORD(msg->lParam));
            const bool left = x >= bounds.left && x < bounds.left + edge;
            const bool right = x < bounds.right && x >= bounds.right - edge;
            const bool top = y >= bounds.top && y < bounds.top + edge;
            const bool bottom = y < bounds.bottom && y >= bounds.bottom - edge;
            if (top && left) *result = HTTOPLEFT;
            else if (top && right) *result = HTTOPRIGHT;
            else if (bottom && left) *result = HTBOTTOMLEFT;
            else if (bottom && right) *result = HTBOTTOMRIGHT;
            else if (left) *result = HTLEFT;
            else if (right) *result = HTRIGHT;
            else if (top) *result = HTTOP;
            else if (bottom) *result = HTBOTTOM;
            else return false;
            return true;
        }
    }
#else
    Q_UNUSED(eventType)
    Q_UNUSED(message)
    Q_UNUSED(result)
#endif
    return false;
}

bool SchemeBStyler::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::Wheel) {
        auto *widget = qobject_cast<QWidget *>(watched);
        auto *wheel = static_cast<QWheelEvent *>(event);
        if (widget && !(wheel->modifiers() & (Qt::ControlModifier | Qt::AltModifier))) {
            QAbstractScrollArea *area = nullptr;
            bool overSettingsList = false;
            // A wheel can be delivered to the settings page's outer viewport
            // even while the pointer is over a source/tag/handler list.
            if (auto *settings = window_.findChild<SettingsView *>();
                settings && settings->isVisible()) {
                for (QListWidget *list : settings->findChildren<QListWidget *>()) {
                    if (list->isVisible()
                        && list->rect().contains(list->mapFromGlobal(
                            wheel->globalPosition().toPoint()))) {
                        area = list;
                        overSettingsList = true;
                        break;
                    }
                }
            }
            if (!area) {
                for (QWidget *ancestor = widget; ancestor; ancestor = ancestor->parentWidget()) {
                    if ((area = qobject_cast<QAbstractScrollArea *>(ancestor))) break;
                }
            }
            if (area && area->objectName() != QStringLiteral("sidebar")) {
                const bool eligibleTable = qobject_cast<QTableView *>(area)
                    && ((window_.findChild<TaskListView *>()
                         && window_.findChild<TaskListView *>()->isAncestorOf(area))
                        || (window_.findChild<PeriodicTaskView *>()
                            && window_.findChild<PeriodicTaskView *>()->isAncestorOf(area))
                        || (window_.findChild<ReferenceView *>()
                            && window_.findChild<ReferenceView *>()->isAncestorOf(area)));
                const bool verticalToHorizontal = eligibleTable
                    && (!area->verticalScrollBar()->isVisible()
                        || area->verticalScrollBar()->maximum() == area->verticalScrollBar()->minimum())
                    && area->horizontalScrollBar()->maximum() > area->horizontalScrollBar()->minimum();
                const bool horizontal = (wheel->modifiers() & Qt::ShiftModifier)
                    || (wheel->angleDelta().x() != 0 && wheel->angleDelta().y() == 0)
                    || verticalToHorizontal;
                auto *bar = horizontal ? area->horizontalScrollBar() : area->verticalScrollBar();
                const int pixelDelta = horizontal && wheel->pixelDelta().x()
                    ? wheel->pixelDelta().x() : wheel->pixelDelta().y();
                if (bar && bar->maximum() > bar->minimum() && pixelDelta) {
                    bar->setValue(bar->value() - pixelDelta);
                    return true;
                }
                const int delta = horizontal && wheel->angleDelta().x()
                    ? wheel->angleDelta().x() : wheel->angleDelta().y();
                if (bar && bar->maximum() > bar->minimum() && delta) {
                    auto *animation = bar->findChild<QPropertyAnimation *>(
                        QStringLiteral("schemeBWheelAnimation"));
                    if (!animation) {
                        animation = new QPropertyAnimation(bar, "value", bar);
                        animation->setObjectName(QStringLiteral("schemeBWheelAnimation"));
                        animation->setDuration(210);
                        animation->setEasingCurve(QEasingCurve::OutCubic);
                    }
                    const int start = animation->state() == QAbstractAnimation::Running
                        ? bar->property("schemeBWheelTarget").toInt() : bar->value();
                    const int target = qBound(bar->minimum(),
                        start - qRound(delta * 36.0 / 120.0), bar->maximum());
                    if (target != bar->value()) {
                        animation->stop();
                        bar->setProperty("schemeBWheelTarget", target);
                        animation->setStartValue(bar->value());
                        animation->setEndValue(target);
                        animation->start();
                    }
                    // Do not pass a wheel at the inner area's edge to its parent page.
                    return true;
                }
                // A wheel over an inner list must never scroll the containing
                // page, including when the list is already at an edge.
                if (overSettingsList) return true;
                for (QWidget *parent = area->parentWidget(); parent; parent = parent->parentWidget()) {
                    if (qobject_cast<QAbstractScrollArea *>(parent)) return true;
                }
            }
        }
    }
    if (event->type() == QEvent::Show
        && watched->property("schemeBDetailHidden").toBool()) {
        if (auto *badge = qobject_cast<QWidget *>(watched)) badge->hide();
        return true;
    }
    if (auto *widget = qobject_cast<QWidget *>(watched)) {
        QDateEdit *dateEdit = nullptr;
        for (QWidget *ancestor = widget; ancestor; ancestor = ancestor->parentWidget()) {
            if (ancestor->objectName() == QStringLiteral("schemeBDatePopup")) break;
            if ((dateEdit = qobject_cast<QDateEdit *>(ancestor))) break;
        }
        if (dateEdit && (event->type() == QEvent::MouseButtonPress
                         || event->type() == QEvent::MouseButtonRelease)) {
            auto *mouse = static_cast<QMouseEvent *>(event);
            if (mouse->button() == Qt::LeftButton) {
                if (event->type() == QEvent::MouseButtonPress) {
                    auto *popup = dateEdit->findChild<SchemeBDatePopup *>(QStringLiteral("schemeBDatePopup"));
                    if (!popup) popup = new SchemeBDatePopup(dateEdit);
                    popup->openAt(dateEdit);
                }
                return true;
            }
        }
    }
    if (auto *button = qobject_cast<QPushButton *>(watched)) {
        if (button->property("taskId").isValid() && button->property("focused").isValid()) {
            if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::KeyPress) {
                if (auto *home = window_.findChild<HomeView *>()) {
                    for (QWidget *ancestor = home->parentWidget(); ancestor; ancestor = ancestor->parentWidget()) {
                        if (auto *scroll = qobject_cast<QScrollArea *>(ancestor)) {
                            button->setProperty("schemeBScrollBeforeFocus", scroll->verticalScrollBar()->value());
                            break;
                        }
                    }
                }
            } else if (event->type() == QEvent::MouseButtonRelease || event->type() == QEvent::KeyRelease) {
                const int oldPosition = button->property("schemeBScrollBeforeFocus").toInt();
                if (auto *home = window_.findChild<HomeView *>()) {
                    for (QWidget *ancestor = home->parentWidget(); ancestor; ancestor = ancestor->parentWidget()) {
                        if (auto *scroll = qobject_cast<QScrollArea *>(ancestor)) {
                            auto *scrollbar = scroll->verticalScrollBar();
                            QTimer::singleShot(0, scrollbar, [scrollbar, oldPosition]() {
                                scrollbar->setValue(oldPosition);
                            });
                            break;
                        }
                    }
                }
            }
        }
    }
    if (event->type() == QEvent::Resize) {
        if (watched == &window_) {
            if (auto *backdrop = window_.findChild<SchemeBWindowBackdrop *>(
                    QStringLiteral("schemeBWindowBackdrop"))) {
                backdrop->setGeometry(window_.rect());
                backdrop->lower();
            }
            QTimer::singleShot(0, &window_, [this]() { window_.clearMask(); });
        } else if (auto *popup = qobject_cast<QWidget *>(watched);
                   popup && popup->property("schemeBComboPopup").toBool()) {
            QPainterPath shape;
            shape.addRoundedRect(QRectF(popup->rect()), 8, 8);
            popup->setMask(QRegion(shape.toFillPolygon().toPolygon()));
        } else if (auto *detail = qobject_cast<TaskDetailView *>(watched)) {
            if (auto *title = detail->findChild<QLabel *>(QStringLiteral("schemeBDetailTitle"))) {
                title->setMaximumWidth(qMax(300, detail->width() - 100));
            }
        } else if (auto *stack = qobject_cast<QStackedWidget *>(watched)) {
            updateContentCorner(stack);
        } else if (auto *dateEdit = qobject_cast<QDateEdit *>(watched)) {
            if (auto *icon = dateEdit->findChild<QLabel *>(QStringLiteral("schemeBDateIcon"))) {
                icon->move(9, (dateEdit->height() - icon->height()) / 2);
            }
            if (auto *arrow = dateEdit->findChild<QLabel *>(QStringLiteral("schemeBDateArrow"))) {
                arrow->move(dateEdit->width() - arrow->width() - 9,
                            (dateEdit->height() - arrow->height()) / 2);
            }
        } else if (auto *box = qobject_cast<QCheckBox *>(watched)) {
            positionCheckMark(box);
        } else if (auto *combo = qobject_cast<QComboBox *>(watched)) {
            if (auto *arrow = combo->findChild<QLabel *>(QStringLiteral("schemeBComboArrow"))) {
                arrow->move(combo->width() - arrow->width() - 9,
                            (combo->height() - arrow->height()) / 2);
                arrow->raise();
            }
        }
    } else if (event->type() == QEvent::ChildAdded || event->type() == QEvent::ChildRemoved) {
        auto *parent = qobject_cast<QWidget *>(watched);
        if (parent && (parent == &window_ || window_.isAncestorOf(parent))) {
            scheduleRefresh();
        }
    } else if (event->type() == QEvent::StyleChange && !refreshing_) {
        auto *label = qobject_cast<QLabel *>(watched);
        if (label && window_.isAncestorOf(label)
            && !label->property("schemeBNoLabelReset").toBool()
            && !label->styleSheet().contains(QStringLiteral("/*schemeBText*/"))) {
            scheduleRefresh();
        }
    }
    return QObject::eventFilter(watched, event);
}

void SchemeBStyler::scheduleRefresh()
{
    if (refreshPending_) return;
    refreshPending_ = true;
    QTimer::singleShot(0, this, [this]() {
        refreshPending_ = false;
        refresh();
    });
}

void SchemeBStyler::updateContentCorner(QStackedWidget *stack)
{
    const QRect bounds = stack->rect();
    if (bounds.isEmpty()) return;
    constexpr int radius = 16;
    QPainterPath path;
    path.addRoundedRect(QRectF(bounds), radius, radius);
    stack->setMask(QRegion(path.toFillPolygon().toPolygon()));
}

void SchemeBStyler::refresh()
{
    if (refreshing_) return;
    refreshing_ = true;
    auto *home = window_.findChild<HomeView *>();
    if (!home) { refreshing_ = false; return; }

    const auto widgets = window_.findChildren<QWidget *>();
    for (QWidget *widget : widgets) {
        QFont font = widget->font();
        if (font.family() != QStringLiteral("MiSans")) {
            font.setFamily(QStringLiteral("MiSans"));
            widget->setFont(font);
        }
        QString inlineStyle = widget->styleSheet();
        const QString originalStyle = inlineStyle;
        if (inlineStyle.contains(QStringLiteral("Microsoft YaHei"))) {
            inlineStyle.replace(QStringLiteral("Microsoft YaHei"), QStringLiteral("MiSans"));
        }
        normalizeStatusColors(inlineStyle);
        if (inlineStyle != originalStyle) widget->setStyleSheet(inlineStyle);
        if (auto *label = qobject_cast<QLabel *>(widget)) {
            if (label->property("schemeBNoLabelReset").toBool()) {
                continue;
            }
            static const QRegularExpression semanticText(
                QStringLiteral("\\bcolor\\s*:\\s*#(?:FA423E|40C977|F5B400)\\b"),
                QRegularExpression::CaseInsensitiveOption);
            if (!label->styleSheet().contains(QStringLiteral("/*schemeBText*/"))
                && !semanticText.match(inlineStyle).hasMatch()) {
                label->setStyleSheet(label->styleSheet()
                                     + QStringLiteral("; color:#000000; background:transparent; "
                                                      "border:none; padding-left:6px; padding-right:6px; "
                                                      "/*schemeBText*/"));
            }
            label->setAutoFillBackground(false);
            if (label->text() == QStringLiteral("任务标题")) label->setText(QStringLiteral("任务名称"));
        }
    }

    if (auto *button = home->findChild<QPushButton *>(QStringLiteral("btnPrimary"))) button->hide();
    for (QDateEdit *edit : window_.findChildren<QDateEdit *>()) {
        if (edit->calendarPopup()) edit->setCalendarPopup(false);
        if (edit->width() < 156) edit->setMinimumWidth(156);
        if (!edit->property("schemeBDateStyled").toBool()) {
            edit->setStyleSheet(QStringLiteral(
                "QDateEdit { background:#FFFFFF; border:1px solid #DCE3EF; border-radius:7px; "
                "color:#172033; padding:6px 27px 6px 28px; font-size:14px; }"
                "QDateEdit:focus { border-color:#2B72EF; }"
                "QDateEdit::up-button, QDateEdit::down-button { width:0; height:0; border:none; }"
                "QDateEdit::up-arrow, QDateEdit::down-arrow { image:none; width:0; height:0; }"));
            edit->setProperty("schemeBDateStyled", true);
        }
        if (!edit->findChild<QLabel *>(QStringLiteral("schemeBDateIcon"))) {
            QPixmap iconImage(16, 16);
            iconImage.fill(Qt::transparent);
            QPainter painter(&iconImage);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setPen(QPen(QColor("#667085"), 1.3));
            painter.drawRoundedRect(QRectF(1.5, 3.5, 13, 11), 1.5, 1.5);
            painter.drawLine(QPointF(1.5, 7), QPointF(14.5, 7));
            painter.drawLine(QPointF(5, 1.5), QPointF(5, 5));
            painter.drawLine(QPointF(11, 1.5), QPointF(11, 5));
            painter.end();
            auto *icon = new QLabel(edit);
            icon->setObjectName(QStringLiteral("schemeBDateIcon"));
            icon->setProperty("schemeBNoLabelReset", true);
            icon->setAttribute(Qt::WA_TransparentForMouseEvents);
            icon->setStyleSheet(QStringLiteral("background:transparent; border:none; padding:0;"));
            icon->setFixedSize(16, 16);
            icon->setPixmap(iconImage);
            icon->move(9, (edit->height() - icon->height()) / 2);
            icon->show();
        }
        if (!edit->findChild<QLabel *>(QStringLiteral("schemeBDateArrow"))) {
            auto *arrow = new QLabel(edit);
            arrow->setObjectName(QStringLiteral("schemeBDateArrow"));
            arrow->setProperty("schemeBNoLabelReset", true);
            arrow->setAttribute(Qt::WA_TransparentForMouseEvents);
            arrow->setStyleSheet(QStringLiteral("background:transparent; border:none; padding:0;"));
            arrow->setFixedSize(14, 14);
            arrow->setPixmap(downArrowPixmap());
            arrow->move(edit->width() - arrow->width() - 9,
                        (edit->height() - arrow->height()) / 2);
            arrow->show();
        }
    }
    for (QCheckBox *box : window_.findChildren<QCheckBox *>()) {
        const QString checkStyle = QStringLiteral(
            "QCheckBox { background:transparent; color:#000000; font-size:14px; spacing:8px; }"
            "QCheckBox::indicator { width:17px; height:17px; border:1px solid #BAC6D8; "
            "border-radius:4px; background:#FFFFFF; }"
            "QCheckBox::indicator:checked { border:1px solid #2463EB; background:#2463EB; }");
        if (box->styleSheet() != checkStyle) box->setStyleSheet(checkStyle);
        auto *mark = box->findChild<QLabel *>(QStringLiteral("schemeBCheckMark"));
        if (!mark) {
            QPixmap tick(17, 17);
            tick.fill(Qt::transparent);
            QPainter painter(&tick);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setPen(QPen(Qt::white, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            painter.drawLine(QPointF(3.5, 8.5), QPointF(7, 12));
            painter.drawLine(QPointF(7, 12), QPointF(13.5, 5));
            painter.end();
            mark = new QLabel(box);
            mark->setObjectName(QStringLiteral("schemeBCheckMark"));
            mark->setProperty("schemeBNoLabelReset", true);
            mark->setAttribute(Qt::WA_TransparentForMouseEvents);
            mark->setStyleSheet(QStringLiteral("background:transparent; border:none; padding:0;"));
            mark->setAlignment(Qt::AlignCenter);
            mark->setPixmap(tick);
            connect(box, &QCheckBox::toggled, mark, &QWidget::setVisible);
        }
        positionCheckMark(box);
        mark->setVisible(box->isChecked());
    }

    auto clearPopupCorners = [this](QWidget *popup) {
        if (!popup || popup->property("schemeBTransparentCorners").toBool()) return;
        popup->setObjectName(QStringLiteral("schemeBComboPopup"));
        // A translucent native popup can composite its clipped corners as
        // black on Windows. Use an opaque surface and clip the native window.
        popup->setAttribute(Qt::WA_TranslucentBackground, false);
        popup->setAutoFillBackground(true);
        QPalette palette = popup->palette();
        palette.setColor(QPalette::Window, Qt::white);
        popup->setPalette(palette);
        popup->setStyleSheet(QStringLiteral(
            "QWidget#schemeBComboPopup { background:#FFFFFF; border:none; }"));
        popup->setProperty("schemeBTransparentCorners", true);
        QPainterPath shape;
        shape.addRoundedRect(QRectF(popup->rect()), 8, 8);
        popup->setMask(QRegion(shape.toFillPolygon().toPolygon()));
    };
    for (QComboBox *combo : window_.findChildren<QComboBox *>()) {
        if (!combo->styleSheet().contains(QStringLiteral("/*schemeBComboArrow*/"))) {
            combo->setStyleSheet(combo->styleSheet() + QStringLiteral(
                " QComboBox::drop-down { border:none; width:30px; }"
                " QComboBox::down-arrow { image:none; width:0; height:0; }"
                " /*schemeBComboArrow*/"));
        }
        if (!combo->findChild<QLabel *>(QStringLiteral("schemeBComboArrow"))) {
            auto *arrow = new QLabel(combo);
            arrow->setObjectName(QStringLiteral("schemeBComboArrow"));
            arrow->setProperty("schemeBNoLabelReset", true);
            arrow->setAttribute(Qt::WA_TransparentForMouseEvents);
            arrow->setStyleSheet(QStringLiteral("background:transparent; border:none; padding:0;"));
            arrow->setFixedSize(14, 14);
            arrow->setPixmap(downArrowPixmap());
            arrow->move(combo->width() - arrow->width() - 9,
                        (combo->height() - arrow->height()) / 2);
            arrow->show();
            arrow->raise();
        }
        clearPopupCorners(combo->view()->window());
    }
    for (QCompleter *completer : window_.findChildren<QCompleter *>()) {
        if (completer->popup()) clearPopupCorners(completer->popup()->window());
    }

    for (TaskDetailView *detail : window_.findChildren<TaskDetailView *>()) {
        auto *header = detail->findChild<QFrame *>(QStringLiteral("detailHeader"));
        auto *basic = detail->findChild<QFrame *>(QStringLiteral("basicInfoSection"));
        auto *attachment = detail->findChild<QFrame *>(QStringLiteral("attachmentSection"));
        if (!header || !basic || !attachment) continue;

        auto *headerLayout = qobject_cast<QVBoxLayout *>(header->layout());
        auto *topRow = headerLayout && headerLayout->itemAt(0)
            ? qobject_cast<QHBoxLayout *>(headerLayout->itemAt(0)->layout()) : nullptr;
        auto *titleRow = headerLayout && headerLayout->itemAt(1)
            ? qobject_cast<QHBoxLayout *>(headerLayout->itemAt(1)->layout()) : nullptr;
        if (topRow && titleRow && !detail->property("schemeBTitleMoved").toBool()) {
            auto *key = qobject_cast<QLabel *>(topRow->itemAt(2)->widget());
            auto *title = qobject_cast<QLabel *>(titleRow->itemAt(0)->widget());
            if (key && title) {
                detail->setProperty("schemeBKeyTask", !key->isHidden());
                for (int index = 0; index < 4; ++index) {
                    if (auto *badge = qobject_cast<QLabel *>(topRow->itemAt(index)->widget())) {
                        badge->setProperty("schemeBDetailHidden", true);
                        badge->hide();
                    }
                }
                title->setObjectName(QStringLiteral("schemeBDetailTitle"));
                title->setProperty("schemeBNoLabelReset", true);
                titleRow->removeWidget(title);
                topRow->insertWidget(0, title, 1);
                detail->setProperty("schemeBTitleMoved", true);
                connect(detail, &TaskDetailView::taskUpdated, this, [this]() {
                    scheduleRefresh();
                });
            }
        }
        if (auto *title = header->findChild<QLabel *>(QStringLiteral("schemeBDetailTitle"))) {
            const bool keyTask = detail->property("schemeBKeyTask").toBool();
            const QString prefix = QStringLiteral("【重点】");
            if (keyTask && !title->text().startsWith(prefix)) {
                title->setText(prefix + title->text());
            }
            title->setTextFormat(Qt::PlainText);
            title->setWordWrap(true);
            title->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            title->setMinimumWidth(0);
            QFont titleFont(QStringLiteral("MiSans"), 20, QFont::DemiBold);
            title->setFont(titleFont);
            title->setMaximumHeight(QFontMetrics(titleFont).lineSpacing() * 2 + 4);
            title->setStyleSheet(QStringLiteral(
                "color:%1; background:transparent; border:none; padding:0; "
                "font-family:MiSans; font-size:22px; font-weight:700;")
                .arg(keyTask ? QLatin1String(kRed) : QLatin1String(kText)));
            if (!detail->property("schemeBTitleWidthNormalized").toBool()) {
                title->setMaximumWidth(660);
                detail->setProperty("schemeBTitleWidthNormalized", true);
                detail->resize(760, detail->height());
                detail->adjustSize();
            }
        }

        for (QLabel *label : basic->findChildren<QLabel *>()) {
            if (label->text() == QStringLiteral("标签")) {
                label->setText(QStringLiteral("事项标签"));
            }
        }
        auto *folder = attachment->findChild<QPushButton *>(QStringLiteral("openFolderBtn"));
        if (folder && !folder->property("schemeBInBasicInfo").toBool()) {
            if (auto *attachmentLayout = qobject_cast<QVBoxLayout *>(attachment->layout())) {
                if (auto *titleRow = attachmentLayout->itemAt(0)->layout()) {
                    titleRow->removeWidget(folder);
                }
            }
            folder->setParent(basic);
            auto *footer = new QHBoxLayout();
            footer->setContentsMargins(0, 0, 0, 0);
            footer->addStretch();
            footer->addWidget(folder);
            if (auto *basicLayout = qobject_cast<QVBoxLayout *>(basic->layout())) {
                basicLayout->addLayout(footer);
            }
            folder->setProperty("schemeBInBasicInfo", true);
            folder->show();
        }
    }

    if (auto *focus = home->findChild<QFrame *>(QStringLiteral("FocusArea"))) {
        focus->setFixedHeight(230);
        if (auto *focusLayout = qobject_cast<QVBoxLayout *>(focus->layout())) {
            // Keep the empty-state message centered in the reserved three-row
            // area, but never give a one-row focus list an extra blank row.
            const auto rows = focus->findChildren<QFrame *>(QStringLiteral("FocusItem"));
            const bool empty = rows.isEmpty();
            focusLayout->setAlignment(Qt::AlignTop);
            focusLayout->setStretch(1, 0);
            focusLayout->setStretch(2, empty ? 1 : 0);
            if (auto *listItem = focusLayout->itemAt(1)) {
                if (listItem->layout()) {
                    listItem->layout()->setAlignment(Qt::AlignTop);
                }
            }
            if (auto *title = qobject_cast<QLabel *>(focusLayout->itemAt(0)->widget())) title->setFixedHeight(26);
            if (auto *title = qobject_cast<QLabel *>(focusLayout->itemAt(0)->widget())) {
                title->setText(QStringLiteral("今日专注  %1/3").arg(rows.size()));
            }
            for (QFrame *row : rows) row->setFixedHeight(48);
        }
    }

    const auto cards = home->findChildren<QFrame *>(QStringLiteral("Card"));
    for (QFrame *card : cards) {
        if (!card->property("schemeBCardBorder").toBool()) {
            card->setStyleSheet(QStringLiteral(
                "QFrame#Card { background:#FFFFFF; border:1px solid #DCE3EF; border-radius:12px; }"));
            card->setProperty("schemeBCardBorder", true);
        }
        QLabel *heading = nullptr;
        for (QLabel *label : card->findChildren<QLabel *>(QString(), Qt::FindDirectChildrenOnly)) {
            heading = label;
            break;
        }
        if (!heading) continue;

        if (heading->text() == QStringLiteral("进行中任务")) {
            if (!card->findChild<QWidget *>(QStringLiteral("schemeBColumnHeader"))) {
                auto *layout = qobject_cast<QVBoxLayout *>(card->layout());
                if (layout && layout->count() >= 2) {
                    auto *header = new QFrame(card);
                    header->setObjectName(QStringLiteral("schemeBColumnHeader"));
                    header->setFixedHeight(30);
                    auto *columns = new QHBoxLayout(header);
                    columns->setContentsMargins(0, 0, 0, 0);
                    columns->setSpacing(10);
                    auto addColumn = [columns, header](const QString &text, int width, bool expand) {
                        auto *label = new QLabel(text, header);
                        label->setObjectName(QStringLiteral("schemeBColumnHeaderLabel"));
                        label->setAlignment(Qt::AlignCenter);
                        label->setStyleSheet(QStringLiteral(
                            "color:#000000; background:transparent; font-size:13px; "
                            "padding:0; border:none; %1 /*schemeBText*/")
                            .arg(text == QStringLiteral("截止时间")
                                ? QString() : QStringLiteral("border-right:1px solid #DCE3EF;")));
                        label->setFixedHeight(30);
                        if (width) label->setFixedWidth(width);
                        columns->addWidget(label, expand ? 1 : 0);
                    };
                    addColumn(QStringLiteral("分类"), 50, false);
                    addColumn(QStringLiteral("任务名称"), 0, true);
                    addColumn(QStringLiteral("来源"), 104, false);
                    addColumn(QStringLiteral("截止时间"), 102, false);
                    layout->insertWidget(1, header);
                }
            }
            if (auto *layout = qobject_cast<QVBoxLayout *>(card->layout())) {
                if (layout->count() > 2) {
                    if (auto *list = layout->itemAt(2)->widget()) list->setFixedHeight(480);
                }
            }
            card->setFixedHeight(670);
            if (auto *pagination = card->findChild<PaginationWidget *>()) {
                if (!pagination->property("schemeBPageSizeSet").toBool()) {
                    pagination->setPageSize(10);
                    pagination->updateDisplay();
                    pagination->setProperty("schemeBPageSizeSet", true);
                }
                for (QLabel *label : card->findChildren<QLabel *>()) {
                    if (label->text().startsWith(QStringLiteral("显示 "))
                        && !pagination->isAncestorOf(label)) {
                        label->setText(pagination->infoText());
                    }
                }
            }
        } else if (heading->text() == QStringLiteral("本月概览")) {
            for (QFrame *cell : card->findChildren<QFrame *>(QString(), Qt::FindDirectChildrenOnly)) {
                if (qobject_cast<QLabel *>(cell)) continue;
                if (cell->objectName() != QStringLiteral("schemeBStatCell")) {
                    cell->setObjectName(QStringLiteral("schemeBStatCell"));
                    cell->setStyleSheet(QStringLiteral(
                        "QFrame#schemeBStatCell { background:#F3F5F8; border:none; border-radius:10px; }"));
                }
            }
        }
    }

    // V2 pages by creation time. Rebind each visible row to a page of the
    // deadline-sorted result, preserving the original row/star interactions.
    auto homeRows = home->findChildren<QFrame *>(QStringLiteral("InProgressItem"));
    if (!homeRows.isEmpty()) {
        auto *container = homeRows.first()->parentWidget();
        auto *listLayout = container ? qobject_cast<QVBoxLayout *>(container->layout()) : nullptr;
        if (listLayout) {
            while (homeRows.size() < 10) {
                auto *row = createSchemeBHomeRow(home, container);
                listLayout->insertWidget(homeRows.size(), row);
                homeRows.append(row);
            }
        }
        int page = 1;
        for (QFrame *card : cards) {
            if (card->findChild<QFrame *>(QStringLiteral("schemeBColumnHeader"))) {
                if (auto *pagination = card->findChild<PaginationWidget *>()) page = pagination->currentPage();
                break;
            }
        }
        QSqlQuery sorted;
        sorted.prepare(QStringLiteral(
            "SELECT id,title,category,source,is_key_task,deadline,is_today_focus FROM task "
            "WHERE status='进行中' ORDER BY "
            "CASE WHEN deadline IS NULL OR deadline='' THEN 1 ELSE 0 END, "
            "deadline ASC, id ASC LIMIT 10 OFFSET ?"));
        sorted.addBindValue((page - 1) * 10);
        if (sorted.exec()) {
            for (QFrame *row : homeRows) {
                if (!sorted.next()) { row->hide(); continue; }
                row->show();
                row->setProperty("taskId", sorted.value(0));
                const QString category = sorted.value(2).toString();
                if (!row->property("schemeBCategorySlotCentered").toBool()) {
                    if (auto *rowLayout = qobject_cast<QHBoxLayout *>(row->layout())) {
                        if (auto *badge = qobject_cast<QFrame *>(rowLayout->itemAt(0)->widget())) {
                            // The column is 50px; the badge itself stays at the task-list width.
                            auto *slot = new QWidget(row);
                            slot->setFixedWidth(50);
                            auto *slotLayout = new QHBoxLayout(slot);
                            slotLayout->setContentsMargins(0, 0, 0, 0);
                            rowLayout->removeWidget(badge);
                            slotLayout->addWidget(badge, 0, Qt::AlignCenter);
                            rowLayout->insertWidget(0, slot);
                            row->setProperty("schemeBCategorySlotCentered", true);
                        }
                    }
                }
                for (QFrame *tag : row->findChildren<QFrame *>()) {
                    if (qobject_cast<QLabel *>(tag)) continue;
                    if (auto *label = tag->findChild<QLabel *>()) {
                        label->setText(category);
                        label->setAlignment(Qt::AlignCenter);
                    }
                    tag->setFixedSize(32, 24);
                    if (tag->layout()) tag->layout()->setContentsMargins(0, 0, 0, 0);
                    tag->setStyleSheet(QStringLiteral(
                        "QFrame { background:%1; border:none; border-radius:6px; }")
                        .arg(categoryBackground(category)));
                }
                const auto labels = row->findChildren<QLabel *>(QString(), Qt::FindDirectChildrenOnly);
                if (labels.size() >= 3) {
                    labels.at(1)->setFixedWidth(104);
                    labels.at(2)->setFixedWidth(102);
                    labels.at(1)->setText(sorted.value(3).toString());
                    const QDate date = QDate::fromString(sorted.value(5).toString(), Qt::ISODate);
                    labels.at(2)->setText(date.isValid()
                        ? QStringLiteral("截止 %1").arg(date.toString(QStringLiteral("MM-dd"))) : QString());
                }
                for (QPushButton *button : row->findChildren<QPushButton *>()) {
                    if (!button->property("taskId").isValid()) continue;
                    const bool focused = sorted.value(6).toBool();
                    button->setProperty("taskId", sorted.value(0));
                    button->setProperty("focused", focused);
                    button->setText(focused ? QStringLiteral("★") : QStringLiteral("☆"));
                    button->setToolTip(focused ? QStringLiteral("移出今日专注") : QStringLiteral("加入今日专注"));
                    button->setStyleSheet(QStringLiteral(
                        "QPushButton { color:%1; font-size:18px; border:none; background:transparent; padding:0; }")
                        .arg(focused ? QString::fromLatin1(kYellow) : QStringLiteral("#CBD5E1")));
                }
            }
        }
    }

    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT title,category,is_key_task,deadline FROM task WHERE id=?"));
    for (QFrame *row : home->findChildren<QFrame *>()) {
        const QString name = row->objectName();
        if (name != QStringLiteral("InProgressItem") && name != QStringLiteral("FocusItem")) continue;
        if (name == QStringLiteral("FocusItem")) {
            row->setStyleSheet(QStringLiteral(
                "QFrame#FocusItem { background:transparent; border:none; "
                "border-bottom:1px solid #DCE5F3; border-radius:0; }"
                "QFrame#FocusItem:hover { background:#EAF1FF; }"));
        }
        query.bindValue(0, row->property("taskId"));
        if (!query.exec() || !query.next()) continue;

        const QString title = query.value(0).toString();
        const QString category = query.value(1).toString();
        const bool key = query.value(2).toBool();
        const QDate deadline = QDate::fromString(query.value(3).toString(), Qt::ISODate);

        for (QFrame *tag : row->findChildren<QFrame *>()) {
            for (QLabel *tagLabel : tag->findChildren<QLabel *>()) {
                if (tagLabel->text() == category) {
                    tagLabel->setStyleSheet(QStringLiteral("color:%1; background:transparent; border:none; "
                                                           "font-size:14px; font-weight:700; "
                                                           "padding-left:6px; padding-right:6px; /*schemeBText*/")
                                                .arg(categoryColor(category)));
                }
            }
        }

        const auto labels = row->findChildren<QLabel *>(QString(), Qt::FindDirectChildrenOnly);
        if (!labels.isEmpty()) {
            const QString displayTitle = key && !title.startsWith(QStringLiteral("【重点】"))
                ? QStringLiteral("【重点】") + title : title;
            labels.at(0)->setTextFormat(Qt::RichText);
            labels.at(0)->setText(QStringLiteral("<span style='color:%1'>%2</span>")
                                      .arg(key ? QLatin1String(kRed) : QLatin1String(kText),
                                           displayTitle.toHtmlEscaped()));
        }
        if (name == QStringLiteral("FocusItem") && labels.size() >= 2) {
            if (deadline.isValid()) {
                const int days = QDate::currentDate().daysTo(deadline);
                const QString color = days <= 0 ? QLatin1String(kRed)
                    : days < 3 ? QLatin1String(kYellow) : QLatin1String(kText);
                labels.last()->setText(QStringLiteral("截止 %1").arg(deadline.toString(QStringLiteral("MM-dd"))));
                labels.last()->setStyleSheet(QStringLiteral(
                    "color:%1; font-size:14px; background:transparent; border:none; "
                    "padding-left:6px; padding-right:6px; /*schemeBText*/").arg(color));
                labels.last()->show();
            } else {
                labels.last()->hide();
            }
        }
        if (name == QStringLiteral("InProgressItem") && labels.size() >= 3 && deadline.isValid()) {
            const int days = QDate::currentDate().daysTo(deadline);
            const QString color = days <= 0 ? QLatin1String(kRed)
                : days < 3 ? QLatin1String(kYellow) : QLatin1String(kText);
            labels.last()->setStyleSheet(QStringLiteral("color:%1; font-size:14px; font-weight:%2; "
                                                         "background:transparent; border:none; "
                                                         "padding-left:6px; padding-right:6px; /*schemeBText*/")
                                             .arg(color, days < 3 ? QStringLiteral("700") : QStringLiteral("400")));
        }
        // In every list, metadata is centered while the task title remains left aligned.
        if (name == QStringLiteral("InProgressItem")) {
            if (labels.size() >= 2) labels.at(1)->setAlignment(Qt::AlignCenter);
            if (labels.size() >= 3) labels.at(2)->setAlignment(Qt::AlignCenter);
        } else if (name == QStringLiteral("FocusItem")) {
            if (labels.size() >= 2) labels.at(1)->setAlignment(Qt::AlignCenter);
        }
    }

    for (QTableView *table : window_.findChildren<QTableView *>()) {
        if (qobject_cast<TaskItemDelegate *>(table->itemDelegate())
            && !dynamic_cast<SchemeBTaskDelegate *>(table->itemDelegate())) {
            table->setItemDelegate(new SchemeBTaskDelegate(table));
        }
    }
    QFont listFont(QStringLiteral("MiSans"));
    listFont.setPixelSize(14);
    if (auto *tasks = window_.findChild<TaskListView *>()) {
        if (auto *table = tasks->findChild<QTableView *>()) listFont = table->font();
    }
    if (auto *periodic = window_.findChild<PeriodicTaskView *>()) {
        if (auto *table = periodic->findChild<QTableWidget *>()) {
            table->setFont(listFont);
            table->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
            table->horizontalHeader()->setStyleSheet(QStringLiteral(
                "QHeaderView::section { background:#F5F8FC; color:#000000; font-family:'MiSans'; "
                "font-size:14px; font-weight:700; padding:10px 7px; border:none; "
                "border-right:1px solid #DCE3EF; }"));
            for (int row = 0; row < table->rowCount(); ++row) {
                for (int col = 0; col < table->columnCount(); ++col) {
                    if (auto *item = table->item(row, col)) {
                        item->setTextAlignment(col == 0
                            ? Qt::AlignLeft | Qt::AlignVCenter : Qt::AlignCenter);
                    }
                }
                if (auto *actions = table->cellWidget(row, 6)) {
                    if (auto *layout = qobject_cast<QHBoxLayout *>(actions->layout())) {
                        if (layout->count() && layout->itemAt(layout->count() - 1)->spacerItem()) {
                            delete layout->takeAt(layout->count() - 1);
                        }
                        layout->setAlignment(Qt::AlignCenter);
                    }
                }
            }
        }
    }
    if (auto *references = window_.findChild<ReferenceView *>()) {
        if (auto *table = references->findChild<QTableView *>()) {
            table->setFont(listFont);
            if (!dynamic_cast<SchemeBReferenceDelegate *>(table->itemDelegate())) {
                table->setItemDelegate(new SchemeBReferenceDelegate(table));
            }
        }
    }
    for (QAbstractScrollArea *area : window_.findChildren<QAbstractScrollArea *>()) {
        if (area->objectName() == QStringLiteral("sidebar")) continue;
        if (auto *view = qobject_cast<QAbstractItemView *>(area)) {
            view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
            view->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
        }
        area->verticalScrollBar()->setSingleStep(24);
        area->horizontalScrollBar()->setSingleStep(24);
    }
    if (auto *settings = window_.findChild<SettingsView *>()) {
        for (const QString &name : {QStringLiteral("workDirCard"),
                                    QStringLiteral("dictMgmtCard"),
                                    QStringLiteral("exportCard"),
                                    QStringLiteral("dataMgmtCard"),
                                    QStringLiteral("aboutCard")}) {
            if (auto *card = settings->findChild<QFrame *>(name)) {
                if (!card->property("schemeBCardBorder").toBool()) {
                    card->setStyleSheet(QStringLiteral(
                        "QFrame#%1 { background:#FFFFFF; border:1px solid #DCE3EF; "
                        "border-radius:12px; }").arg(name));
                    card->setProperty("schemeBCardBorder", true);
                }
            }
        }
    }
    for (QHeaderView *header : window_.findChildren<QHeaderView *>()) {
        if (header->orientation() != Qt::Horizontal
            || header->styleSheet().contains(QStringLiteral("/*schemeBHeaderDividers*/"))) continue;
        header->setStyleSheet(header->styleSheet() + QStringLiteral(
            " QHeaderView::section { border-right:1px solid #DCE3EF; }"
            " /*schemeBHeaderDividers*/"));
    }

    auto resizeCreateDialog = [](QWidget *dialog, int descriptionHeight, int filesHeight) {
        if (!dialog || dialog->property("schemeBSectionsSized").toBool()) return;
        if (auto *description = dialog->findChild<QTextEdit *>()) {
            description->setFixedHeight(descriptionHeight);
        }
        if (auto *files = dialog->findChild<DragDropArea *>()) {
            files->setFixedHeight(filesHeight);
            if (auto *hint = files->findChild<QLabel *>()) hint->setFixedHeight(filesHeight);
            if (auto *scroll = files->findChild<QScrollArea *>()) scroll->setFixedHeight(filesHeight);
        }
        dialog->setProperty("schemeBSectionsSized", true);
    };
    for (TaskCreateDialog *dialog : window_.findChildren<TaskCreateDialog *>()) {
        resizeCreateDialog(dialog, 200, 80);
    }
    for (ReferenceCreateDialog *dialog : window_.findChildren<ReferenceCreateDialog *>()) {
        resizeCreateDialog(dialog, 200, 96);
    }
    refreshing_ = false;
}
