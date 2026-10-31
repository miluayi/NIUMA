# S12-C Qt 原生像素捕获测试计划

日期：2026-09-23（Asia/Shanghai）  
状态：页面像素捕获通道已恢复；活动桌面/系统选择器/DPI 通道仍阻断。

## 问题与方案

pywinauto 的 BitBlt 截图持续失败，且电脑控制桥接连续三次返回空应用列表与 `nodeRepl.fetch request failed`，不能使用活动桌面截图或输入。为恢复可审阅的方案 B 页面像素证据，本切片使用 Qt Widgets 自身的 `QWidget::grab()`：这直接捕获完成布局和 QSS 渲染后的窗口内容，不依赖 Windows 桌面合成器。

该方案不替代 OS 原生窗口、物理键盘输入、窗口阴影/遮挡、任务栏/多显示器或系统 DPI 证据。

## 可观察用例

| 编号 | 场景 | 通过条件 |
| --- | --- | --- |
| C12-C-01 | 隔离启动 | 使用 `staging-fixture/native-visual-capture/Data` 初始化，不读取真实 `Data`。 |
| C12-C-02 | 八页导航 | 通过方案 B 侧栏行（设置页保持 spacer 后的第 8 行）切换工作区 0–7。 |
| C12-C-03 | 像素捕获 | 每个活动页面生成非空 PNG：`page-0.png` 至 `page-7.png`。 |
| V12-C-01 | 人工抽样 | 直接审阅首页、任务管理、设置中心 PNG，确认存在方案 B 外壳、侧栏、卡片、表单/表格与滚动区域，而非空白/崩溃画面。 |

## 结果

- 新增 `NiumaV3NativeVisualCaptureContractTest`，使用 `MainWindow::grab()` 保存八张 1440×900 隔离页面 PNG。
- 输出目录：[qt-native-captures](../../06_test/evidence/v3-staging-debug/qt-native-captures)。已抽样审阅 [首页](../../06_test/evidence/v3-staging-debug/qt-native-captures/page-0.png)、[任务管理](../../06_test/evidence/v3-staging-debug/qt-native-captures/page-1.png)、[设置中心](../../06_test/evidence/v3-staging-debug/qt-native-captures/page-7.png)。
- 构建成功；CTest 15/15 通过；既有 pywinauto 桌面 E2E 退出码 0；`/simplify` 未发现高置信可简化项。

## 仍未解除的阻断

1. 电脑控制桥接在首次枚举、重试、会话重置后的枚举均返回 `apps: []` 与 `nodeRepl.fetch request failed`。因此本会话无法激活或解锁 Windows 桌面，也不能操控系统原生文件/目录选择器。
2. pywinauto 仍报告 BitBlt failed；它的结构化会话报告不可用作像素证据。
3. Qt `grab()` 仅验证当前 Qt 渲染表面（本次为默认 100% DPI 的逻辑尺寸），未验证 125%、150%、200% DPI 或原生选择器。

恢复活动桌面后，应先重新枚举电脑控制应用；再在隔离 fixture 中运行系统选择器取消/确认、物理 Esc/Tab，并按 100/125/150/200% DPI 采集 OS 级截图。S12 在这些项完成前不能标为完整视觉验收。
