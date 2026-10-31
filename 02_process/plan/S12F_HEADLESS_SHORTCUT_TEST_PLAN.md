# S12-F · 无活动桌面快捷键收口

日期：2026-09-23（Asia/Shanghai）  
范围：在保留 v2 Qt5 Widgets 功能链的前提下，补齐可安全定义的方案 B 键盘快捷键；不把破坏性 `Delete` 绑定到没有确认链的列表。

## 可观察要求

| 编号 | 场景 | 实现与验收 |
| --- | --- | --- |
| K01 | 主窗口新建 | `Ctrl+N` 触发既有 `onCreateTaskClicked`。 |
| K02 | 查找任务 | `Ctrl+F` 切到任务管理并聚焦 `taskSearchEdit`。 |
| K03 | 刷新 | `F5` 调用既有 `refreshAllViews`。 |
| K04 | 表单保存 | 任务新建/编辑、资料新建/编辑、周期模板新建/编辑、成果登记各自注册窗口级 `Ctrl+S`，连接既有 create/save 槽。 |
| K05 | 安全删除 | 不实现全局 `Delete`。任务列表尚未公开稳定的当前任务删除动作，也没有键盘触发的确认链；不得绕过确认或在文本输入时误删内容。 |

## 无活动桌面测试方法

- `NiumaV3ShellContractTest` 对主窗口发送 `QKeyEvent(Ctrl+F)`，断言任务页和搜索框焦点；并检查 `Ctrl+N`、`Ctrl+F`、`F5` 的 `QShortcut` 序列。
- `NiumaV3TaskDialogContractTest` 检查任务新建/编辑的 `Ctrl+S` 序列，并通过各 `QShortcut::activated` 到既有保存槽的连接完成新增/保存持久化。
- `Reference`、`PeriodicTask`、`Achievement` 合同分别检查对应 `QShortcut`；资料和成果合同还经 `activated` 路径执行既有保存。
- 这些是 Qt 进程内事件和连接验证，不声称验证了物理键盘、IME 或 Windows 全局键盘注册。

## 结果

- CMake 构建成功；完整 CTest 18/18 通过。
- `/simplify` 审查没有新增可复用抽象：每个弹窗将快捷键直接连接到本类既有保存槽，避免引入跨弹窗行为层。
- 不读取或修改真实 `Data`；测试使用各自 staging fixture。

## 后续

`Delete` 如需支持，应先在任务列表定义“选中记录、确认框、文本输入排除、取消不变更”的完整产品契约，再单独实现和测试。
