# S03 首页迁移：E2E 与回归方案

日期：2026-09-22  
范围：方案 B 首页；不改变 v2 的任务、专注、周期提醒或统计服务契约。

## 可观察验收项

| 编号 | 场景 | 自动化方式 | 断言 |
| --- | --- | --- | --- |
| S03-E01 | 首页空数据 | Qt 合同测试 | 今日专注空态可见；标题为 `今日专注  0/3`；进行中分页信息为 0。 |
| S03-E02 | 11 条进行中任务 | Qt 合同测试 | 首页只显示 10 条、分页为 2 页、首页排序为有效截止日升序且无截止日最后；任务管理默认排序不受影响。 |
| S03-E03 | 首项加入/移出专注 | Qt 合同测试 | 星标经 `FocusService` 加入后标题更新为 1/3、行仍是 48px；移出后恢复 0/3。该刷新不重建首页根部布局，滚动容器位置保持。 |
| S03-E04 | 周期提醒和本月概览 | Qt 合同测试 | 到期提醒行、四个统计单元及本月总数/完成数/完成率均由现有服务结果驱动。 |
| S03-E05 | 无框桌面前端路径 | pywinauto | 在隔离 staging 数据目录启动正式程序，确认首页主窗尺寸、首页导航、刷新及正常关闭；产出结构化会话证据。 |
| S03-E06 | 整体回归 | CTest + 构建 | 已有 staging、Shell 契约和新增首页契约全部通过。 |

## 迁移边界

- 复用 `HomeView`、`TaskDao`、`FocusService`、`PeriodicTaskService` 与现有数据结构；不复制原型 `SchemeBStyler` 的 SQL 重建行。
- 首页在 `TaskFilter` 上显式请求截止时间排序；任务管理页继续使用既有默认排序。
- 迁移定稿的三项专注预留高度、10 条分页、列表列头、卡片边界及统计单元，不迁移任何方案 A/C 样式。
- pywinauto 是 LazyTesting/Qt 桌面 E2E MCP 的本地替代驱动。当前无活动桌面，BitBlt 像素截图可能不可用；失败时报告仅作为结构化 UI 会话证据，不伪称像素视觉通过。

## 数据与隔离

所有测试数据写入 `06_test/data/v3-staging-debug/`；禁止读取或修改真实 `Data`、工作目录与日志。Qt 合同测试使用进程私有 SQLite staging 数据库，并在退出前关闭数据库。

## 执行结果

- `cmake --build --preset v3-staging-debug`：通过。
- `ctest --preset v3-staging-debug --output-on-failure`：4/4 通过（含新增 `niuma_v3_home_contract`）。S03 合同 fixture 覆盖空态、11 项两页、截止排序/无截止项、星标加入和移出、滚动位置、周期提醒、四格月度统计。
- `qt_desktop_e2e.py`：退出码 0；会话证据为 [`qt-desktop-e2e-session.json`](../../06_test/evidence/v3-staging-debug/qt-desktop-e2e-session.json)。确认无框首页的主窗、八项导航、940×580/1440×900 尺寸、Ctrl+N、F5、最小化/最大化/关闭。
- `/simplify`：已对 S03 改动做复用、质量和效率三项复核；无高置信、行为保持的进一步简化。随后复跑本阶段构建与 CTest。
- 未验证项：当前无活动桌面导致 BitBlt 像素截图不可用，因此 `page-0`、`home-bottom`、`focus` 的像素级视觉比对留在有交互桌面的 S12 DPI/视觉门禁，不计为本阶段已通过。
