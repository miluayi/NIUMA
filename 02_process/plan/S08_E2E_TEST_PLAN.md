# S08 年度成果 E2E 与契约测试方案

## 范围与边界

本阶段复用 v2 任务完成、成果 DAO、金额分单位和 Word HTML `.doc` 导出；仅迁移方案 B 成果页、统计卡、表格和登记弹窗的样式/可定位标识。不改变导出格式、设置项或系统保存对话框。

## 可观察验收场景

| 编号 | 场景 | 通过条件 |
| --- | --- | --- |
| S08-C01 | 登记/跳过 | 560px 登记弹窗含并排数量/金额、72px 说明和年度/重点勾选；Esc 跳过不创建成果。 |
| S08-C02 | 直接登记 | 金额按元输入、按分持久化，年度/重点标记保存。 |
| S08-C03 | 完成任务触发 | 任务完成后自动打开登记弹窗；保存后任务为已完成且成果关联该任务。 |
| S08-C04 | 成果页面 | 年度统计、金额、重点成果和成果表格与数据一致。 |
| S08-C05 | Word 导出 | fixture 内 `.doc` 成功写出并可读；缺少父目录的输出路径返回失败。 |
| S08-E01 | 桌面回归 | pywinauto 从隔离 Data 启动无框主程序，验证导航、快捷键、弹窗和窗口控制。 |

## 执行边界

- `NiumaV3AchievementContractTest` 仅在 `NIUMA_TEST_ROOT` 的 fixture 内创建任务、目录和 Word 文件；不会接触真实 Data 或工作目录。
- 系统保存对话框不通过 pywinauto 自动确认；导出行为由直接调用既有导出器验证，保存对话框的像素/取消路径留 S12。
- BitBlt 不可用时仅记录结构化桌面 E2E；`page-4` 像素对照不以此替代。

## 执行结果（2026-09-22）

- `AchievementView` 已迁入方案 B 的年度统计卡、表格卡片、表头和选择态，并新增年份、导出、统计值和表格的稳定对象名；`AchievementDialog` 的内容、数量、金额、说明和两个勾选框也已可定位。
- `NiumaV3AchievementContractTest` 在 `staging-fixture/achievement` 覆盖 S08-C01 至 S08-C05：Esc 跳过、元到分的金额存储、年度/重点标记、任务完成后自动登记、年度统计，以及 Word `.doc` 成功输出与缺少父目录的失败输出。
- `cmake --build --preset v3-staging-debug` 成功；最终 `ctest --preset v3-staging-debug --output-on-failure` 为 9/9 通过。
- `qt_desktop_e2e.py` 退出码 0：结构化会话确认 1440×900 与 940×580、8 项导航、Ctrl+N、F5、WM_CLOSE 对话框关闭及最小化/最大化/恢复/关闭；证据见 `06_test/evidence/v3-staging-debug/qt-desktop-e2e-session.json`。
- `/simplify` 已复核本轮 UI 和测试：无高置信、行为保持的简化项，因此上述 9/9 是最终验证结果。
- 未通过/递延：系统保存对话框取消和像素截图不由当前桌面会话可自动验证；BitBlt 仍报失败，`page-4` 与登记/保存弹窗的像素门禁留 S12。
