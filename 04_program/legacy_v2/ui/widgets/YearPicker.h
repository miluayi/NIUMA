#pragma once

#include <QComboBox>
#include <QDate>
#include <QEvent>
#include <QGridLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QRegion>
#include <QPushButton>
#include <QScreen>
#include <QStyle>
#include <QVBoxLayout>
#include "RoundedCornerFilter.h"

// 与现有日期控件一致的外观；弹窗按十年网格选择年度。
// QComboBox 仅保存年度值，供页面复用既有的筛选状态接口。
class YearPicker final : public QComboBox {
public:
    explicit YearPicker(QWidget *parent = nullptr) : QComboBox(parent)
    {
        setAccessibleName(QStringLiteral("年度选择器"));
        setCursor(Qt::PointingHandCursor);
        setStyleSheet(QStringLiteral(
            "QComboBox { background:#FFFFFF; border:1px solid #DCE3EF; border-radius:7px;"
            "color:#172033; padding:8px 30px 8px 28px; font-size:14px; }"
            "QComboBox:focus { border-color:#2B72EF; }"
            "QComboBox::drop-down { border:none; width:30px; }"));
    }

    void showPopup() override
    {
        if (!popup_) buildPopup();
        const int year = currentData().toInt() > 0 ? currentData().toInt() : QDate::currentDate().year();
        decade_ = year / 10 * 10;
        updateYears();
        QPoint position = mapToGlobal(QPoint(0, height() + 4));
        if (QScreen *screen = QGuiApplication::screenAt(position)) {
            const QRect available = screen->availableGeometry();
            position.setX(qBound(available.left(), position.x(), qMax(available.left(), available.right() - popup_->width() + 1)));
            if (position.y() + popup_->height() > available.bottom() + 1)
                position.setY(mapToGlobal(QPoint(0, 0)).y() - popup_->height() - 4);
            position.setY(qMax(available.top(), position.y()));
        }
        popup_->move(position);
        popup_->show();
        popup_->raise();
        for (QPushButton *button : cells_) {
            if (button->property("year").toInt() == year) button->setFocus();
        }
    }

    void hidePopup() override
    {
        if (popup_) popup_->hide();
        QComboBox::hidePopup();
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (watched == popup_ && event->type() == QEvent::Hide) QComboBox::hidePopup();
        return QComboBox::eventFilter(watched, event);
    }

    void paintEvent(QPaintEvent *event) override
    {
        QComboBox::paintEvent(event);
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.translate(9, (height() - 16) / 2);
        painter.setPen(QPen(QColor("#667085"), 1.3));
        painter.drawRoundedRect(QRectF(1.5, 3.5, 13, 11), 1.5, 1.5);
        painter.drawLine(QPointF(1.5, 7), QPointF(14.5, 7));
        painter.drawLine(QPointF(5, 1.5), QPointF(5, 5));
        painter.drawLine(QPointF(11, 1.5), QPointF(11, 5));
    }

private:
    QWidget *popup_ = nullptr;
    QLabel *heading_ = nullptr;
    QPushButton *previous_ = nullptr;
    QPushButton *next_ = nullptr;
    QList<QPushButton *> cells_;
    int decade_ = 2020;

    void chooseYear(int year)
    {
        int index = findData(year);
        if (index < 0) {
            addItem(QString::number(year), year);
            index = count() - 1;
        }
        const bool unchanged = currentIndex() == index;
        hidePopup();
        setCurrentIndex(index);
        // 重新选择同一年也应用全年范围，丢弃未提交的手动日期。
        if (unchanged) emit currentIndexChanged(index);
    }

    void buildPopup()
    {
        popup_ = new QWidget(this, Qt::Popup | Qt::FramelessWindowHint);
        popup_->installEventFilter(this);
        popup_->setObjectName("appUiYearPopup");
        popup_->setFixedSize(300, 280);
        popup_->installEventFilter(new RoundedCornerFilter(12, popup_));
        popup_->setStyleSheet(QStringLiteral(
            "QWidget#appUiYearPopup { background:#FFFFFF; border:1px solid #D9DFE9; border-radius:7px; }"
            "QPushButton { background:transparent; border:none; border-radius:6px;"
            "color:#344054; font-size:14px; padding:4px; }"
            "QPushButton:hover, QPushButton:focus { background:#EEF3FB; color:#2164E5; }"
            "QPushButton#appUiYearCell[adjacent='true'] { color:#98A2B3; }"
            "QPushButton#appUiYearCell[selected='true'] { background:#2B72EF; color:#FFFFFF; }"
            "QPushButton:disabled { color:#C8CDD7; }"
            "QPushButton#appUiYearAll { color:#2164E5; border-top:1px solid #E3E7EF; border-radius:0; }"));
        auto *root = new QVBoxLayout(popup_);
        root->setContentsMargins(12, 10, 12, 8);
        root->setSpacing(8);
        auto *navigation = new QHBoxLayout();
        previous_ = new QPushButton(QStringLiteral("«"), popup_);
        previous_->setObjectName("appUiYearPrevious");
        previous_->setAccessibleName(QStringLiteral("上一十年"));
        next_ = new QPushButton(QStringLiteral("»"), popup_);
        next_->setObjectName("appUiYearNext");
        next_->setAccessibleName(QStringLiteral("下一十年"));
        heading_ = new QLabel(popup_);
        heading_->setObjectName("appUiYearHeading");
        heading_->setProperty("appUiNoLabelReset", true);
        heading_->setAlignment(Qt::AlignCenter);
        heading_->setStyleSheet("color:#344054; font-size:15px; border:none; background:transparent;");
        for (QPushButton *button : {previous_, next_}) {
            button->setFixedSize(32, 32);
            button->setCursor(Qt::PointingHandCursor);
        }
        navigation->addWidget(previous_);
        navigation->addWidget(heading_, 1);
        navigation->addWidget(next_);
        root->addLayout(navigation);
        connect(previous_, &QPushButton::clicked, popup_, [this]() { decade_ -= 10; updateYears(); });
        connect(next_, &QPushButton::clicked, popup_, [this]() { decade_ += 10; updateYears(); });
        auto *grid = new QGridLayout();
        grid->setSpacing(6);
        for (int index = 0; index < 12; ++index) {
            auto *button = new QPushButton(popup_);
            button->setObjectName("appUiYearCell");
            button->setCursor(Qt::PointingHandCursor);
            button->setMinimumHeight(36);
            cells_.append(button);
            grid->addWidget(button, index / 3, index % 3);
            connect(button, &QPushButton::clicked, popup_, [this, button]() {
                chooseYear(button->property("year").toInt());
            });
        }
        root->addLayout(grid, 1);
        auto *all = new QPushButton(QStringLiteral("全部年度"), popup_);
        all->setObjectName("appUiYearAll");
        all->setCursor(Qt::PointingHandCursor);
        root->addWidget(all);
        connect(all, &QPushButton::clicked, popup_, [this]() { chooseYear(0); });
    }

    void updateYears()
    {
        heading_->setText(QStringLiteral("%1 — %2").arg(decade_).arg(decade_ + 9));
        previous_->setEnabled(decade_ > 1900);
        next_->setEnabled(decade_ < 9990);
        for (int index = 0; index < cells_.size(); ++index) {
            QPushButton *button = cells_.at(index);
            const int year = decade_ - 1 + index;
            button->setText(QString::number(year));
            button->setProperty("year", year);
            button->setProperty("selected", year == currentData().toInt());
            button->setProperty("adjacent", year < decade_ || year > decade_ + 9);
            button->setEnabled(year >= 1900 && year <= 9999);
            button->style()->unpolish(button);
            button->style()->polish(button);
        }
    }
};
