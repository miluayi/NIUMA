#pragma once

#include <QApplication>
#include <QClipboard>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QLineEdit>
#include <QListWidget>
#include <QPointer>
#include <QMouseEvent>
#include <QPersistentModelIndex>
#include <QTableView>

// Preserve row activation on a click; a drag selects read-only cell text.
template <class View>
class CopyableItemView final : public View
{
public:
    using View::viewport;
    using View::model;
    using View::indexAt;
    using View::visualRect;
    using View::font;
    using View::setState;
    using View::NoState;
    using View::currentIndex;
    explicit CopyableItemView(QWidget *parent = nullptr) : View(parent)
    {
        this->setEditTriggers(QAbstractItemView::NoEditTriggers);
        editor_ = new QLineEdit(viewport());
        editor_->setObjectName(QStringLiteral("cellCopyText"));
        editor_->setReadOnly(true);
        editor_->setFrame(false);
        editor_->setStyleSheet(QStringLiteral(
            "QLineEdit { background:#FFFFFF; color:#0F172A; border:none; border-radius:0;"
            " padding:0 7px; selection-background-color:#2463EB; selection-color:white; }"));
        editor_->hide();
        editor_->installEventFilter(this);
    }
protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        if (observedModel_ != model()) {
            observedModel_ = model();
            if (model()) {
                QObject::connect(model(), &QAbstractItemModel::modelAboutToBeReset, this, [this]() { editor_->hide(); });
                QObject::connect(model(), &QAbstractItemModel::rowsAboutToBeRemoved, this, [this]() { editor_->hide(); });
                QObject::connect(model(), &QAbstractItemModel::dataChanged, this, [this]() { editor_->hide(); });
                QObject::connect(model(), &QAbstractItemModel::layoutAboutToBeChanged, this, [this]() { editor_->hide(); });
            }
        }
        editor_->hide();
        pressed_ = event->button() == Qt::LeftButton ? indexAt(event->pos()) : QModelIndex();
        origin_ = event->pos();
        dragging_ = false;
        View::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        if ((event->buttons() & Qt::LeftButton) && pressed_.isValid()
            && (event->pos()-origin_).manhattanLength() >= QApplication::startDragDistance()) {
            if (!dragging_) {
                dragging_ = true;
                setState(NoState);
                editor_->setFont(font());
                editor_->setGeometry(visualRect(pressed_));
                editor_->setText(pressed_.data(Qt::DisplayRole).toString());
                editor_->show();
                editor_->setFocus(Qt::MouseFocusReason);
                const QPoint start = origin_-editor_->pos();
                QMouseEvent press(QEvent::MouseButtonPress, start, Qt::LeftButton,
                                  Qt::LeftButton, event->modifiers());
                QApplication::sendEvent(editor_, &press);
                editor_->grabMouse();
            }
            QMouseEvent move(QEvent::MouseMove, event->pos()-editor_->pos(), Qt::NoButton,
                             event->buttons(), event->modifiers());
            QApplication::sendEvent(editor_, &move);
            event->accept();
            return;
        }
        View::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent *event) override
    {
        if (dragging_) {
            QMouseEvent release(QEvent::MouseButtonRelease, event->pos()-editor_->pos(),
                                event->button(), event->buttons(), event->modifiers());
            QApplication::sendEvent(editor_, &release);
            pressed_ = QModelIndex();
            event->accept();
            return;
        }
        pressed_ = QModelIndex();
        View::mouseReleaseEvent(event);
    }

    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (watched == editor_) {
            if (event->type() == QEvent::MouseButtonRelease) {
                editor_->releaseMouse();
                pressed_ = QModelIndex();
            }
            if (event->type() == QEvent::FocusOut
                && static_cast<QFocusEvent *>(event)->reason() != Qt::PopupFocusReason)
                editor_->hide();
        }
        return View::eventFilter(watched, event);
    }

    void scrollContentsBy(int dx, int dy) override
    {
        editor_->hide();
        View::scrollContentsBy(dx, dy);
    }

    void resizeEvent(QResizeEvent *event) override
    {
        editor_->hide();
        View::resizeEvent(event);
    }

    void keyPressEvent(QKeyEvent *event) override
    {
        if (event->matches(QKeySequence::Copy) && currentIndex().isValid()) {
            QApplication::clipboard()->setText(currentIndex().data().toString());
            event->accept();
            return;
        }
        View::keyPressEvent(event);
    }

private:
    QPointer<QAbstractItemModel> observedModel_;
    QLineEdit *editor_ = nullptr;
    QPersistentModelIndex pressed_;
    QPoint origin_;
    bool dragging_ = false;
};

using CopyableTableView = CopyableItemView<QTableView>;
using CopyableListWidget = CopyableItemView<QListWidget>;
