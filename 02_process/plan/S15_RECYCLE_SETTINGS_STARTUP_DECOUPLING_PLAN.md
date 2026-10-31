# S15 回收站、设置与启动维护解耦

## 范围约束

- 保留方案 B UI 与现有 v2 Qt 5 Widgets 外观/交互；不重设计 UI。
- 只使用 `v3-staging-debug` 的 staging fixture；不得读取或操作正式 `Data`、用户数据库、工作目录、备份或附件。
- 不使用 git 命令；文件变更使用 `apply_patch`。
- 每个代码切片先跑相关 CTest，再跑完整 CTest；随后执行 `/simplify` 的行为保持审查和复测。
- 无活动桌面时不声称系统选择器、物理键盘或 OS DPI 已测；使用 Qt 合同、`QWidget::grab()` 与 `QT_SCALE_FACTOR` 替代证据。

## 初始依赖盘点

| 区域 | 当前直接依赖 | 计划边界 |
| --- | --- | --- |
| `RecycleBinView` | `TaskDao::findDeleted`；`TaskService` 恢复/永久删除 | `RecycleBinUseCase` 提供已删除任务列表及状态命令，确认框/提示/刷新留 UI |
| `SettingsView` | 字典表 `QSqlQuery`；`SettingsDao`；`DatabaseManager`；维护请求信号 | 设置/字典 application 用例返回列表与结果；UI 保留表单、确认及文件/目录选择 |
| `MainWindow` | 状态栏查询、工作目录变更与附件路径迁移、备份/恢复、启动清理/跨日专注、退出行为 | startup/maintenance application 用例协调服务；UI 负责选择路径、进度与结果提示 |

## 切片

### S15-A 回收站列表与命令边界

- [x] `RecycleBinUseCase` 承接删除任务查询、恢复和永久删除；`RecycleBinView` 不直接引用 DAO 或 `TaskService`。
- [x] 保持选择、二次确认、批量部分成功计数、分页、刷新信号、删除文件夹副作用与 30 天严格大于边界。
- [x] `NiumaV3RecycleBinContractTest` fixture 覆盖空态、恢复原状态、确认取消/确认、批量与单项永久删除；既有保留期边界测试通过。

### S15-B 设置与字典

- [x] 把三类字典读取/新增/删除、导出配置、工作目录和备份时间读取从 `SettingsView` 移入 `SettingsUseCase`。
- [x] 保留 DAO `create_time`、`sort_order,id` 排序、最大排序值加一、默认配置、提示和信号行为；Settings 与 SettingsMaintenance 合同均通过。

### S15-C 维护、启动与退出

- [x] `MainWindowMaintenanceUseCase` 承接状态栏数据、跨日专注、工作目录初始化/迁移、备份与恢复；`StartupUseCase` 承接数据库启动、自动恢复/备份、工作目录初始化、跨日清零、回收站过期清理、退出导出与关闭数据库。
- [x] `SettingsUseCase` 承接确认后的数据重置；新增 `ScanImportUseCase` 承接工作目录读取、扫描和逐项导入/结果计数。UI 保留目录选择、恢复确认、表格勾选、对话框与提示。
- [x] `settings-maintenance` staging fixture 覆盖启动工作目录初始化、手工备份与恢复成功/失败、扫描导入及初始化确认取消/确认；未执行系统选择器。

### S15-D 收口

- [x] 静态扫描正式构建 `ui/` 与 `src/main.cpp`，未发现 DAO、`DatabaseManager` 或 `QSqlQuery` 直接依赖；`ScanImportDialog` 不再直接引用 `ScanMatchService`、`FileService` 或 `Logger`。
- [x] 本轮 `v3-staging-debug` 全量构建成功，设置维护合同 1/1、完整 CTest 19/19 通过；`/simplify` 三视角复核未发现高置信行为保持简化项。

## 本轮首切片执行结果

2026-09-23：S15-A 新增 `RecycleBinUseCase`，迁移回收站列表、单项/批量恢复和永久删除协调；确认框、选择状态、分页、提示和刷新保留在 UI。S15-B 新增 `SettingsUseCase`，迁移三类字典读写、导出配置和工作目录/备份时间读取。`v3-staging-debug` 构建成功，回收站合同 1/1、设置及维护合同 2/2、完整 CTest 19/19 通过。`RecycleBinView.cpp/.h` 静态扫描不含 TaskDao、`DatabaseManager`、SQL 或 TaskService；SettingsView 不再有字典/设置 DAO 依赖，仅初始化 SQL 留待 S15-C。`/simplify` 复核后批量动作复用单个无状态 TaskService 实例，三类字典排序共用一个小 helper，并完成相关合同和完整 CTest 复测。S15-C MainWindow/启动维护和软件初始化迁移及 S15-D 全 UI 收口尚未完成。

2026-09-23：S15-C/D 完成。维护/启动/退出编排及扫描导入协调均由 application 用例承担，UI 保留确认、路径选择和展示。静态扫描正式 UI 与 `src/main.cpp` 未命中 DAO、`DatabaseManager`、`QSqlQuery`；扫描导入弹窗无直接 service 依赖。最终构建成功，`niuma_v3_settings_maintenance_contract` 1/1、完整 CTest 19/19 通过；`/simplify` 的复用、可读性和效率复核均无高置信行为保持简化项。证据只来自 `v3-staging-debug` staging fixture 与 Qt 合同；系统目录选择器、物理键盘及 OS DPI 未声称已测。
