/**
 * @file ColorPalette.h
 * @brief 色值常量定义（项目唯一色值来源）
 *
 * 严格对齐 qt_implementation_guide.md 第 1.1 节与 coding.md 第 7.2 节。
 * 所有 UI 色值必须从本结构体获取，禁止散落硬编码。
 *
 * 禁用色值：#FAFBFC（已替换为 #F5F7FA）
 *
 * 依赖：Qt5 Gui（QColor）
 * 文档：qt_implementation_guide.md 1.1、coding.md 7.2
 */
#pragma once

#include <QColor>

/**
 * @brief 色值常量集合，所有成员为 static const QColor
 *
 * 色值分类：
 * - 中性色：bg / surface / text / textSecondary / textMuted / border
 * - 品牌色：primary / primaryHover / primaryLight
 * - 状态色：success / successBg / warning / warningBg / danger / dangerBg
 * - 功能色：fire / star / starInactive
 * - 界面扩展色：sidebarSelectedBg / hoverBg / lightDivider / inProgressBg / completedBg / headerBg
 */
struct ColorPalette {
    // ---- 中性色 ----
    static const QColor bg;              // #F0F2F5 主背景
    static const QColor surface;         // #FFFFFF 卡片/表面背景
    static const QColor text;            // #0F172A 主文字
    static const QColor textSecondary;   // #475569 次要文字
    static const QColor textMuted;       // #94A3B8 弱化文字/占位符
    static const QColor border;          // #E2E8F0 分隔线/边框

    // ---- 品牌色 ----
    static const QColor primary;         // #2463EB 主品牌色（按钮/链接）
    static const QColor primaryHover;    // #1D4ED8 主色悬停
    static const QColor primaryLight;    // #EFF6FF 主色浅底（分类标签背景）

    // ---- 状态色 ----
    static const QColor success;         // #16A34A 成功（完成按钮/Toast 成功）
    static const QColor successBg;       // #F0FDF4 成功浅底
    static const QColor warning;         // #EAB308 警告（Toast 警告）
    static const QColor warningBg;       // #FEFCE8 警告浅底
    static const QColor danger;          // #DC2626 危险（删除/截止标红/Toast 错误）
    static const QColor dangerBg;        // #FEF2F2 危险浅底

    // ---- 功能色 ----
    static const QColor fire;            // #F97316 暖色强调色
    static const QColor star;            // #F59E0B 今日专注五角星点亮色
    static const QColor starInactive;    // #CBD5E1 今日专注五角星未点亮色

    // ---- 界面扩展色 ----
    static const QColor sidebarSelectedBg;  // #EFF4FF 侧边栏选中背景
    static const QColor hoverBg;            // #F5F7FA 悬停背景（#FAFBFC 禁用）
    static const QColor lightDivider;       // #F1F5F9 浅分隔线
    static const QColor inProgressBg;       // #FDE68A 进行中状态背景
    static const QColor completedBg;        // #BBF7D0 已完成状态背景
    static const QColor headerBg;           // #F5F7FA 表头背景
};
