#pragma once
#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QKeyEvent>
#include <QListWidget>
#include <QMenu>
#include <QMouseEvent>
#include <QPersistentModelIndex>
#include <QPointer>
#include <QStyle>
#include <QTableView>
#include "CopyTextLayout.h"

// Highlight the original text in place; never create an editor.
template<class View> class CopyableItemView final : public View {
public:
    explicit CopyableItemView(QWidget *parent=nullptr) : View(parent) {
        this->setEditTriggers(QAbstractItemView::NoEditTriggers);
    }
    ~CopyableItemView() override {
        // QListWidget clears its model in the base destructor, after our state is destroyed.
        if(observedModel_) QObject::disconnect(observedModel_.data(),nullptr,this,nullptr);
    }
    QString selectedText() const {
        return selected_.isValid() ? layout(selected_).text.mid(qMin(anchor_,caret_),qAbs(caret_-anchor_)) : QString();
    }
protected:
    void mousePressEvent(QMouseEvent *event) override {
        if (event->button()==Qt::LeftButton) {
            observeModel();
            clearTextSelection();
            selected_=this->indexAt(event->pos());
            origin_=event->pos();
            if (selected_.isValid()) anchor_=caret_=layout(selected_).hit(origin_);
            dragging_=false;
            modifiers_=event->modifiers();
            this->setFocus(Qt::MouseFocusReason);
            event->accept();
            return;
        }
        View::mousePressEvent(event);
    }
    void mouseMoveEvent(QMouseEvent *event) override {
        if ((event->buttons() & Qt::LeftButton) && selected_.isValid()
            && (dragging_ || layout(selected_).hit(event->pos())!=anchor_
                || (event->pos()-origin_).manhattanLength()>=QApplication::startDragDistance())) {
            dragging_=true;
            this->setState(View::NoState);
            caret_=layout(selected_).hit(event->pos());
            this->viewport()->update();
            event->accept();
            return;
        }
        View::mouseMoveEvent(event);
    }
    void mouseReleaseEvent(QMouseEvent *event) override {
        if (dragging_ && event->button()==Qt::LeftButton) {
            caret_=layout(selected_).hit(event->pos());
            this->setState(View::NoState);
            this->viewport()->update();
            event->accept();
            return;
        }
        if(event->button()==Qt::LeftButton) {
            QMouseEvent press(QEvent::MouseButtonPress,origin_,this->viewport()->mapToGlobal(origin_),
                Qt::LeftButton,Qt::LeftButton,modifiers_);
            View::mousePressEvent(&press);
        }
        View::mouseReleaseEvent(event);
    }
    void paintEvent(QPaintEvent *event) override {
        View::paintEvent(event);
        if (!selectedText().isEmpty()) {
            QPainter painter(this->viewport());
            layout(selected_).highlight(painter,qMin(anchor_,caret_),qAbs(caret_-anchor_));
        }
    }
    void keyPressEvent(QKeyEvent *event) override {
        if (event->matches(QKeySequence::Copy) && (selected_.isValid() || this->currentIndex().isValid())) {
            QApplication::clipboard()->setText(selectedText().isEmpty()
                ? (selected_.isValid() ? selected_.data() : this->currentIndex().data()).toString() : selectedText());
            event->accept();return;
        }
        View::keyPressEvent(event);
    }
    void contextMenuEvent(QContextMenuEvent *event) override {
        if (!selectedText().isEmpty()) {
            QMenu menu(this);
            auto *copy=menu.addAction(QStringLiteral("复制"));
            copy->setShortcut(QKeySequence::Copy);
            if (menu.exec(event->globalPos())==copy) QApplication::clipboard()->setText(selectedText());
            return;
        }
        View::contextMenuEvent(event);
    }
    void scrollContentsBy(int x,int y) override { clearTextSelection();View::scrollContentsBy(x,y); }
    void resizeEvent(QResizeEvent *event) override { clearTextSelection();View::resizeEvent(event); }
private:
    CopyTextLayout layout(const QModelIndex &index) const {
        QStyleOptionViewItem option=this->viewOptions();
        option.rect=this->visualRect(index);
        option.widget=this;
        if (auto *provider=dynamic_cast<const CopyTextProvider *>(this->itemDelegate(index)))
            return provider->copyTextLayout(option,index);
        option.text=index.data().toString();
        option.features|=QStyleOptionViewItem::HasDisplay;
        const QVariant alignment=index.data(Qt::TextAlignmentRole);
        if (alignment.isValid()) option.displayAlignment=Qt::Alignment(alignment.toInt());
        QRect rect=this->style()->subElementRect(QStyle::SE_ItemViewItemText,&option,this);
        // QCommonStyle applies this inner text margin after SE_ItemViewItemText.
        const int margin=this->style()->pixelMetric(QStyle::PM_FocusFrameHMargin,nullptr,this)+1;
        rect.adjust(margin,0,-margin,0);
        return {option.fontMetrics.elidedText(option.text,option.textElideMode,rect.width()),
                option.font,rect,option.displayAlignment,false,0,true};
    }
    void clearTextSelection() { selected_=QModelIndex();anchor_=caret_=0;this->viewport()->update(); }
    void observeModel() {
        if (observedModel_==this->model()) return;
        observedModel_=this->model();
        if (observedModel_) {
            QObject::connect(observedModel_,&QAbstractItemModel::modelAboutToBeReset,this,[this](){clearTextSelection();});
            QObject::connect(observedModel_,&QAbstractItemModel::rowsAboutToBeRemoved,this,[this](){clearTextSelection();});
            QObject::connect(observedModel_,&QAbstractItemModel::dataChanged,this,[this](){clearTextSelection();});
            QObject::connect(observedModel_,&QAbstractItemModel::layoutAboutToBeChanged,this,[this](){clearTextSelection();});
        }
    }
    QPointer<QAbstractItemModel> observedModel_;
    QPersistentModelIndex selected_;
    QPoint origin_;
    int anchor_=0,caret_=0;
    bool dragging_=false;
    Qt::KeyboardModifiers modifiers_=Qt::NoModifier;
};
using CopyableTableView=CopyableItemView<QTableView>;
using CopyableListWidget=CopyableItemView<QListWidget>;
