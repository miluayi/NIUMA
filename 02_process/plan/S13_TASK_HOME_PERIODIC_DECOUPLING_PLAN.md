# S13 · 任务、首页与周期任务解耦计划

日期：2026-09-23（Asia/Shanghai）  
状态：S13-A 架构盘点、S13-B/C 任务域、S13-D 首页边界均已完成；周期模板尚待迁移。  
目标：使任务、首页和周期任务的正式 UI 源码不再直接包含 DAO、`DatabaseManager` 或 `QSqlQuery`；保留 v2 业务规则、SQLite schema、状态字符串、文件与回收站契约。

## 范围和边界

| 包含 | 不包含 |
| --- | --- |
| `TaskListView`、`TaskCreateDialog`、`TaskEditDialog`、`TaskDetailView`、`HomeView`、`PeriodicTaskView`、`PeriodicTaskCreateDialog` 的查询与命令编排解耦 | 参考资料/成果/统计（S14）；回收站/设置/启动和 `MainWindow` 的维护编排（S15） |
| application DTO、查询/命令 use case、现有 service/DAO 的 infrastructure 适配 | schema 变更、真实 `Data`、异步线程模型、加载/重试 UX（R05 留后续的可观察状态设计） |

## 当前调用图（已观察）

```text
TaskListView
  filter controls -> TaskDao(DatabaseManager) -> TaskTableModel/pagination
  Delete shortcut -> TaskService -> soft delete -> MainWindow refresh

Task forms/detail
  dictionaries -> SourceDictDao/TagDictDao/HandlerDao(DatabaseManager)
  create/update/state/remarks -> TaskService
  history/achievement -> OperationLogDao/AchievementDao(DatabaseManager)

HomeView
  today focus -> FocusService
  in-progress/month stats -> TaskDao(DatabaseManager)
  reminders -> PeriodicTaskService
  star toggle -> FocusService

Periodic views/forms
  list/delete/enable/template -> PeriodicTaskService
  source/tag options -> QSqlQuery(DatabaseManager)
```

证据：`TaskListView.cpp:490,501,549,697`；`TaskCreateDialog.cpp:357,366,375,403,486`；`TaskEditDialog.cpp:375,384,393,406`；`TaskDetailView.cpp:867,905,994`；`HomeView.cpp:447,513,625,743,819`；`PeriodicTaskView.cpp:253,416,439`；`PeriodicTaskCreateDialog.cpp:464,478,492,576`。

## 目标 application 接口

接口按“UI 需要的数据和动作”划分，DTO 继续使用既有领域模型 `Task`、`PeriodicTask`、`TaskFilter`，不做无收益的模型复制。

| application use case | DTO / 操作 | 主要 UI 使用者 |
| --- | --- | --- |
| `TaskQueryUseCase` | `TaskListPage { QList<Task> tasks; int total; }`、`TaskFormOptions { sources, tags, handlers }` | 列表、创建、编辑 |
| `TaskDetailUseCase` | 任务、备注、历史、成果读取；完成、取消完成、软删除、备注操作；结果沿用 `TaskResult` | 详情 |
| `HomeDashboardUseCase` | `HomeDashboard { focusTasks, inProgressPage, reminders, monthlyStats }` | 首页四个展示区 |
| `HomeCommandUseCase` | 设置专注、从周期提醒创建任务、忽略提醒 | 首页星标与提醒按钮 |
| `PeriodicTaskUseCase` | 模板列表/详情、表单选项、创建/更新/启用/删除、提醒操作 | 周期页和模板弹窗 |

application 实现可以在本阶段内部复用 `TaskService`、`FocusService`、`PeriodicTaskService` 与现有 DAO；DAO 只移动到 application/infrastructure 边界，UI 不得新建 DAO 或 `QSqlQuery`。

## 分片与验收

### S13-B · 任务查询与表单选项

1. 建立 `application/task/TaskQueryUseCase` 与 DTO；先迁移 `TaskListView` 的列表/总数/来源/标签和两个任务表单的字典读取。
2. 保留现有控件、筛选、分页、`Ctrl+S`、Delete 确认、信号和 objectName。
3. 在 `NiumaV3TaskListContractTest`、`NiumaV3TaskDialogContractTest` 添加/保留成功、空/筛选无结果、取消、重复 Delete 和表单保存断言。
4. 退出：三文件 UI 静态搜索不含 `database/`、`DatabaseManager`、`QSqlQuery`。

### S13-C · 任务详情与命令边界

1. 建立 `TaskDetailUseCase`；将详情的日志、成果、备注和生命周期编排移出 UI。
2. `TaskService` 保持原有状态迁移与文件语义，use case 只组织 UI 所需读取/命令。
3. 复跑任务、成果、回收站合同，确认删除/恢复、完成成果登记、备注链不变。
4. 退出：四个任务 UI 文件清零直接持久化依赖。**已完成：**`TaskDetailView` 经静态扫描清零；专项 4/4、完整 CTest 19/19 通过。测试记录见 `S13C_TASK_DETAIL_USE_CASE_TEST_PLAN.md`。

### S13-D · 首页仪表盘

1. 建立 `HomeDashboardUseCase`/`HomeCommandUseCase`，一次返回首页四区数据；保留截止排序、十条分页、三项专注和月度口径。
2. 避免 UI 以多次 DAO 查询拼装统计；DTO 只承载渲染所需字段。
3. 复跑首页、任务和周期合同，覆盖专注上限、滚动位置、分页与提醒创建/忽略。
4. 退出：首页 UI 已清零直接持久化依赖；HomeDashboardUseCase 提供四区 DTO，HomeCommandUseCase 委托星标/周期提醒命令；专项 2/2、完整 CTest 19/19 通过。见 S13D_HOME_DASHBOARD_TEST_PLAN.md。

### S13-E · 周期模板

1. 建立 `PeriodicTaskUseCase`，迁移模板列表、选项、CRUD、启用与提醒动作；表单 source/tag 选项不再写 SQL。
2. 保留 `PeriodicTaskService` 的日期计算、模板→任务映射和最后提醒日期更新。
3. 复跑周期与首页合同，覆盖双模式、Esc、取消/确认删除、启用往返、创建任务和忽略。
4. 退出：两个周期 UI 文件清零直接持久化依赖。

### S13-F · 静态门与回归

1. 静态检查 S13 UI 文件：不包含 `database/`、`DatabaseManager`、`QSqlQuery`。
2. 完整构建、19 项 CTest、Qt 原生 100/125/150/200% 截图回归；不将其等同于 OS DPI 或物理键盘测试。
3. 每个代码分片后执行 `/simplify`，仅接受保持行为不变的简化。

## 风险与防护

| 风险 | 防护 |
| --- | --- |
| 拆分时改变 v2 生命周期或状态字符串 | use case 委托现有 `TaskService`/`PeriodicTaskService`，先迁查询、再迁命令；现有 fixture 合同作为回归基线。 |
| 首页查询聚合改变分页/排序口径 | `HomeDashboardUseCase` 显式接受当前页、固定 pageSize=10、`sortByDeadline=true`；首页合同保留首末行和总数断言。 |
| DTO 重复造成额外层级 | 首阶段直接复用领域 `Task`/`PeriodicTask`；只为列表页、首页仪表盘和表单选项定义聚合 DTO。 |
| UI 与 application 循环依赖 | application 不包含 Qt Widgets/UI 头文件；UI 只依赖 application public headers。 |
| 无活动桌面影响 E2E | 继续使用隔离 Qt 合同、`QWidget::grab()` 和 `QT_SCALE_FACTOR`；原生 OS 行为仍明确留作发布人工门。 |

## S13-A 验证（盘点时）

- 使用 `rg` 对过渡债表与七个 S13 UI 文件执行依赖扫描，结果与上方证据一致。
- 对照 `home_contract_test.cpp`、`task_list_contract_test.cpp`、`task_dialog_contract_test.cpp`、`periodic_task_contract_test.cpp` 的 fixture 行为，确认现有自动回归可作为迁移门。
- 本切片未修改 C++、CMake 或真实数据；故不新增构建结果。下一切片开始前先写 S13-B 可执行测试增量。

- S13-B：`TaskQueryUseCase`、`TaskFormUseCase` 已迁移列表、创建和编辑 UI 的数据边界；完整 CTest 19/19 通过。
- S13-C：`TaskDetailUseCase` 已迁移详情读取与命令协调；`TaskDetailView` 静态扫描清零，专项 4/4、完整 CTest 19/19 通过。见 `S13C_TASK_DETAIL_USE_CASE_TEST_PLAN.md`。
