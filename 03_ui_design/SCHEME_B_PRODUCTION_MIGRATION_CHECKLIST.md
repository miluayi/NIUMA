# 方案 B → v3 正式程序迁移清单（历史迁移基线）

状态：2026-09-23 历史盘点资料。正式程序已在 `04_program/legacy_v2` 完成方案 B 迁移和本地打包；本表保留迁移前对应关系，不代表当前缺口。方案 B 是唯一 UI 基线，A/C 不进入范围。当前计划见 `../02_process/plan/SCHEME_B_V3_DEVELOPMENT_PLAN.md`。

## 路线与边界

- **已完成“复用 v2 Qt 5.15 Widgets/C++17 源码并迁移方案 B 样式”**：正式源码位于 `../04_program/legacy_v2`，预览源码位于 `../04_program/ui_preview`；保留 v2 页面、弹窗、模型与业务行为，不恢复方案 A/C、不从截图重建页面。
- 当前 `04_program/` 仅有 README，正式迁移未开始。原型的 `CMakeLists.txt:10-42` 从相邻 `05_workspace_v2/04_program` 收集 `src/ui/database/service/models/utils` 的 C++，排除原 `src/main.cpp`，另用预览入口；它不是可独立部署的 v3 工程。
- 历史预览入口 `../04_program/ui_preview/v2_source_preview_main.cpp:91-114` 在 `06_test/data/ui-preview-debug/v2-preview-data-b/database.db` 初始化并填充种子任务；正式启动所需的维护行为由 `../04_program/legacy_v2` 正式入口承接，预览库不得连接或复制生产数据。
- 方案 B 四个源文件的 SHA-256 已与 `FINAL_UI_HANDOFF.md` 指纹核对一致（2026-09-22）；未来 v2 源码变化仍须重新比对，因为 CMake 按 glob 直接取其当前源码。

## 主窗体与主页面对应（八页均有定稿截图，不等于功能验收）

| 迁移项 / v2 结构与功能入口 | 方案 B 定稿对应 | 验收要点与缺口 |
| --- | --- | --- |
| 外壳、导航、状态栏：`ui/MainWindow.cpp:139-246,888-998`，`ui/widgets/TitleBar.cpp` | `SchemeBStyler.cpp:511-730,979-987`，`scheme_b.qss:3-23` | 184px 侧栏、70px 标题栏、前进/后退/刷新提示、搜索、新建、圆角白工作区；验证 1440×900、940×580、DPI、窗口拖动/缩放、键盘焦点；旧侧栏第 7 行 spacer 与设置第 8 行的索引契约不可丢。 |
| 首页 `ui/HomeView.cpp`：专注、进行中、周期提醒、本月概览；`MainWindow.cpp:405-435` | `SchemeBStyler.cpp:1231-1482`；`page-0.png`、`home-bottom-page*.png`、`focus-*.png` | 专注最多 3 项且占位高度固定；进行中截止排序、10 条分页；提醒的创建/忽略与滚动位置；空态和日期临界。 |
| 任务管理 `ui/TaskListView.cpp`、`models/TaskTableModel.cpp`、`ui/widgets/TaskItemDelegate.cpp`；`MainWindow.cpp:297-322` | `SchemeBStyler.cpp:1484-1494` 的委托替换；`page-1.png` | 筛选/搜索、行打开详情、分页、空/无结果/加载/错误状态、列对齐、横向及嵌套滚轮；各状态尚无完整定稿截图。 |
| 周期任务 `ui/PeriodicTaskView.cpp:53-426`；`MainWindow.cpp:457-481,704-755` | `SchemeBStyler.cpp:1495-1520`；`page-2.png` | 模板新建/编辑、启用对号、删除确认、首页创建任务/忽略提醒；核对表格列与响应式宽度。 |
| 参考资料 `ui/ReferenceView.cpp`；`MainWindow.cpp:323-348,607-651` | `SchemeBStyler.cpp:1521-1528`；`page-3.png` | 来源/标签筛选、目录、新建、行打开详情、附件与空态；横向滚轮。 |
| 年度成果 `ui/AchievementView.cpp`；`MainWindow.cpp:349-372,662-702` | 通用 QSS/修饰；`page-4.png` | 年份、统计卡、成果表格、Word 导出与系统保存对话框；导出链路未被视觉截图验证。 |
| 统计 `ui/StatsView.cpp`、`ui/widgets/TrendChart.cpp`、`HeatmapWidget.cpp`；`MainWindow.cpp:437-456` | 通用 QSS/修饰；`page-5.png` | 三标签、指标、分布、趋势、热力图及空数据；未见各标签/状态分别截图。 |
| 回收站 `ui/RecycleBinView.cpp:71-726`；`MainWindow.cpp:483-507` | 通用 QSS/修饰；`page-6.png` | 勾选、批量恢复、单项/批量永久删除、清空及确认、分页、空态；危险操作仅隔离数据验证。 |
| 设置中心 `ui/SettingsView.cpp:183-604,691-1050`；`MainWindow.cpp:373-404,760-865` | `SchemeBStyler.cpp:1538-1565`；`page-7.png` | 工作目录、来源/标签/办理人字典（页面内新增/删除）、导出配置、备份/恢复、扫描导入、初始化、关于；列表嵌套滚轮与所有破坏性确认。 |

以上 v2 路径均相对于 `D:/project/05_workspace_v2/04_program/`；方案 B 路径均相对于本目录。页面索引和截图对应 `MainWindow.cpp:170-197,230-239` 与 `v2_source_preview_main.cpp:562-577`。

## 详情、新建、编辑、确认及系统选择框（逐项迁移）

| 类型 / v2 类与触发点 | 定稿覆盖 | 必须保留和补验 |
| --- | --- | --- |
| 新建任务 `ui/TaskCreateDialog.cpp`；`MainWindow.cpp:543-553` | `new-task.png`、`new-task-calendar.png`；`SchemeBStyler.cpp:1586-1588` | 双列表单、分类/来源/标签/办理人、截止日期、重点与子目录、说明/附件、校验/保存/取消；截图仅打开态与日期弹层。 |
| 编辑任务 `ui/TaskEditDialog.cpp`；`MainWindow.cpp:555-604` | 无独立定稿截图 | 原值回填、编辑/保存、附件、重点/截止日期、详情返回链；需补截图及交互测试。 |
| 任务详情 `ui/TaskDetailView.cpp:246-719,967-1119`；`MainWindow.cpp:555-590` | `task-detail*.png`；`SchemeBStyler.cpp:1149-1229` | 长标题、`【重点】` 与事项标签区分，基础信息、说明、附件/打开目录、备注、历史、成果；完成/重新打开、编辑、删除及二次确认。截图不覆盖所有动作。 |
| 新建/编辑周期模板，同一 `ui/PeriodicTaskCreateDialog.cpp:88-139,139-698`；`MainWindow.cpp:704-729` | 无独立定稿截图 | 周期类型联动、分类/办理人/提醒/启用、回填/保存；单独验新建与编辑。 |
| 新建资料 `ui/ReferenceCreateDialog.cpp`；`MainWindow.cpp:607-619` | `new-reference.png`；`SchemeBStyler.cpp:1589-1591` | 编号/名称、来源/标签、说明/附件、工作目录、校验；截图不等于提交与文件操作通过。 |
| 编辑资料 `ui/ReferenceEditDialog.cpp`；`MainWindow.cpp:621-651` | 无独立定稿截图 | 回填、附件、目录和保存；补验详情→编辑→返回。 |
| 资料详情 `ui/ReferenceDetailDialog.cpp:62-345`；`MainWindow.cpp:621-651` | 无独立定稿截图 | 基本信息、说明、目录、编辑、不可恢复删除确认。 |
| 登记成果 `ui/AchievementDialog.cpp`；`TaskDetailView.cpp:697-705,967-1005` | 无独立定稿截图 | 工作内容/数量/金额/说明、年度/重点勾选、完成任务时的触发与跳过。 |
| 扫描导入 `ui/ScanImportDialog.cpp`；`MainWindow.cpp:857-865`、`SettingsView.cpp:1010-1013` | 无独立定稿截图 | 扫描提示、预览表格、选择/确认导入及结果反馈；仅隔离目录操作。 |
| 通用确认/提示 `ui/widgets/StyledMessageBox.cpp` | 无独立定稿截图 | 任务删除、资料删除、周期模板删除、回收站单项/批量永久删除与清空、数据恢复覆盖、软件初始化，以及各类成功/失败/校验提示。触发点分别见 `TaskDetailView.cpp:1029-1044`、`ReferenceDetailDialog.cpp:314-329`、`PeriodicTaskView.cpp:394-420`、`RecycleBinView.cpp:599-651,708-728`、`MainWindow.cpp:826-855`、`SettingsView.cpp:1015-1049`。危险语义和按钮缺省选择逐一验。 |
| 系统文件/目录选择器 `QFileDialog` | 无方案 B 专属截图 | 新建/编辑附件的 `DragDropArea.cpp:213-220`、年度导出 `MainWindow.cpp:662-700`、工作目录 `MainWindow.cpp:760-792`、恢复目录 `MainWindow.cpp:826-855`、首次启动 `src/main.cpp:68-89`；遵循系统对话框，不伪称已由 QSS 定稿。 |

## 共用控件、代码归位与验收门槛

1. 基础样式来自 v2 `resources/styles/neutral_modern.qss` 再追加 `styles/scheme_b.qss`，且 `SchemeBStyler.cpp:531-553,989-1147` 改写字体、内联样式、日期弹层、下拉箭头/圆角和勾选对号；迁移不能只拷贝 B 的短 QSS。令牌需集中，处理内联样式优先级；正式程序内置 MiSans 字体资源。
2. 运行时修饰器不仅是样式：`SchemeBStyler.cpp:602-695` 新增导航/搜索/新建，`:783-865` 接管滚轮，`:1149-1229` 改详情层级，`:1231-1482` 改首页布局和数据。正式实现应在所属 Widgets 页面/控件中归位，不把预览 `SchemeBStyler` 当长期业务入口；保留可观察行为。
3. 特别是 `SchemeBStyler.cpp:1329-1410` 直接 `QSqlQuery` 按截止排序并重建 10 行。正式版应由相应查询/应用服务提供排序与分页，UI 只消费结果，避免违反 v3 的 presentation 禁止 DAO/SQL 边界，也不能依赖预览种子库。
4. 日期、非日期下拉、勾选、表头、语义色、分类标签、焦点、Tab/Enter/Esc/Ctrl+N/Ctrl+F/Ctrl+S/F5/Delete、滚动与缩放逐项回归。v2 源码搜索发现多个表单的 Enter 处理，但未发现主窗体全局 `QShortcut`/`setShortcut`；`SCHEME_B_V2_BASELINE.md` 的快捷键清单是契约，不能当作已由原型自动验证。列表的 loading/empty/error 也按实际实现逐态检查，不能以 `page-*.png` 代替。
5. 视觉基线见 `gallery/scheme-b-final-2026-09-22/README.md`：八页、新建任务/资料、任务详情及控件状态有自动截图；其余编辑/确认/导入、键盘全路径、不同 DPI、目标机部署仍是未覆盖项。先在隔离测试数据做功能与视觉双回归，再考虑安装包与回退。

## 迁移前必须协调的决策

此处列出的 Designer `.ui`/AUTOUIC、原生窗框与深色侧栏要求属于迁移前历史约束，已由方案 B 与 Widgets 路线取代。当前目录职责、构建及本地交付说明以仓库根 `README.md`、`../02_process/task.md` 和 `../04_program/README.md` 为准。

建议实施批次：①工程入口和测试隔离；②外壳及通用控件；③八页；④详情与上表全部弹窗/确认；⑤隔离数据的功能及视觉回归；⑥本地打包。每批均核对 v2 功能契约、B 视觉基线和实际截图缺口。
