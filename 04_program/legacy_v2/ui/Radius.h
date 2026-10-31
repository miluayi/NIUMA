/**
 * @file Radius.h
 * @brief 圆角常量定义（6/8/12/16）
 *
 * 用于 QSS border-radius 或 QPainter 绘制圆角矩形。
 *
 * 依赖：无
 * 文档：qt_implementation_guide.md 1.3、coding.md 7.4
 */
#pragma once

/**
 * @brief 圆角常量命名空间
 *
 * 单位：像素（px）
 * - SM = 6   小圆角（标签/小按钮）
 * - MD = 8   中圆角（按钮/输入框）
 * - LG = 12  大圆角（卡片）
 * - XL = 16  超大圆角（弹窗/主窗口）
 */
namespace Radius {
    const int SM = 6;
    const int MD = 8;
    const int LG = 12;
    const int XL = 16;
}
