# S04 任务列表迁移：E2E 与回归方案

日期：2026-09-22  
范围：方案 B 任务管理页。复用 v2 `TaskListView`、`TaskTableModel`、`TaskItemDelegate`、`TaskDao` 和 `PaginationWidget`；不改变 schema、状态文本、筛选字段或详情链路。

## 可观察验收项

| 编号 | 场景 | 自动化方式 | 断言 |
| --- | --- | --- | --- |
| S04-E01 | 数据库为空 | Qt 合同测试 | 显示“暂无任务”空态，不显示表格；分页为 0。 |
| S04-E02 | 筛选无结果 | Qt 合同测试 | 显示“没有符合当前筛选条件的任务”，与数据库空态文案区分。 |
| S04-E03 | 搜索、筛选、重置与分页 | Qt 合同测试 | 关键词、分类、状态、日期进入既有 `TaskFilter`；翻页保留筛选；重置恢复默认。 |
| S04-E04 | 表格视觉/语义契约 | Qt 合同测试 | 9 列、48px 行高、标题列左对齐/其余居中、方案 B 表头与分类/状态 pill、重点与日期语义色。 |
| S04-E05 | 行详情与滚轮 | Qt 合同测试 | 行单击仍发射原 task id；无纵向溢出但有横向溢出时，垂直滚轮转为表格横向滚动且不冒泡。 |
| S04-E06 | 无框桌面前端路径 | pywinauto | staging 数据目录启动、切换任务管理、窗口尺寸、导航、刷新及关闭产生结构化会话证据。 |

## 迁移边界和已知限制

- 仅将原型 `SchemeBTaskDelegate` 的确定性绘制规则收敛到正式 `TaskItemDelegate`；不得引入原型运行时修饰器或假数据 SQL。
- `TaskDao` 是同步接口，当前没有可观察的异步 loading/retry 协议。S04 为空数据库和筛选无结果提供不同状态；真实数据库错误显示/重试需在 S13 解耦为可报告 use case 后实现，不能伪造“已覆盖”。
- 横向滚轮只限定任务表格；不改变 S06/S07 表格，跨列表通用行为在 S12 收口。
- 所有 fixture 位于 `06_test/data/v3-staging-debug/`，不读取或修改真实数据。

## 执行结果

- `cmake --build --preset v3-staging-debug`：通过。
- `ctest --preset v3-staging-debug --output-on-failure`：5/5 通过；新增 `niuma_v3_task_list_contract` 在可重复的 staging 数据库中覆盖 S04-E01–E05（包括受控 `QWheelEvent` 横向滚动）。
- `qt_desktop_e2e.py`：退出码 0；会话证据为 [`qt-desktop-e2e-session.json`](../../06_test/evidence/v3-staging-debug/qt-desktop-e2e-session.json)，确认无框主窗、八项导航、两档窗口尺寸、Ctrl+N、F5 和正常关闭。
- `/simplify`：已复核新增状态栈、事件过滤器、委托配色与测试 fixture；唯一高置信修正为统一表头容器与 section 的 `#F5F8FC` 色值。复跑全部 CTest 后仍为 5/5。
- 未验证项：同步 `TaskDao` 没有可观察的查询失败/重试协议，loading/error/retry 不能据实自动验证，留待 S13；BitBlt 像素截图仍不可用，`page-1` 的像素门禁留给 S12。
