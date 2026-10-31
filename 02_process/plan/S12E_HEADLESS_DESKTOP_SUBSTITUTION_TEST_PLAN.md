# S12-E · 无活动桌面的替代测试方法

日期：2026-09-23（Asia/Shanghai）  
决策：电脑控制插件暂不可修复；S12 后续测试不依赖活动 Windows 桌面，但严格区分应用语义验证与 OS 原生 UI 验证。

## 范围、非目标与可观察要求

| 原阻断项 | 替代方法 | 已验证 / 可验证的结论 | 明确非结论 |
| --- | --- | --- | --- |
| 125/150/200% OS DPI | CTest 分别以 `QT_SCALE_FACTOR=1.25/1.5/2` 启动完整 Qt 程序，`QWidget::grab()` 捕获八页 | 应用在 Qt 高 DPI 缩放因子下可构造、切页并生成非空页面 PNG；200% 任务页已人工抽样审阅 | 非 Windows 显示设置、多显示器、系统菜单或第三方原生窗口实测 |
| 文件/目录选择器 | 按选择结果的业务边界验证：空路径是取消；已选路径交给现有 `FileService`、`BackupService`、`WordExport` 和附件服务在隔离 fixture 验证 | 设置请求信号、初始化取消/确认、恢复有效/无效备份、导出成功/失败、附件服务和目录语义由既有合同测试覆盖 | 非 Windows `QFileDialog` 的外观、可访问性、鼠标输入或 OS 最近目录行为 |
| 真实物理键盘 | 在 Qt 事件循环中对窗口/控件发送 `QKeyEvent`，验证 Tab、Enter、Esc 与合适焦点控件的语义；公共确认框与任务新建合同已覆盖 | 新建表单 Tab 推进、文本区 Enter 不误提交、Esc 取消、确认框 Esc/确认/取消 | 键盘驱动、IME、全局快捷键注册、Windows 焦点切换 |

## DPI 回归结果

新增 CTest：`niuma_v3_native_visual_capture_dpi_125`、`_150`、`_200`。四个倍率（100/125/150/200%）均通过，每个倍率生成八页 PNG；输出目录：

- `06_test/evidence/v3-staging-debug/qt-native-captures`
- `06_test/evidence/v3-staging-debug/qt-native-captures/dpi-125`
- `06_test/evidence/v3-staging-debug/qt-native-captures/dpi-150`
- `06_test/evidence/v3-staging-debug/qt-native-captures/dpi-200`

200% 任务管理页已人工审阅，侧栏、筛选、操作按钮、表格空态和状态栏仍完整可见。

## 键盘与选择器的诚实结论

源码核对表明 `MainWindow` 当前没有 `QShortcut` 或全局 `Ctrl+F`/`Ctrl+S` 实现。故 R07 只对已执行的 Qt 合成事件路径部分通过；Ctrl+F、Ctrl+S、Delete 不能因测试替换而标为通过。它们是后续明确的产品实现任务，而不是电脑控制环境问题。

同理，选择器替代方法只验证“应用收到取消或已选路径后如何处理”。它不声称 Windows 原生对话框被截图或可操作。若电脑控制恢复，再补 OS 原生验证；若不恢复，这些原生外观项作为发布前人工环境验收，不阻塞隔离功能回归。

## 结果与回归

- 构建成功；完整 CTest 18/18 通过。
- `/simplify` 已审阅 DPI CMake 注册；无高置信简化项。
- 未新增产品功能，不读取或写入真实 `Data`。
