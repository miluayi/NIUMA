# S13-B · 任务查询与表单选项解耦测试方案

日期：2026-09-23（Asia/Shanghai）  
范围：迁移 `TaskListView`、`TaskCreateDialog`、`TaskEditDialog` 的直接持久化依赖到 application 层；保留已有 `TaskService` 命令实现。

## 可观察要求

| 编号 | 行为 | 验收 |
| --- | --- | --- |
| B01 | 列表读取 | `TaskQueryUseCase::loadList` 返回当前筛选页、总数、来源/标签选项；列表的空态、筛选、分页与横滚不变。 |
| B02 | 表单选项 | `TaskQueryUseCase::loadFormOptions` 仅返回启用的来源、标签、办理人，保留 DAO 的 sort_order/id 顺序。 |
| B03 | 手动项 | `TaskFormUseCase::ensureDictionaryEntries` 以既有“缺失时插入、sort_order=9999、空办理人→本人”规则补齐字典。 |
| B04 | 表单保存 | 创建和编辑继续使用既有 `TaskService`；创建后的任务编号通过 `TaskService::getTaskById` 获取，不由 UI 查询 DAO。 |
| B05 | 静态边界 | 三个迁移 UI 文件不包含 `database/`、`DatabaseManager`、`QSqlQuery` 或 `QSqlDatabase`。 |

## 测试实现

- 新增 `NiumaV3TaskApplicationContractTest`：隔离 fixture 验证表单字典补齐、启用选项读取、任务创建与关键词列表页/总数/来源标签返回。
- 保留并复跑 `NiumaV3TaskListContractTest`：空/筛选空、分页、横滚、点击、输入框 Delete 保护、删除取消/确认。
- 保留并复跑 `NiumaV3TaskDialogContractTest`：Tab、文本区 Enter、Esc、`Ctrl+S` 创建/编辑持久化。
- 使用 `rg` 静态扫描三项 UI 源码，作为 B05 的可复现检查。

## 结果

- 专项 CTest：任务 application、列表、表单 **3/3 通过**。
- 最终构建成功；完整 CTest **19/19 通过**。
- 静态扫描未发现三项 UI 文件直接包含数据库类型或 DAO。
- `/simplify`：移除了新测试中的未使用 `QDateTime` 包含；其余代码直接复用既有 `TaskService`/DAO，未发现可安全进一步简化项。
- 不读取或修改真实 `Data`；所有测试使用 `06_test/data/v3-staging-debug`。

## 未完成项

`TaskDetailView` 的历史/成果读取，以及任务命令的统一 application 边界留 S13-C；本切片没有把 `TaskService` 的既有规则复制到 UI 或 application DTO。
