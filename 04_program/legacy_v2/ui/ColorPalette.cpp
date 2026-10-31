/**
 * @file ColorPalette.cpp
 * @brief ColorPalette 静态成员定义（QColor 需运行时构造）
 *
 * 依赖：ui/ColorPalette.h
 * 文档：qt_implementation_guide.md 1.1、coding.md 7.2
 */
#include "ColorPalette.h"

// ---- 中性色 ----
const QColor ColorPalette::bg("#F0F2F5");
const QColor ColorPalette::surface("#FFFFFF");
const QColor ColorPalette::text("#0F172A");
const QColor ColorPalette::textSecondary("#475569");
const QColor ColorPalette::textMuted("#94A3B8");
const QColor ColorPalette::border("#E2E8F0");

// ---- 品牌色 ----
const QColor ColorPalette::primary("#2463EB");
const QColor ColorPalette::primaryHover("#1D4ED8");
const QColor ColorPalette::primaryLight("#EFF6FF");

// ---- 状态色 ----
const QColor ColorPalette::success("#16A34A");
const QColor ColorPalette::successBg("#F0FDF4");
const QColor ColorPalette::warning("#EAB308");
const QColor ColorPalette::warningBg("#FEFCE8");
const QColor ColorPalette::danger("#DC2626");
const QColor ColorPalette::dangerBg("#FEF2F2");

// ---- 功能色 ----
const QColor ColorPalette::fire("#F97316");
const QColor ColorPalette::star("#F59E0B");
const QColor ColorPalette::starInactive("#CBD5E1");

// ---- 界面扩展色 ----
const QColor ColorPalette::sidebarSelectedBg("#EFF4FF");
const QColor ColorPalette::hoverBg("#F5F7FA");
const QColor ColorPalette::lightDivider("#F1F5F9");
const QColor ColorPalette::inProgressBg("#FDE68A");
const QColor ColorPalette::completedBg("#BBF7D0");
const QColor ColorPalette::headerBg("#F5F7FA");
