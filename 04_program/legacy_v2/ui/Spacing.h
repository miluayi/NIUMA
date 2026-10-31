/**
 * @file Spacing.h
 * @brief 间距常量定义（4/8/12/16/20/24/32）
 *
 * 用于 Qt 布局的 setSpacing() 与 setContentsMargins()。
 *
 * 依赖：无
 * 文档：qt_implementation_guide.md 1.2、coding.md 7.3
 */
#pragma once

/**
 * @brief 间距常量命名空间
 *
 * 单位：像素（px）
 * - XS  = 4   小间距（图标与文字）
 * - SM  = 8   行内间距
 * - MD  = 12  卡片内边距
 * - LG  = 16  区块间距
 * - XL  = 20  卡片间距
 * - XXL = 24  大区块间距
 * - XXXL= 32  页面间距
 */
namespace Spacing {
    const int XS = 4;
    const int SM = 8;
    const int MD = 12;
    const int LG = 16;
    const int XL = 20;
    const int XXL = 24;
    const int XXXL = 32;
}
