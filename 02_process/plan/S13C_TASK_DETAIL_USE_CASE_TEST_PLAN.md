# S13-C · 任务详情 application 边界测试方案与结果

日期：2026-09-23（Asia/Shanghai）  
状态：已完成  
范围：仅迁移 `legacy_v2/ui/TaskDetailView` 的数据读取与命令协调至 `application/task/TaskDetailUseCase`；不改变方案 B 视觉、v2 业务规则、SQLite schema 或真实 `Data`。

## 目标与实现边界

`TaskDetailUseCase` 以详情页所需粒度提供任务、备注、操作历史和成果读取，并委托既有 `TaskService` 处理完成、取消完成、软删除和备注增删。历史及成果 DAO 仅保留在 application 层实现中；UI 继续负责确认框、成果登记弹窗、提示和刷新信号。

| 场景 | 可执行验证 | 预期 |
| --- | --- | --- |
| 详情初始读取 | `niuma_v3_task_application_contract` | 可取回新建任务，且创建历史存在。 |
| 备注链 | 同一隔离 fixture | 添加备注后数量为 1；删除后为空。 |
| 生命周期 | 同一隔离 fixture | “本人”任务可完成、读取为完成、可取消完成、可软删除且状态为“已删除”。 |
| 相关域兼容 | 任务弹窗、成果、回收站合同 | 任务表单、成果登记和删除/恢复既有行为不回归。 |
| 架构退出条件 | `rg` 静态扫描 | `TaskDetailView.{cpp,h}` 不含 `database/`、`DatabaseManager`、`QSqlQuery`、`QSqlDatabase`、`TaskService`、`OperationLogDao` 或 `AchievementDao`。 |

测试只使用 `06_test/data/v3-staging-debug/task-application`，不访问真实数据目录。无活动桌面时，此处的 Qt 合同不声称替代物理键盘、系统选择器或 OS DPI 验收；本切片亦未更改视觉控件。

## 执行结果

2026-09-23：先构建 `NiumaV3TaskApplicationContractTest`，随后运行任务弹窗、application、成果、回收站专项合同，结果 **4/4 通过**。执行完整 `ctest --preset v3-staging-debug --output-on-failure`，结果 **19/19 通过**（含 100/125/150/200% Qt 原生捕获）。静态扫描无命中。

`/simplify` 复核后将 application 合同中对 `optional` 的直接解引用改为先检查有效性；`TaskDetailUseCase` 的薄委托与既有 service 边界一致，未引入额外抽象或行为改动。复测仍为 19/19 通过。
