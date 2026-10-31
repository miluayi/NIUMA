# S00 · 决策与契约对齐测试方案与结果

日期：2026-09-22。范围是文档、冻结基线与工具就绪性；S00 未生成前后端功能代码，因此不存在可执行的 Qt 前后端业务 E2E 流程。本文件不替代 S01 起每个功能阶段的 MCP E2E 方案。

## 目标与场景

| 场景 | 前置条件 | 操作 | 预期 |
| --- | --- | --- | --- |
| B 路线一致性 | 当前约束文档可读 | 搜索 Designer `.ui`、原生窗口、深色提案等旧强制语句 | 仅历史记录保留旧提案；当前规范指向 ADR-005 和方案 B Widgets。 |
| 过渡边界 | ADR/README/架构/债表可读 | 对照 UI→DAO 规则 | S01–S12 的受控例外、禁止新增、S13–S15 清零条件一致。 |
| 冻结完整性 | v2 和 B 源文件可读 | 重新计算八项 SHA-256 | 与 `S00_V2_AND_SCHEME_B_BASELINE.md`、`FINAL_UI_HANDOFF.md` 一致。 |
| 交接可达性 | 计划/任务/清单存在 | 校验相互引用的文件路径 | 文件存在，当前阶段为 S01。 |
| MCP 就绪性 | Playwright/LazyTesting/simplify 应可调用 | 查找本会话可调用 MCP/技能，并尝试 Playwright 运行时导入 | 记录可用性；若不可用或不支持 Qt Widgets，标记 blocker，不能伪报 E2E。 |

## 执行记录

- 文档一致性、文件存在性和 SHA-256 采用只读 PowerShell/`rg` 检查；结果见本文件下方“结果”。
- 本会话未暴露 Playwright MCP、LazyTesting MCP 或 `/simplify` 技能。发现的 `mcp__node_repl__js` 运行时检查未能成功加载 Playwright（返回 module export 错误），因此不将其视作可用的 Playwright E2E 能力。
- 原生 Qt Widgets 也没有浏览器 DOM；即使未来 Playwright 可用，仍须有浏览器适配表面或 LazyTesting/桌面自动化 MCP 执行原生界面 E2E。

## 结果

- 文档路线：通过。ADR-005/006、架构、验收、简报、功能矩阵和生产 README 已统一为方案 B Widgets 路线；ADR-001/003 明确为历史取代记录。
- 基线文件：通过。v2 CMake/入口/资源/QSS 和 B 四项指纹已冻结；B 四项与 `FINAL_UI_HANDOFF.md` 一致。
- 过渡债表：通过。16 个 v2 UI 直接数据库依赖文件已按域、退出阶段和退出动作登记。
- 初始规划曾将 MCP E2E/simplify 工具可用性视为阻断。该工具门槛已按后续用户决定取消；开发状态与测试结果以当前 staging CTest 记录为准。

## S01 测试方案骨架（须先补全再写代码）

1. 使用只含虚构数据的 fixture 和独立工作目录启动 v3 骨架，证明未访问真实 Data。
2. 通过适用于 Qt 的 MCP 完成启动、首页显示、八个导航切换、关闭的 E2E；如果提供浏览器适配表面，再用 Playwright 覆盖同一流程。
3. 记录 fixture 路径、工具版本/会话、命令、截图和退出码；每次代码生成/修改及测试后运行 `/simplify` 并复跑。
