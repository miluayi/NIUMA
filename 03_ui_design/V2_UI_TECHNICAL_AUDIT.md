# V2 UI 与技术栈审计

## 范围与可信度

本审计基于 v2 的 CMake、入口、UI 源码、QSS、PRD、技术设计与 HTML 原型。结论对静态实现高度可信；没有直接启动 v2 产品 EXE，因为其启动与退出会写日志、执行备份、清理回收站并可能清空跨天专注状态，见 `../../05_workspace_v2/04_program/src/main.cpp:38-120`。

## 技术栈结论

- v2 是 C++17 + Qt 5.15 Widgets + QtSql/SQLite + QtConcurrent + CMake，不是 PySide。证据：`../../05_workspace_v2/04_program/CMakeLists.txt:3-19,37-40,95-99`。
- 当前构建缓存实际解析到 Qt 5.15.18 + MinGW，而文档声称发布目标是 MSVC 2019/v141。工具链声明与现实尚未统一。
- CMake 已开启 `AUTOUIC`，但排除 build 后项目内 `.ui=0`、`.qml=0`。`MainWindow` 和所有页面均在 C++ 中手工创建控件与布局，见 `../../05_workspace_v2/04_program/ui/MainWindow.cpp:139-247`。
- 现行流程是“HTML 原型 → AI 手工翻译 Qt/QSS”，不是 Qt Designer 所见即所得，见 `../../05_workspace_v2/03_ui_design/qt_implementation_guide.md:1-8,345,711-724`。

## 当前产品路径

```text
main.cpp
  → DatabaseManager / BackupService / FileService / FocusService / TaskService
  → MainWindow
     → 首页 / 任务 / 周期任务 / 参考资料
     → 年度成果 / 统计 / 回收站 / 设置
```

v2 的业务核心主要位于 `models/`、`database/`、`service/`、`utils/`；UI 位于 `ui/`。业务与 UI 名义上分层，实际仍有多处页面直接使用 DAO 或数据库，例如 `MainWindow.cpp:526,674`、`HomeView.cpp:476,703`、`SettingsView.cpp:616-841`。

## 关键问题

### P0：输入与键盘行为

1. 多个包含 `QTextEdit` 的创建/编辑弹窗全局拦截 Enter 并提交，导致多行文本无法正常换行且可能误保存。证据：`TaskCreateDialog.cpp:285-293,497-503`、`ReferenceCreateDialog.cpp:187-189,396-400`、`AchievementDialog.cpp:199-201,471-479`。
2. UI 中约有 63 处 `Qt::NoFocus`。侧栏、表格、设置列表和对话框主操作被移出 Tab 焦点链，例如 `MainWindow.cpp:159-168`、`TaskCreateDialog.cpp:313-324`、`RecycleBinView.cpp:240`。
3. PRD 规定的 Esc、Ctrl+N、Ctrl+F、Ctrl+S、Delete、F5 等快捷键见 `PRD_V2.0.md:1066-1078`，实现中缺少统一的快捷键、Tab 顺序、buddy 与 accessible name 契约。

### P1：布局与状态

1. 主窗口最小宽度 940、侧栏固定 220，主内容约 720px；任务筛选控件的最小/固定宽度合计约 840px，表格默认列宽约 1090px，见 `MainWindow.cpp:108-112,159-167`、`TaskListView.cpp:141-213,403-415`。最小尺寸只能依靠横纵双向滚动。
2. PRD 要求列表覆盖 loading/content/empty/error/retry，见 `PRD_V2.0.md:1080-1097`；任务列表、首页与参考资料页没有完整状态机，错误多数仅写日志。
3. 设置中心在 HTML 原型中是分区标签，成品却变成五个大型卡片纵向堆叠；原型与成品已不能互为可靠验收基准。
4. 自定义无边框窗口重造标题栏，但缺少完整边缘命中、系统菜单、Snap 与辅助功能支持，见 `MainWindow.cpp:108`、`TitleBar.cpp:45-56,102-123`。

### P1：可维护性

- UI 约 1.4 万行、61 个头/源文件；长文件同时承担布局、样式、事件与数据访问。
- UI 源中约 342 次 `setStyleSheet()`、约 948 个十六进制色值。尽管 `ColorPalette.h:3-8` 声称是唯一色值来源，大量页面仍以内联字符串覆盖全局 QSS。
- `TaskCreateDialog`/`TaskEditDialog` 与资料创建/编辑维护重复布局，容易继续漂移。
- 现有测试以业务 E2E 可执行程序为主，没有 QtTest 交互测试或截图回归。

## 数据与业务兼容边界

- 实际数据库有 10 张业务表；注释仍写 9 张。`settings.db_version=1`，但 `target_day` 通过结构探测单独迁移，不能仅凭版本号识别真实 schema。
- 状态字符串 `进行中/已完成/已删除/他人办理` 与六个工作目录强耦合，不得因 UI 重构更名。
- `.niuma` 在实现中是目录包而不是 PRD 所称 ZIP；恢复流程和 manifest 仍需在生产接线前加固。
- UI 重构期原则上不改变 schema、目录名、编号、状态转换、备份格式和日期/金额存储。

## 迁移判断

可冻结复用：领域模型、DAO/schema、服务规则、工具类、编号/日期/金额、文件归档约定，以及热力图、趋势图、拖放区、委托等自绘控件。

应重写或先隔离：主窗口与所有页面/弹窗的布局、页面内联 QSS、UI 直接 DAO 调用、重复创建/编辑表单、无边框窗口、键盘与数据状态处理。
