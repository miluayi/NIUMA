# S12-D · R01–R08 证据与阻断矩阵

日期：2026-09-23（Asia/Shanghai）  
依据：`SCHEME_B_V3_DEVELOPMENT_PLAN.md` 第 4 节的 R01–R08。  
结论：S12 的结构化/Qt 渲染子门已显著收口；R01–R08 尚不能整体标为通过。

| 契约 | 已验证证据 | 状态 | 剩余缺口 / 下一验证 |
| --- | --- | --- | --- |
| R01 外壳、八页、窗口操作与尺寸 | `NiumaV3ShellContractTest`；pywinauto 会话的八页、1440×900、1280×720、940×580、最小化/最大化/关闭；Qt 原生八页 PNG | 部分通过 | 顶栏后退/前进/搜索未形成独立端到端交互；1280/940 高频任务的逐项可用性需活动桌面。 |
| R02 页面/详情/弹窗视觉一致性 | 八页 Qt 原生 PNG；各域合同对稳定对象名、布局和通用 `StyledMessageBox`；定稿图库仍为设计对照 | 部分通过 | 编辑、详情、导入、所有确认框的逐项像素基线尚未齐全；需 OS 级截图作最终比对。 |
| R03 任务主链 | `NiumaV3TaskListContractTest`、`NiumaV3TaskDialogContractTest`、`NiumaV3AchievementContractTest`、`NiumaV3RecycleBinContractTest`；全部仅用 staging fixture | 功能部分通过 | 物理附件选择器与真实桌面确认/键盘路径未验证。 |
| R04 周期/资料/成果/统计/回收站/设置维护 | `NiumaV3PeriodicTaskContractTest`、`Reference`、`Achievement`、`Stats`、`RecycleBin`、`Settings`、`SettingsMaintenance` 及通用确认框合同 | 功能部分通过 | 系统保存/目录选择取消与重复操作的真实桌面证据未取得。 |
| R05 四态与重试 | 数据库空与筛选无结果已区分（任务列表等）；各测试覆盖若干错误返回 | 未通过 | 现有 v2 同步 DAO 缺少统一 loading/error/retry 可观察协议；留 S13–S15 解耦时设计并实现，不能由截图补足。 |
| R06 首页语义与滚轮 | `NiumaV3HomeContractTest`、`NiumaV3TaskListContractTest`、`NiumaV3ReferenceContractTest`；首页 Qt PNG | 部分通过 | 嵌套滚轮的真实物理桌面验证，以及不同 DPI 下的滚动/焦点仍缺。 |
| R07 键盘与可访问性 | Qt 合同验证新建表单 Tab、文本区 Enter、Esc、确认框 Esc；`Ctrl+F` Qt 事件；主窗 `Ctrl+N/Ctrl+F/F5` 与任务/资料/周期/成果 `Ctrl+S` 的 `QShortcut`/保存链；任务表格 `Delete` 的输入保护、取消与软删除确认链 | 部分通过 | 连续 Tab、物理键盘/IME 与真实桌面 Esc 仍无 OS 级证据；`Delete` 仅覆盖任务表格，其他域未定义该行为。 |
| R08 DPI、尺寸、日期/下拉、缩放/焦点 | 三种逻辑尺寸 pywinauto；Qt 原生 100/125/150/200% `QT_SCALE_FACTOR` 八页截图；既有日期/下拉合同 | 部分通过 | Qt 缩放因子不等同于 Windows 显示设置；原生日期/下拉弹层和系统选择器仍不具备 OS 级证据。 |

## 证据位置

- 可执行回归：`04_program/build/v3-staging-debug` 的 15 项 CTest；运行命令记录于 `02_process/task.md`。
- 桌面结构化会话：`06_test/evidence/v3-staging-debug/qt-desktop-e2e-session.json`。
- Qt 原生页面像素：`06_test/evidence/v3-staging-debug/qt-native-captures/page-0.png` 至 `page-7.png`。
- 切片计划：`S12A_CROSS_DOMAIN_E2E_TEST_PLAN.md`、`S12B_DIALOG_AND_SYSTEM_SELECTOR_TEST_PLAN.md`、`S12C_NATIVE_VISUAL_CAPTURE_TEST_PLAN.md`。

## 环境阻断与恢复顺序

电脑控制驱动在本轮的首次枚举、重试、内核重置后重试都返回 `apps: []` 及 `nodeRepl.fetch request failed`。在驱动能枚举并绑定 Windows 应用前，不得声称 OS 级 E2E、系统选择器或 DPI 通过。

恢复后按以下顺序执行：1）绑定隔离版 `NiumaV3LegacyBaseline.exe` 并确认实际截图；2）Ctrl+F/Ctrl+S/Delete/Esc/连续 Tab；3）附件、导出、目录/恢复/初始化选择器的取消和确认；4）100/125/150/200% DPI × 1440×900/1280×720/940×580；5）将 R01/R02/R03/R04/R06/R07/R08 的状态逐项更新。R05 另依赖 S13–S15 的可观察异步状态设计。
