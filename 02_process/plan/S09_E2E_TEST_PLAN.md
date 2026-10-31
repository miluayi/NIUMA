# S09 统计页迁移与 E2E 测试方案

## 范围与边界

S09 仅将 v2 Qt5 Widgets 的 `StatsView` 迁移到方案 B 的页面、卡片和标签视觉契约；保留既有统计 SQL、日期口径、趋势图和热力图算法。测试数据只写入 `06_test/data/v3-staging-debug/stats` 的隔离数据库。

## 自动化契约

| 编号 | 场景 | 断言 |
|---|---|---|
| C01 | 空数据 | 三个标签、四个指标、分类/来源空态、趋势和热力图控件均存在。 |
| C02 | 任务统计 | 写入跨月、分类、来源和状态数据后，DAO 与页面指标、分布文本一致。 |
| C03 | 月趋势 | 断言 12 个月输出及一、二月数量；趋势页标题含当前年份。 |
| C04 | 热力图 | 分别核对创建日期和完成日期热力图数据，并切换页面维度控件。 |
| E01 | 桌面冒烟 | pywinauto 驱动正式暂存程序完成导航、窗口控制、快捷键和安全关闭。 |

## 执行命令

```powershell
& 'C:\msys64\mingw64\bin\cmake.exe' --build --preset v3-staging-debug
& 'C:\msys64\mingw64\bin\ctest.exe' --preset v3-staging-debug --output-on-failure
```

桌面 E2E 使用 `04_program/tests/qt_desktop_e2e.py` 和 `qt-e2e-venv`；BitBlt 截图在当前会话不可用时，保留其 JSON 操作证据，最终像素回归集中在 S12 执行。

## 执行结果（2026-09-22）

- `v3-staging-debug` 全量构建成功，CTest 10/10 通过，其中 `niuma_v3_stats_contract` 用隔离数据库验证 C01–C04。
- pywinauto 无框 Qt E2E 成功：八页导航、940×580/1440×900 窗口尺寸、`Ctrl+N`、`F5`、弹窗 `WM_CLOSE` 和最小化/最大化/还原/关闭均通过；证据见 `06_test/evidence/v3-staging-debug/qt-desktop-e2e-session.json`。
- 当前会话的 BitBlt 仍不可用，`page-5` 的像素截图不以结构化 E2E 替代，留 S12 视觉门禁。
