# S06 周期任务 E2E 与契约测试方案

## 范围与边界

本阶段只迁移 v2 Qt5 Widgets 的方案 B 页面和弹窗样式；复用 `PeriodicTaskService`、`PeriodicTaskDao`、提醒计算与 `MainWindow` 信号链，不修改业务规则、数据库结构或工作目录行为。

## 可观察验收场景

| 编号 | 场景 | 通过条件 |
| --- | --- | --- |
| S06-C01 | 空周期页 | `periodicTaskTable` 无行时隐藏，空状态可见。 |
| S06-C02 | 双模式模板弹窗 | 新建与编辑均有方案 B 640px 最小宽度、命名输入和周期/启用控件；Esc 取消不保存。 |
| S06-C03 | 模板页面 | 有模板时表格显示，名称靠左、其余数据列居中、启用单元格存在。 |
| S06-C04 | 启用与删除 | 启用状态可往返持久化；删除确认取消后模板仍在，确认后模板删除。 |
| S06-C05 | 首页提醒动作 | 到期模板可由服务创建实际任务，且创建或忽略均推进 `last_reminded_date`，不再作为当次到期提醒（未来 7 天预览仍可显示）。 |
| S06-E01 | 桌面回归 | pywinauto 在隔离 Data 目录启动无框主程序，遍历导航、快捷键、对话框和窗口控制。 |

## 执行方法与证据

- C++ 契约：`NiumaV3PeriodicTaskContractTest` 使用 `NIUMA_TEST_ROOT` 下的 staging fixture；不读取或写入真实 Data、日志与工作目录。
- 全量回归：构建后运行 CTest 预设，确保既有 S00–S05 契约不回归。
- 桌面 E2E：运行 `tests/qt_desktop_e2e.py`，保留 JSON 会话证据。
- 当前桌面会话不提供可采集的 BitBlt 截图时，只记录结构化 UI 证据；像素截图比对继续留给 S12，不能以文字替代。

## 执行结果（2026-09-22）

- 迁入 `PeriodicTaskView` 的方案 B 卡片、表头、选择态、列对齐和居中操作区；给表格、空态和周期模板表单建立稳定 object name。
- `NiumaV3PeriodicTaskContractTest` 以单独的 `staging-fixture/periodic-task` 验证 S06-C01 至 S06-C05：包括新建/编辑模式、Esc、启用往返、删除确认取消与确认、从到期模板创建任务、忽略本次提醒。提醒服务本身会保留未来 7 天预览，故测试断言的是当日到期标记被推进。
- `cmake --build --preset v3-staging-debug` 成功；`ctest --preset v3-staging-debug --output-on-failure` 为 7/7 通过。
- `qt_desktop_e2e.py` 退出码 0：结构化会话确认 1440×900 与 940×580、8 项导航、Ctrl+N、F5、WM_CLOSE 对话框关闭及最小化/最大化/恢复/关闭。证据：`06_test/evidence/v3-staging-debug/qt-desktop-e2e-session.json`。
- `/simplify` 复核本轮变更，没有高置信、行为保持的简化项；上述全量 CTest 已在复核前后的最终代码上通过。
- 未通过/递延：该桌面环境仍报 `BitBlt failed`，没有生成双模式弹窗和删除确认的像素截图；`page-2` 像素级验收保留 S12，不以结构化 E2E 替代。
