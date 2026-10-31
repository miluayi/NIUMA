/**
 * @file StyledMessageBox.cpp
 * @brief 软件UI风格的确认弹窗实现
 */
#include "StyledMessageBox.h"
#include "RoundedCornerFilter.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainterPath>
#include <QPushButton>
#include <QRegion>
#include <QResizeEvent>
#include <QVBoxLayout>

#include "Spacing.h"
#include "WindowDragger.h"

StyledMessageBox::StyledMessageBox(QWidget *parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    installEventFilter(new RoundedCornerFilter(12, this));
    setModal(true);
}

void StyledMessageBox::resizeEvent(QResizeEvent *event)
{
    QDialog::resizeEvent(event);
}

int StyledMessageBox::question(QWidget *parent, const QString &title, const QString &text,
                                const QString &yesText, const QString &noText)
{
    StyledMessageBox dlg(parent);
    dlg.setupUI(title, text, false, true, yesText, noText);
    return dlg.exec() == QDialog::Accepted ? Yes : No;
}

int StyledMessageBox::choice(QWidget *parent, const QString &title, const QString &text,
                             const QString &primaryText, const QString &secondaryText)
{
    StyledMessageBox dlg(parent);
    dlg.setupUI(title, text, false, true, primaryText, secondaryText);
    QObject::disconnect(dlg.noBtn_, nullptr, &dlg, nullptr);
    QObject::disconnect(dlg.yesBtn_, nullptr, &dlg, nullptr);
    QObject::connect(dlg.noBtn_, &QPushButton::clicked, &dlg, [&dlg]() { dlg.done(2); });
    QObject::connect(dlg.yesBtn_, &QPushButton::clicked, &dlg, [&dlg]() { dlg.done(1); });
    return dlg.exec();
}

void StyledMessageBox::information(QWidget *parent, const QString &title, const QString &text)
{
    StyledMessageBox dlg(parent);
    dlg.setupUI(title, text, false, false, QStringLiteral("我知道了"), QString());
    dlg.exec();
}

void StyledMessageBox::warning(QWidget *parent, const QString &title, const QString &text)
{
    StyledMessageBox dlg(parent);
    dlg.setupUI(title, text, true, false, QStringLiteral("我知道了"), QString());
    dlg.exec();
}

void StyledMessageBox::setupUI(const QString &title, const QString &text, bool isWarning,
                                bool twoButtons, const QString &yesText, const QString &noText)
{
    setMinimumWidth(420);
    setMaximumWidth(520);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ---- 标题栏 ----
    QFrame *header = new QFrame(this);
    header->setObjectName("msgHeader");
    header->setStyleSheet("#msgHeader { background: #FFFFFF; border-bottom: 1px solid #F1F5F9; }");
    header->installEventFilter(new WindowDragger(this));
    QHBoxLayout *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(Spacing::XL, Spacing::LG, Spacing::XL, Spacing::LG);

    QLabel *titleLabel = new QLabel(title, header);
    titleLabel->setFont(QFont(QStringLiteral("MiSans"), 16, QFont::Normal));
    titleLabel->setStyleSheet(QString("color: %1; border: none;").arg(isWarning ? "#DC2626" : "#0F172A"));
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();

    QPushButton *closeBtn = new QPushButton(QStringLiteral("\u2715"), header);  // ✕
    closeBtn->setFixedSize(28, 28);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setFocusPolicy(Qt::NoFocus);
    closeBtn->setStyleSheet(
        "QPushButton { border: none; background: transparent; font-size: 14px; "
        "color: #94A3B8; border-radius: 4px; }"
        "QPushButton:hover { color: #0F172A; background: #F5F7FA; }");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::reject);
    headerLayout->addWidget(closeBtn);

    mainLayout->addWidget(header);

    // ---- 内容区 ----
    QFrame *body = new QFrame(this);
    body->setStyleSheet("QFrame { background: #FFFFFF; }");
    QVBoxLayout *bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(Spacing::XL, Spacing::XL, Spacing::XL, Spacing::XL);
    bodyLayout->setSpacing(Spacing::LG);

    textLabel_ = new QLabel(text, body);
    textLabel_->setWordWrap(true);
    textLabel_->setStyleSheet("font-size: 15px; color: #0F172A; border: none; background: transparent;");
    bodyLayout->addWidget(textLabel_);

    mainLayout->addWidget(body);

    // ---- 底部按钮栏 ----
    QFrame *footer = new QFrame(this);
    footer->setStyleSheet("QFrame { background: #FFFFFF; border-top: 1px solid #F1F5F9; }");
    QHBoxLayout *footerLayout = new QHBoxLayout(footer);
    footerLayout->setContentsMargins(Spacing::XL, Spacing::MD, Spacing::XL, Spacing::MD);
    footerLayout->addStretch();

    if (twoButtons) {
        noBtn_ = new QPushButton(noText, footer);
        noBtn_->setCursor(Qt::PointingHandCursor);
        noBtn_->setFocusPolicy(Qt::NoFocus);
        noBtn_->setMinimumWidth(96);
        noBtn_->setStyleSheet(
            "QPushButton { background: #FFFFFF; color: #475569; border: 1px solid #E2E8F0; "
            "border-radius: 8px; padding: 8px 20px; font-size: 14px; }"
            "QPushButton:hover { background: #F5F7FA; border-color: #CBD5E1; }");
        connect(noBtn_, &QPushButton::clicked, this, &QDialog::reject);
        footerLayout->addWidget(noBtn_);
    }

    yesBtn_ = new QPushButton(yesText, footer);
    yesBtn_->setCursor(Qt::PointingHandCursor);
    yesBtn_->setFocusPolicy(Qt::NoFocus);
    yesBtn_->setMinimumWidth(96);
    yesBtn_->setDefault(true);
    // 确认弹窗用蓝色按钮，警告弹窗用红色按钮
    if (isWarning) {
        yesBtn_->setStyleSheet(
            "QPushButton { background: #DC2626; color: #FFFFFF; border: 1px solid #DC2626; "
            "border-radius: 8px; padding: 8px 20px; font-size: 14px; font-weight: 700; }"
            "QPushButton:hover { background: #B91C1C; }");
    } else {
        yesBtn_->setStyleSheet(
            "QPushButton { background: #2463EB; color: #FFFFFF; border: 1px solid #2463EB; "
            "border-radius: 8px; padding: 8px 20px; font-size: 14px; font-weight: 700; }"
            "QPushButton:hover { background: #1D4ED8; }");
    }
    connect(yesBtn_, &QPushButton::clicked, this, &QDialog::accept);
    footerLayout->addWidget(yesBtn_);

    mainLayout->addWidget(footer);
}
