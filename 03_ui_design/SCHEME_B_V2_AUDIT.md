# 方案 B v2 读取审计

审计日期：2026-09-01

## 结论

此前的方案 B **没有读取 v2 的实际页面实现**。它读取的是 v3 的
`forms/MainWindow.ui` / `forms/DialogGallery.ui`，仅额外切换了 v2 颜色、无边框标题栏和部分尺寸。因此“看起来接近 v2”不等于“复用了 v2 UI 设计”。你提供的截图与 v2 原型一致，而此前 B 的页面结构仍然是 v3 的静态近似表单。

## 证据对照

| 关注点 | v2 实际实现 | 此前方案 B | 结论 |
| --- | --- | --- | --- |
| 主窗体 | `MainWindow.cpp` 创建 `TitleBar` → 内容区 HBox（侧栏 + QStackedWidget）→ 状态栏 | `PreviewWindow` 加载 v3 `MainWindow.ui`，运行时重新组织控件 | 不是 v2 MainWindow 的直接 UI |
| 首页今日专注 | `HomeView::buildFocusArea()`：渐变 `FocusArea`、动态任务行、空态提示 | `.ui` 静态 `focusItemOne/focusItemTwo` | 结构和状态模型不同 |
| 首页进行中 | `HomeView::buildInProgressArea()`：固定 400px、240px 列表容器、5 行×48px、自定义分页 | `QTableView#homeTaskTable` | 视觉相似但控件结构不同 |
| 周期提醒 | 动态提醒行，每行“创建任务 / 忽略本次” | 两个静态 QLabel + 一个“管理周期任务”按钮 | 功能入口缺失 |
| 本月概览 | 4 个统计 cell，24px 数值、13px 标签、网格等宽 | 4 个静态 QLabel 网格 | 数据呈现层级不同 |
| 任务管理 | `TaskListView` 动态筛选、表格、分页和空/加载/错误状态 | v3 `.ui` 的 `filterPanel` + `taskStateStack` | 状态外壳近似，实际控件链不同 |
| 统计 | `StatsView` 动态指标卡、分类/来源分布、`TrendChart`、`HeatmapWidget` | 静态进度条和占位文本 | 缺少 v2 图表实现 |
| 设置中心 | 工作目录、三列字典、导出设置、数据管理、初始化、关于共 6 组 | 只有工作目录、字典、数据、关于 4 组 | 缺少导出设置和初始化 |
| 任务详情 | 6 个完整 section：基本信息、说明、附件、备注、历史、成果 | 任务标签、基础信息、说明、历史文本 | 缺少附件/备注/成果交互 |
| 弹窗体系 | 每个 Dialog 独立 C++ 类，独立 header/body/footer、字段和按钮 | 一个 `DialogGallery.ui` 共享壳 + 9 个简化页面 | 不是 v2 全链弹窗 |

## v2 设计基线

- 技术栈：Qt 5.15 Widgets + C++17 + CMake + SQLite（`03_ui_design/qt_implementation_guide.md`）。
- 主窗体：无边框；1440×900；最小 940×580；TitleBar 52px；侧栏 220px；状态栏 36px（`04_program/ui/MainWindow.cpp:108-203`）。
- 页面：所有 `*View` 自己构建 UI，外层由 `QScrollArea` 承载，页面内容边距 24/20，区块间距 16。
- 首页：`HomeView.cpp:66-81` 定义页面顺序；`199-246` 定义进行中固定高度与分页；`443-567` 定义任务行横向结构。
- 颜色/字体/间距：`03_ui_design/qt_implementation_guide.md` 与 `index.html:10-45` 为唯一来源。
- 弹窗：任务创建/编辑宽 720px，任务详情宽 760px，周期任务 500px，参考资料 560/600px，成果 560px，扫描导入 900px；均为无边框模态并启用 size grip。

## 修正方向

方案 B 下一步不能继续在 v3 的简化 `.ui` 上叠加 QSS。正确路径是：

1. 以 v2 的 `MainWindow.cpp`、`HomeView.cpp`、各 `*View.cpp` 和各 `*Dialog.cpp` 为 UI 结构源，建立 B 专属的模拟组件层。
2. 组件层使用假数据替代 DAO/Service，但保留 v2 的控件层级、按钮位置、表格列、空/加载/错误态和弹窗 section。
3. B 的模拟程序与 A 分离；每轮只启动 B 预览，不再把 v3 `MainWindow.ui` 当作 B 的页面源。
4. 完成首页、任务管理、周期任务、参考资料、年度成果、统计、回收站、设置和全部弹窗后，再逐页与用户提供的 v2 截图及源码复核。

## 复核结果（2026-09-01）

上述“此前方案 B”已被源码级目标替换。`NiumaV2SourcePreview` 直接编译 v2 的
`MainWindow`、全部 `*View`、全部 `*Dialog` 和自定义 widgets，仅在入口处使用确定性的
演示数据库种子；`run_preview.ps1 -SchemeB` 已切换到该目标。首页渲染已与 v2 参考图的
标题栏、侧栏、焦点空态、任务行、分页、周期提醒和本月概览结构一致。
