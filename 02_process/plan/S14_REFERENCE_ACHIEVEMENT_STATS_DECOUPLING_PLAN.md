# S14 参考资料、年度成果与统计解耦计划

## 范围与约束

- 唯一 UI 方案为方案 B；本阶段不调整布局、色彩、控件或交互。
- 程序使用 `04_program/legacy_v2` 的 Qt 5 Widgets 源码；验证仅使用 `v3-staging-debug` fixture。
- 不读取、复制或操作真实 `Data`、数据库、工作目录或附件。
- 桌面无活动窗口；系统选择器、物理键盘、OS DPI 不标记为已测。原生控件以 Qt 合同、`QWidget::grab` 和 `QT_SCALE_FACTOR` 合同替代。

## S14-D 年度成果

- [x] `AchievementView` 通过 `AchievementUseCase` 获得年度统计、完成任务数和带任务编号的列表 DTO；按原 50 条/页在视图呈现。
- [x] `AchievementDialog` 通过用例登记成果；金额仍以分存储，保留自动完成任务登记流程。
- [x] 年度 Word 导出的设置读取和生成调用由成果用例协调；路径选择、进度和结果提示仍由现有 UI 处理。
- [x] 成果 UI 及图表类静态扫描无 DAO、`DatabaseManager`、SQL 命中。
- [x] `niuma_v3_achievement_contract`、`niuma_v3_stats_contract` 均通过；完整构建成功，完整 CTest 19/19 通过。
- [x] `/simplify` 复核：对用例列表 DTO 做可读性整理并移除过期 DAO 依赖；没有改变金额、年度/重点标记、分页、完成触发或导出语义。

## S14-C 参考资料

- [x] 参考资料查询/命令用例已接入；参考资料列表、创建、编辑、详情 UI 的直接 DAO/SQL 依赖已按前序记录清零。
- [x] 本轮完整 CTest 19/19 通过，其中包含 `niuma_v3_reference_contract`；该结果复核了前序迁移。

## S14-E 统计页面

- [x] 新增 `StatsUseCase`，封装总览、分类、来源、月趋势与热力图读取。
- [x] 统计数据结构移至 `models/Statistics.h`；`StatsView`、`TrendChart`、`HeatmapWidget` 不再依赖 DAO 头文件。
- [x] 统计合同及成果合同通过；完整构建成功，完整 CTest 19/19 通过。
- [x] 既有 StatsView 合同验证空/有数据总览、分类/来源分布、月趋势和热力图维度切换均通过 application 用例读取。

## S14-F 收口门

- [x] 年度成果、统计页面、趋势图与热力图相关 UI 文件静态扫描无 DAO、`DatabaseManager`、SQL 依赖。
- [x] 年度导出仍调用同一 `WordExport::exportAnnualReport`，默认配置和设置配置选择逻辑保留。
- [x] 最终构建及相关/完整 CTest 的实际结果已记录。
- [x] 成果合同直接走 `AchievementUseCase::exportAnnualReport`，使用 staging 设置中的配置验证成功导出、内容和失败路径。
- [x] 更新过渡债表和任务状态；成果/统计静态扫描无 DAO、`DatabaseManager`、SQL 命中。
- [x] `/simplify` 对复用、清晰度、边界和查询成本完成复核；无额外高置信行为保持简化项。

## 本轮证据边界

CTest 覆盖 staging fixture 上的直接成果登记、完成触发登记、金额精度、年度列表与统计显示、Word 文档内容/失败路径、StatsView 空/非空汇总与分布、趋势/热力图切换，以及 125/150/200% 的 Qt DPI 契约。未声称系统文件选择器、物理键盘、真实 OS DPI 或真实 Data 已验证。
