# S01 · 独立安全骨架 E2E 测试方案

状态：开发前测试方案。所有测试只可使用构建输出目录下的虚构 fixture；不读取或修改 v2/v3 真实 `Data`、工作目录、日志、备份或附件。

## 测试目标

确认 v3 受控源码快照能以独立数据路径完成启动和最小业务闭环，并且测试环境不会退回到 v2 或真实数据路径。

## 环境

- 可执行目标：`NiumaV3LegacyBaseline`。
- 数据注入：`NIUMA_DATA_DIR=06_test/data/<build-profile>/Data`；该路径必须是绝对路径，并位于本次构建 profile 的 staging 根目录。
- 工作目录：由 fixture 初始化在同一 staging 根目录；`NiumaV3Phase3StagingTest` 通过 `NIUMA_TEST_ROOT=06_test/data/<build-profile>/phase3` 注入，成功后清理该临时目录。禁止指定用户真实目录。
- 业务后端基线：`NiumaV3Phase3StagingTest`，验证文件归档生命周期。

## MCP E2E 场景

| ID | 工具与场景 | 操作 | 预期证据 |
| --- | --- | --- | --- |
| E01 | LazyTesting/Qt 桌面自动化 MCP | 带 `NIUMA_DATA_DIR` 启动程序 | 窗口可见，标题/首页出现；fixture 内创建数据库，v2 Data 未被访问。 |
| E02 | LazyTesting/Qt 桌面自动化 MCP | 点击八个导航入口，最小化/最大化/关闭 | 八页均可达，关闭无崩溃；截图/会话记录保存。 |
| E03 | LazyTesting/Qt 桌面自动化 MCP | Ctrl+N、搜索、F5 | 保持可达且不写入真实路径；功能深度留给后续阶段。 |
| E04 | Playwright MCP（仅有浏览器适配表面时） | 对同一启动、导航和关闭流程执行浏览器 E2E | 可复现脚本、页面/日志证据；若无浏览器适配面，记录 N/A，不能替代 E01–E03。 |
| E05 | backend staging test | 任务创建→完成→取消完成→删除→恢复，检查六个 fixture 工作目录 | 退出码 0、所有断言通过，输出路径均在 staging 根目录。 |

## 前置与完成门

1. Playwright MCP、LazyTesting 或等价 Qt 桌面自动化 MCP、`/simplify` 必须可调用；工具名称、版本/会话和输出写入执行记录。
2. 在写入目标文件后、运行 E01–E05 前调用 `/simplify`；若产生代码修改，重跑构建和 E01–E05。
3. E01、E02、E03、E05 必须通过。E04 仅在浏览器适配表面存在时适用；不存在时保留 N/A 理由。
4. 当前会话缺少上述 MCP/技能，因此原生 UI MCP E2E 与 simplify 尚未执行，S01 不得关闭。

## 已执行的非 MCP 骨架验证

- `cmake --preset v3-staging-debug`：通过。
- `cmake --build --preset v3-staging-debug --target NiumaV3LegacyBaseline NiumaV3Phase3StagingTest NiumaV3StagingPathTest`：通过。`NiumaV3LegacyBaseline.exe` 已生成；保留 v2 `TrendChart.cpp` 的一个未使用变量警告，未作无关改动。
- `ctest --preset v3-staging-debug --output-on-failure`：当时 2/2 通过。后端任务/文件生命周期测试使用并清理 staging `phase3` fixture；数据库与临时报告现按仓库目录约定分别归入 `06_test/data/` 与 `06_test/reports/`。
- 2026-09-22 复跑：三个 S01 target 增量构建成功；同一 CTest 命令 2/2 通过（总计 0.22 秒）。该结果仅复核后端与隔离路径，不改变 MCP E2E/simplify 的未完成状态。
- simplify：已于 2026-09-22 对 S01 的隔离路径、CMake、fixture 及 Qt E2E 驱动完成复核；删除过时的 `pip --target` pywin32 路径补丁和未使用返回值，其余未发现可高置信、行为保持的简化项。每轮后均重跑受影响验证。
- E01–E03：通过。由于 `computer-use` 无法枚举当前无框 Qt 窗口，改用项目隔离虚拟环境中的 `pywinauto==0.6.9` Win32 驱动（`04_program/tools/qt-e2e-venv`），按启动 PID 定位窗口。2026-09-22 的历史会话记录确认标题、导航、快捷键和窗口控制；数据与报告现按仓库目录约定分别归入 `06_test/data/` 和 `06_test/evidence/`。该记录不把无活动桌面描述为当前已测。
- 截图限制：宿主未提供可活动桌面，BitBlt 截图不可用；`e01-main-window.txt`、`e02-navigation.txt` 记录该原因。结构化会话报告是本次可审计替代证据，未将其写为视觉验收通过。
- E04：N/A。当前无浏览器适配面，且 Playwright Node 通道不可载入。插件目录以 `Playwright LazyTesting desktop Qt automated testing`、`automated testing E2E` 检索均无候选。
