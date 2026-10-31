# UI → DAO 过渡债表

状态：S15-D 更新。正式构建的 `legacy_v2/ui/` 与 `src/main.cpp` 已静态清零 DAO、`DatabaseManager` 和 `QSqlQuery` 直接依赖；后续维持禁止新增 UI 直连的规则。

| 功能域 | 当前文件 | 证据 | 允许至 | 退出动作 |
| --- | --- | --- | --- |
| 全局/启动与维护 | 已清零（`MainWindow`、`SettingsView`、`ScanImportDialog`、`src/main.cpp`） | S15-C 将状态栏数据、目录迁移、备份/恢复、启动退出序列、重置与扫描导入协调迁入 application；相关 UI 无 DAO/DB/SQL 直连 | 已完成 | S15-C maintenance 合同 1/1、完整 CTest 19/19 通过；静态扫描 `ui/` 与 `src/main.cpp` 无 DAO、`DatabaseManager`、`QSqlQuery` 命中。物理选择器未声称已测。 |
| 首页 | 已清零 | S13-D 已迁移专注、截止排序分页、周期提醒和月度概览读取；星标/提醒命令经 application | 已完成 | HomeView 静态扫描不含 DAO、DatabaseManager、SQL 或 service；首页/application 合同与完整 CTest 19/19 通过。 |
| 任务 | 已清零 | S13-B 已迁移列表、创建、编辑；S13-C 已迁移详情的历史/成果读取、备注和状态命令 | 已完成 | 四个任务 UI 文件静态扫描均不含 DAO、`DatabaseManager` 或 `QSqlQuery`；任务 application 合同与完整 CTest 19/19 通过。 |
| 周期任务 | `ui/PeriodicTaskView.cpp`、`PeriodicTaskCreateDialog.cpp` | 模板读取/写入与启用状态 | S13 | 周期模板查询/命令端口覆盖新建、编辑、启用、删除和提醒动作。 |
| 参考资料 | 已清零（`ReferenceView`、创建/编辑/详情弹窗） | S14-C 查询/命令用例与文件编排端口；此前参考资料专项通过，本轮完整 CTest 再次通过 | 已完成 | 参考资料 UI 静态 DAO/SQL 清零；本轮 `niuma_v3_reference_contract` 纳入全量 19/19。 |
| 年度成果 | 已清零（页面、登记弹窗和年度导出协调） | S14-D/F 经 `AchievementUseCase` 提供年度统计、任务编号 DTO、登记与 Word 导出；成果合同覆盖金额单位、完成触发、年度/重点标记及导出成功/失败 | 已完成 | 成果相关 UI 静态扫描无 DAO、`DatabaseManager`、SQL 命中；年度导出设置/生成调用已位于 application。 |
| 统计 | 已清零（`StatsView.cpp`、`TrendChart`、`HeatmapWidget`） | S14-E 通过 `StatsUseCase` 读取总览、分类、来源、月趋势和热力图；统计模型移至 `models/Statistics.h` | 已完成 | 相关 UI 静态扫描无 DAO、`DatabaseManager`、SQL 命中；StatsView 空/非空、趋势、热力图合同及完整 CTest 19/19 通过。 |
| 回收站 | 已清零 | S15-A `RecycleBinUseCase` 承接列表、单项/批量恢复与永久删除；启动期过期清理在 `StartupUseCase` | 已完成 | 回收站合同及完整 CTest 通过；UI 不含 DAO、数据库或 TaskService 直连。 |
| 设置 | 已清零 | S15-B `SettingsUseCase` 管理三类字典、导出配置和清理命令；S15-C 承接维护、启动与扫描导入 | 已完成 | 设置维护合同通过；SettingsView 不含 DAO、数据库或 SQL 直连。 |

## 管理规则

1. S01 复制后，以 v3 快照相对路径、符号、原始行号、依赖类型和测试覆盖补全每条记录；本表的 v2 路径仅是初始索引。
2. 每个对应解耦阶段前先为该域写 E2E 方案并用可用 MCP 在 fixture 上执行；重构和 `/simplify` 后重复执行。
3. 删除一条债务前，静态搜索证明该 UI 文件不再包含 DAO、`DatabaseManager` 或 `QSqlQuery`，且该域功能/视觉 E2E 通过。
4. 任一新增 UI 直连数据库是 blocker；不以“临时”或“样式需要”豁免。
