# S12-A 跨域交互与窗口结构测试计划

日期：2026-09-23（Asia/Shanghai）  
范围：方案 B 正式暂存程序的非像素跨页收口；不更改业务行为、真实 `Data` 或系统 DPI。

## 目标与边界

本切片验证可由隔离 Qt fixture 与无框窗口桌面驱动稳定观察的交互契约：窗口三种关键尺寸、八页导航、快捷键、弹窗 Esc、Tab 焦点推进及文本区 Enter 行为。它不替代视觉或 DPI 验收。

## 自动化用例

| 编号 | 层级 | 场景 | 通过条件 |
| --- | --- | --- | --- |
| C12-A-01 | Qt 合同 | 新建任务表单 Tab 焦点 | 标题输入框收到 Tab 后焦点离开该控件。 |
| C12-A-02 | Qt 合同 | 说明文本区 Enter | 文本区收到 Return 后弹窗仍可见，不触发新建。 |
| C12-A-03 | Qt 合同 | 新建任务 Esc | 新建弹窗结果为 `Rejected`。 |
| E12-A-01 | pywinauto | 无框主窗尺寸 | 初始 1440×900，依次可调整为 1280×720、940×580，且恢复至 1440×900。 |
| E12-A-02 | pywinauto | 八页导航 | 依次触发 0–7 页，进程保持运行。 |
| E12-A-03 | pywinauto | 快捷键与弹窗 | Ctrl+N 打开新建任务弹窗；Esc 关闭；F5 后进程保持运行。 |
| E12-A-04 | pywinauto | 窗口操作 | 最小化、还原、最大化、还原、关闭后以 0 退出。 |

## 隔离与执行

仅使用 `06_test/data/v3-staging-debug`。先构建并运行 CTest 以准备虚构数据库，随后运行 `04_program/tests/qt_desktop_e2e.py`；会话 JSON 输出至 `06_test/evidence/v3-staging-debug/qt-desktop-e2e-session.json`。

## 视觉与 DPI 明确阻断

当前执行环境的 BitBlt 截图不可用，`capture_as_image()` 只会产生错误文本，不能作为像素视觉证据。pywinauto 的尺寸和交互报告仅证明结构化行为。

100%、125%、150%、200% DPI 以及各页面、详情、编辑/确认/导入/系统选择框的像素检查均未在本切片执行，仍为 S12 后续门禁；在可捕获桌面像素且可控制 DPI 的环境完成前，R01–R08 不得标为视觉通过。

## 结果（2026-09-23）

- 构建：`cmake --build --preset v3-staging-debug` 成功。
- Qt 合同：`ctest --preset v3-staging-debug --output-on-failure` 为 13/13 通过；`niuma_v3_task_dialog_contract` 覆盖 C12-A-01 至 C12-A-03。
- 桌面 E2E：`qt_desktop_e2e.py` 退出码 0，结构化会话报告见 [`../../06_test/evidence/v3-staging-debug/qt-desktop-e2e-session.json`](../../06_test/evidence/v3-staging-debug/qt-desktop-e2e-session.json)。E12-A-01、E12-A-02、E12-A-04 通过；E12-A-03 的 Ctrl+N/F5 通过，Esc 的真实桌面注入被无活动桌面阻断，合同测试作为 Qt 行为证据，不能冒充物理桌面键盘证据。
- `/simplify`：按复用、质量、效率三项审阅本轮两处测试改动，未发现高置信、行为保持的简化项；随后已完成上述构建、CTest 与桌面 E2E 复测。
- 像素截图：仍为 BitBlt failed，未通过/未替代视觉验收。
