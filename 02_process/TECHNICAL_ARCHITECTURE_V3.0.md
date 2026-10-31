# NIUMA V3 UI 技术架构

## 决策摘要

本轮采用 **C++17 + Qt 5.15.18 Widgets + CMake 显式源码清单 + 方案 B QSS/Widgets 修饰**。v2 明确要求 Win7 SP1，因此 UI 重构不同时引入 Qt 5→6、编译器、部署和平台最低版本变化。方案 B 原型已直接复用 v2 Widgets 源码；正式迁移沿用该路线，不采用 Qt Designer `.ui` 重录。

当前开发环境可用 `C:\msys64\mingw64\bin\designer-qt5.exe` 与 `uic 5.15.18`。正式发布工具链仍需冻结为一条经过 Win7 验证的链；当前 MinGW 构建缓存不能自动证明 Win7 兼容。

若用户明确放弃 Win7，另建 ADR 评估 Qt 6 + Windows 10/11。Qt5/Qt6 双版本只可作为短期迁移桥，不作为长期发布架构。

## 目标依赖图

```text
方案 B C++/QSS + v2 Widgets 源码快照
           │
           ▼
presentation (View wrapper / Presenter / Model / custom widgets)
           │  DTO + use case
           ▼
application (facade / ports / transaction & compensation semantics)
           │
      ┌────┴────┐
      ▼         ▼
   domain   infrastructure
             SQLite / filesystem / backup / export
```

- `domain` 不依赖 Qt Widgets、SQL 或文件系统。
- S01–S12 的受控 `legacy_v2/ui` 可保留已登记的 v2 DAO、`DatabaseManager`、`QSqlQuery` 依赖；不得新增。S13–S15 后所有正式 UI 源码均不包含这些依赖或文件迁移编排。
- `application` 提供页面所需的窄接口与稳定 DTO，并定义跨数据库/文件系统操作的失败与补偿语义。
- `infrastructure` 实现 application 声明的端口。

## Widgets 源码契约

以下旧 `.ui` 契约由 ADR-005 取代，保留本节标题变更记录以避免将早期表单作为当前实现依据。当前 Widgets C++ 负责：

- 静态 widget tree、layout、stretch、size policy、object name、Tab 顺序、toolTip、accessible name 与四态容器；
- model、数据绑定、信号槽、校验、错误映射和动态列表内容；
- `HeatmapWidget`、`TrendChart`、delegate、拖放区、日期/下拉/勾选等复杂控件，以及窄屏/DPI 的必要运行时调整；
- 方案 B 的主窗体、八页、详情与全部弹窗必须保留 v2 的结构/功能语义，并按 `SCHEME_B_PRODUCTION_MIGRATION_CHECKLIST.md` 逐项迁移。

禁止在正式 target 引入预览入口、预览种子数据库或早期 `03_ui_design/forms/*.ui`。`SchemeBStyler` 可在早期用于画面等价，但每条包含业务/数据处理的修饰须在所属域解耦前归位。

## 设计令牌与 QSS

令牌分三层：

1. Primitive：颜色、字体、4px 间距阶梯、4/6/8px 圆角、动画时长。
2. Semantic：surface、text、accent、focus、danger、success、warning、divider。
3. Component：button、input、nav、table、panel、status、toast。

Qt 5 QSS 没有变量，因此 `03_ui_design/DESIGN_SYSTEM.md` 是令牌规范，v2 `neutral_modern.qss` 与 `scheme_b.qss` 是当前实现基线。页面 C++ 优先设置 object name 或如 `role=primary`、`state=error` 的动态属性；S12 前将预览修饰器中散落的通用色值/尺寸逐步收敛。

## 数据状态组件

每个数据页都使用四态容器：

- loading：进度与明确文案；
- content：实际模型视图；
- empty：区分数据库为空与筛选无结果，并给出下一步；
- error：用户可理解的错误与重试按钮。

同一时刻只有一个状态可见；重复刷新需合并或禁用，不允许多次并发覆盖较新的结果。

## 键盘与无障碍

- Tab 顺序在 Widgets 初始化中显式设置，所有主要操作可达；不以 `Qt::NoFocus` 隐藏控件。
- `QTextEdit` Enter 换行；Ctrl+Enter 才可提交。
- `QDialogButtonBox` 定义默认/取消语义；Esc 取消，Ctrl+S 保存。
- Ctrl+N 新建、Ctrl+F 聚焦搜索、F5 刷新；Delete 只在可恢复或经确认的上下文使用。
- 表格提供行选择、键盘打开详情和可读的状态文本；自绘统计提供表格/文字替代。
- 颜色不是唯一状态信号；焦点边框在浅色和深色表面上均清晰可见。

## CMake 目标结构

生产迁移后使用显式 source list 和分层 target：

```text
niuma_domain
niuma_application
niuma_infrastructure
niuma_presentation
NIUMA
```

- 使用显式 C++/资源 source list；不以 `file(GLOB)` 追踪 v2 源码，也不为本轮页面开启 `AUTOUIC`。
- `.qrc` 直接列入 target 并使用 `AUTORCC`，不再同时调用 `qt5_add_resources`。
- `include(CTest)` + QtTest；避免 v2 五个测试目标重复复制相同 source/link/include 块。
- CMakePresets 固定编译器、Qt 根目录、Debug/Release、测试和打包配置。

## 视觉验证

方案 B 预览不代表正式程序像素或功能完全一致。每阶段必须先制定并执行 E2E 方案：对浏览器适配表面使用 Playwright MCP；对原生 Qt Widgets 使用可用的 LazyTesting/桌面自动化 MCP。随后调用可用 `/simplify`，并重跑受影响构建、测试和 E2E。没有可调用的相应 MCP/技能时，将其记录为阻断，不把手工检查冒充为 MCP E2E。视觉回归仍须在 100%、125%、150%、200% DPI 和 1440×900、1280×720、940×580 下检查字体、QSS、自绘控件、滚动与截断。
