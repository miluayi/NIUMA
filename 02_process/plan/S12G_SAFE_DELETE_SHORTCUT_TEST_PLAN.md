# S12-G · 任务列表安全 Delete 快捷键

日期：2026-09-23（Asia/Shanghai）  
范围：为任务管理表格补齐受确认保护的 `Delete`；复用 v2 的软删除/回收站语义，不影响文本编辑。

## 可观察要求

| 编号 | 场景 | 验收 |
| --- | --- | --- |
| D01 | 表格选中 | `Delete` 仅当 `taskTable` 聚焦且存在有效当前行时生效。 |
| D02 | 输入保护 | 搜索框等非表格控件聚焦时，`Delete` 不弹确认、不改变任务状态。 |
| D03 | 取消 | 确认框取消或 Esc 后任务保持原状态。 |
| D04 | 确认 | 确认后调用既有 `TaskService::deleteTask`，任务变为“已删除”，列表与主窗口关联页面刷新。 |
| D05 | 无选择 | 表格没有当前行时不弹确认、不调用服务。 |

## 实现边界

- `QShortcut(Delete)` 的父控件为 `taskTable`，上下文显式为 `Qt::WidgetShortcut`；因此不会从主窗口或文本输入控件触发。
- 确认使用既有 `StyledMessageBox::question`，文字与详情页删除保持同一“可在回收站恢复”语义。
- 不执行物理删除，不新增 DAO/SQL，也不读取真实 `Data`。

## 无活动桌面验证

`NiumaV3TaskListContractTest` 在独立 fixture 中：

1. 创建任务、选中首行，向搜索框发送 Qt `Delete` 事件，断言无模态框且任务仍“进行中”；
2. 通过表格快捷键的 Qt `activated` 连接触发删除，定时关闭确认框并断言取消不变；
3. 再触发并确认，断言状态成为“已删除”。

结果：专项构建成功；`niuma_v3_shell_contract` 与 `niuma_v3_task_list_contract` 2/2 通过；最终完整 CTest 18/18 通过。

## Simplify 复核

- 复用：删除直接复用 `TaskService::deleteTask`、`StyledMessageBox` 和既有主窗口刷新链，无重复 DAO 实现。
- 质量：`WidgetShortcut` 将作用域限定到表格，且先验证当前索引与任务 ID，避免输入框误触和无选中操作。
- 效率：删除成功后只调用现有 `refresh()`，再由主窗口刷新关联页面；未增加轮询、缓存或额外查询。

未发现保持行为不变且能进一步简化的高置信改动。
