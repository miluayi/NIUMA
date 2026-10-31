/**
 * @file ToastWidget.h
 * @brief 轻提示组件（T8.11）
 *
 * 无边框圆角提示条，3 秒自动消失，支持淡入淡出动画。
 * 四种类型：Success（绿）/ Error（红）/ Info（灰）/ Warning（黄）
 * Unicode 符号（✓ ✕ ℹ ⚠），不使用 SVG/emoji。
 *
 * 用法：
 *   ToastWidget::show(mainWindow, ToastWidget::Success, "操作成功");
 *
 * 特性：
 *   - 自动定位到父窗口底部中央（距底部 48px）
 *   - 多个 Toast 垂直堆叠显示
 *   - 不抢焦点、不阻塞用户操作（WA_TransparentForMouseEvents）
 *   - QPropertyAnimation 透明度动画 0.0→1.0→1.0→0.0
 *
 * 依赖：Qt5 Widgets
 * 文档：task.md T8.11、qt_guide Prompt 6、2.4
 */
#pragma once

#include <QColor>
#include <QList>
#include <QWidget>

class QLabel;
class QPropertyAnimation;
class QGraphicsOpacityEffect;
class MainWindow;

class ToastWidget : public QWidget {
    Q_OBJECT

public:
    /// 提示类型
    enum Type {
        Success,  ///< 成功（绿 #16A34A，符号 ✓）
        Error,    ///< 错误（红 #DC2626，符号 ✕）
        Info,     ///< 信息（灰 #475569，符号 ℹ）
        Warning   ///< 警告（黄 #EAB308，符号 ⚠）
    };

    explicit ToastWidget(QWidget *parent = nullptr);
    ~ToastWidget() override;

    /**
     * @brief 显示一条 Toast 提示
     * @param parent 父窗口（MainWindow），用于定位
     * @param type 提示类型
     * @param message 提示消息文本
     *
     * 自动定位到父窗口底部中央，距底部 48px。
     * 多个 Toast 垂直堆叠显示，3 秒后自动消失。
     */
    static void show(MainWindow *parent, Type type, const QString &message);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QColor bgColor_;
    QPropertyAnimation *animation_;
    QGraphicsOpacityEffect *opacityEffect_;

    /// 当前活跃的 Toast 列表（用于堆叠定位）
    static QList<ToastWidget *> activeToasts_;

    /// 设置内容（符号 + 消息 + 样式 + 动画）
    void setupContent(Type type, const QString &message);

    /// 重新定位所有活跃 Toast（底部居中堆叠）
    static void repositionToasts(QWidget *parent);
};
