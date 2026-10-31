# NIUMA V3.0 当前技术设计

更新：2026-09-25。本文件描述已构建的正式程序，不把历史重构计划当成已完成的架构。代码事实以 `../04_program/legacy_v2/` 和 `../04_program/CMakeLists.txt` 为准。

## 1. 构建与模块

正式程序使用 C++17、Qt 5.15 Widgets/Sql/Concurrent、CMake 与 SQLite。入口为 `legacy_v2/src/main.cpp`，内部 target 保留为 `NiumaV3LegacyBaseline`，产品输出文件固定为 `Niuma.exe`。`legacy_v2/ui` 实现窗口、八个页面和弹窗；`application` 提供业务用例；`service` 处理任务目录、扫描、备份和信息文件；`database` 保存和查询 SQLite 数据；`models` 为数据模型；`resources` 内置样式、图标及字体。开发期预览源码不进入正式 target。

当前工程保留部分旧版结构和命名；文档不宣称所有 UI 均已完成 DAO 隔离。正式 UI 使用已定稿样式，产品代码和 ZIP 名称不使用开发期方案名称。

Windows 11 主窗口由 DWM 提供圆角、边框和阴影，不叠加全窗口透明子控件。通过动态解析 RtlGetVersion 与 DwmIsCompositionEnabled 判断原生圆角能力；Windows 7/8/10 使用 WA_TranslucentBackground 与八个窄边缘绘制控件，透明清除圆角外部并绘制抗锯齿边框，不使用 QRegion、全窗口图形效果或完整覆盖层。窄边缘不创建原生子窗口，且不接收输入；最大化转直角、还原转圆角。旧系统移除 WS_CAPTION，禁用系统非客户区绘制并处理 WM_NCPAINT/WM_NCACTIVATE，保留 WS_THICKFRAME 和主窗口最小化/最大化/系统菜单能力。阴影按系统合成能力提供，软件圆角与边框不依赖 Aero。

弹窗只对已经存在的原生句柄设置 DWM 外观，只读取 internalWinId，不通过 winId 强制创建句柄；提前设置 WA_DontCreateNativeAncestors，避免普通祖先/兄弟控件被提升为原生窗口并截获鼠标输入。Show 事件先于 Qt 可见标记更新，因此不依赖 isVisible 判断句柄是否可配置。窗口控制按钮不调用 QPushButton 的系统绘制路径，背景、按下、键盘焦点及矢量图标均自行绘制。

WindowsChrome.cpp 作为独立静态库，使用配套 Qt5 Gui 私有开发接口的 WindowsCustomMargins 抵消系统边框，并让 Qt 处理 NCCALCSIZE 以同步几何缓存；保留窗口原生位置/尺寸，在 DPI 改变后重算。现代 DPI 接口动态解析，Windows 7 回退至 AdjustWindowRectEx，不新增强制 API 或运行时 DLL。这避免跨显示器时 Qt 缓存边框宽度，导致透明 backing store 小于系统窗口、显示或鼠标命中失效。回归检查实际客户区、系统窗口与 Qt 绘制尺寸一致。

## 2. 本地数据

`PathUtil` 确定程序数据目录及 `database.db` 路径；`FileService` 保存和校验工作目录。程序初始化工作目录时创建以下六个根目录：

| 目录 | 用途 |
| --- | --- |
| `01_工作区_进行中` | 本人进行中的任务 |
| `02_历史归档` | 已完成任务，按年/月分层 |
| `03_年度成果` | 年度成果文件 |
| `04_参考资料` | 资料目录 |
| `05_回收站` | 已删除任务 |
| `06_他人办理` | 非本人办理的任务 |

任务与资料的数据库记录分别关联工作目录中的实体目录。状态变更先尝试移动目录；失败时不提交新的任务状态。异步附件复制未完成时会阻止相关移动。程序不应将工作目录当成可随意清理的构建缓存。

## 3. 信息文件与扫描

`TaskInfoFile` 将 `.niuma-task.json` 写在任务目录根部，记录 `task_no`、`title`、`category`、`source`、`tag`、`handler`、`deadline`、`description`、`is_key_task`、`has_subdirs`、`original_status`、创建、完成与删除时间。`ReferenceInfoFile` 将 `.niuma-reference.json` 写在资料目录根部，记录编号、名称、来源、标签、说明和创建时间。写入使用 `QSaveFile`；Windows 上设置隐藏属性。

`StartupUseCase` 初始化数据库与工作目录、为缺少 info 文件的旧记录补建文件、扫描新增目录并清理过期回收站任务。`ScanMatchService` 仅扫描工作区任务根层、历史归档年/月层、资料根层、回收站根层和他人办理根层；附件子目录不作为独立任务。扫描按合法编号和目录名解析，目录位置决定任务状态，info 补充不能从目录名获得的字段。冲突留给扫描导入界面处理。

## 4. 备份、验证与发布

备份 `.niuma` 是目录包，不是 ZIP；其数据库副本、配置、清单和可选的 `workdir/` 内容由 `BackupService` 管理。交付 ZIP 是另一种格式，仅含可执行程序和运行依赖。格式与回退步骤详见 [Backup_Recovery_V3.0.md](Backup_Recovery_V3.0.md)。

正式构建使用根目录 `CMakePresets.json` 的 `v3-staging-debug` 与 `v3-staging-release`。`04_program/tests` 中的 CTest 源码应保留；生成的构建树、虚拟环境、测试数据库、旧抓图与日志可以重新生成，应与正式源码和产品区分。验证先运行相关 CTest，再运行完整 CTest，并检查 `QWidget::grab()` 抓图。Qt 抓图不能证明物理桌面输入或 Windows 系统级 DPI 行为。

原生输入合同通过系统鼠标路径点击自己启动的进程，检查最小化；原生导航合同在新启动的窗口检查真实点击切换页面及搜索框聚焦。点击前核对鼠标命中的窗口所属 PID，避免操作其他应用。全局失焦依据事件实际接收控件判断，避免自定义原生边框的全局坐标偏差导致文本框点击后立即失焦。下拉框外观合同等待 Qt 显示动画结束后才检查可见状态。

旧系统外观测试仅在测试进程设置 niumaLegacyWindowChrome 应用属性，强制进入真实兼容绘制路径；覆盖 100%/200% 透明度边缘、缩放、最大化/还原及 Windows/Fusion 样式下按下图像一致性。导入审计递归检查程序、已交付插件及依赖，拒绝已知 Windows 8/10 强制导入接口与高于 6.1 的 PE 子系统版本；可选 API 字符串不视为静态导入。此检查不替代 Windows 7 SP1 实机执行。参考：[微软圆角说明](https://learn.microsoft.com/en-us/windows/apps/desktop/modernize/ui/apply-rounded-corners)、[MSYS2 目标系统说明](https://www.msys2.org/docs/windows_support/)。
