# S16 发布候选与回退验证方案

## 安全边界

- 所有运行验证使用 `06_test/data/v3-staging-debug`；不得读取或复制正式 `Data`、数据库、WAL/SHM、日志、备份、工作目录或附件。
- 本阶段按用户 2026-09-23 指示，以 Release 本地测试通过后制作本地交付 ZIP 为完成标准。始终不读取、复制或修改真实 Data；交付包不得包含真实数据。
- MiSans 字体以内置资源随程序运行。Win7 SP1、系统选择器、物理键盘和 OS DPI 不作为本地测试与打包条件。

## 验证矩阵

| 门 | 仅隔离执行步骤 | 通过证据 | 当前状态 |
| --- | --- | --- | --- |
| 构建与全链 | `cmake --build --preset v3-staging-debug`；随后完整 `ctest --preset v3-staging-debug --output-on-failure` | 正式目标构建成功、所有注册 CTest 通过 | 通过：构建成功，CTest 19/19 |
| 功能与数据安全 | 检查 CTest 环境变量均指向 `06_test/data/<build-profile>`；在 maintenance 合同验证启动、备份、恢复成功/失败和重置取消/确认；不得接触外部数据目录 | fixture 测试退出 0；维护合同通过 | 通过：maintenance 合同 1/1；范围仅为 fixture |
| 视觉/缩放替代证据 | 原生 Qt `QWidget::grab()` 合同及 `QT_SCALE_FACTOR=1.25/1.5/2` CTest | PNG 与合同通过；证据只代表 Qt 控件捕获 | 通过：相关合同 4/4；不代表 OS DPI |
| 构建与运行库部署 | 记录当前 CMake、MinGW、Qt 版本与构建类型；检查 Release 配置、Qt DLL/插件收集及在隔离 PATH 下的运行 | 可复现的开发候选描述和隔离运行结果 | 部分通过：手动 Release 配置/构建有效；`windeployqt-qt5 5.15.18` 部署 smoke bundle 后，Settings、中文路径/SQLite、Qt 抓图均在仅含 bundle 与 Windows 系统目录的 PATH 下通过。无 CMake 安装/部署规则；未形成安装包 |
| PE/目标系统静态审计 | 只读取 Release EXE 的 PE Optional Header 与直接导入表 | 原始字段和模块列表；静态数据不得表述为 OS 兼容实测 | 部分通过：GUI 子系统，`MajorSubsystemVersion=5`、`MinorSubsystemVersion=2`；直接导入含 `dwmapi.dll`、Qt5 Core/Gui/Sql/Widgets 和 MinGW 运行库。此字段不能证明 Win7 SP1 运行兼容性，仍需目标机验证 |
| 回退 | 只对 staging maintenance fixture 建立基线，运行备份、清空/恢复和恢复失败场景；比较 fixture 记录及文件存在性 | 合同证据，不得外推为真实数据回退能力 | 通过：maintenance 合同内 fixture 备份成功、恢复成功和缺失包失败；不能外推为发布回退 |
| 正式数据副本 | 不读取/复制任何正式数据 | 本地包不包含真实数据 | 取消：按用户决定不执行正式数据副本回退演练；真实 Data 安全边界继续有效 |
| 目标 OS 与人工输入 | 本地交付不依赖 Win7、系统选择器、物理键盘或 OS DPI 门禁 | 明确记录未执行、不声称通过 | 取消：按用户决定从本地交付门禁移除；均未测试 |
| 本地打包 | Release 全量 CTest 通过后，以 `windeployqt` 部署、闭包检查、隔离启动和 ZIP 解包核验 | 包含应用与运行依赖，不含 fixture、测试 EXE、数据库或日志 | 通过：ZIP 63 个条目，含 1 个 EXE、61 个 DLL/插件及 README，无 `.db`/`.log`；隔离启动数据库完整性检查通过 |

## 执行命令

```powershell
$env:Path='C:\msys64\mingw64\bin;C:\msys64\usr\bin;' + $env:Path
& 'C:\msys64\mingw64\bin\cmake.exe' --build --preset v3-staging-debug
& 'C:\msys64\mingw64\bin\ctest.exe' --preset v3-staging-debug --output-on-failure
```

Release 构建输出位于 `04_program/build/v3-staging-release`，CTest fixture 与报告分别位于 `06_test/data/v3-staging-release` 和 `06_test/reports`：

```powershell
$env:Path='C:\msys64\mingw64\bin;C:\msys64\usr\bin;' + $env:Path
& 'C:\msys64\mingw64\bin\cmake.exe' --preset v3-staging-release
& 'C:\msys64\mingw64\bin\cmake.exe' --build --preset v3-staging-release
& 'C:\msys64\mingw64\bin\ctest.exe' --preset v3-staging-release --output-on-failure --output-log 06_test/reports/ctest-v3-staging-release.log
```

发布门禁结果须按实填写；完整 CTest 或 staging 维护合同通过不等于真实数据、Win7、系统字体缺失回退或正式发布通过。

## 本轮结果

2026-09-23：Debug 隔离构建成功；maintenance 与 Qt 抓图/缩放专项 5/5、完整 CTest 19/19 通过。随后在独立 `04_program/build/v3-staging-release` 配置 Release，构建成功；maintenance 与 Qt 抓图/缩放专项 5/5、完整 CTest 19/19 通过。统计图源码中删除了未使用的局部变量 `h`；设置中心增加字号和字重预览选择卡片，字体资源以内置方式提供；`SchemeBTheme::preferredFontFamily` 的纯选择合同分别断言 MiSans 存在和仅有回退字体的结果。`/simplify` 检查复用、清晰度与执行成本，无额外高置信简化项。新增字体选择合同后 Debug shell 专项 1/1、完整 CTest 19/19；Release shell 专项 1/1、完整 CTest 19/19。

构建环境 CMake 4.3.3、MinGW g++ 16.1.0、Qt 5.15.18。初次审查时尚无 Release preset；之后已增加 `v3-staging-release`，构建输出归入 `04_program/build/`，测试 fixture 与报告归入 `06_test/`。工程未配置安装规则；系统有 `windeployqt-qt5 5.15.18`。`windeployqt` 默认 ANGLE dry-run 曾提示缺少 `libGLESv2.dll`；`--no-angle` dry-run 成功。隔离 smoke bundle 曾用于 Release Settings、shell、中文路径/SQLite 与 Qt offscreen 抓图合同验证；该 bundle 含测试程序、fixture 和截图，不是交付物。

对旧 Release 主程序执行 `objdump -p` 的静态检查结果：PE 子系统为 Windows GUI，子系统版本字段为 5.2；直接导入 DWM API、MinGW GCC/C++ 运行库及 Qt5 Core/Gui/Sql/Widgets。PE 字段和导入模块只记录二进制构成，不表示操作系统兼容性。旧 portable smoke 目录曾包含 staging 数据、截图和测试程序，已按清理要求删除；当前测试输入和 Qt 抓图只保留在 `06_test/` 的 staging 目录中。

隔离候选指纹（本轮最终构建）：Debug exe 45,363,333 字节，SHA-256 `DB1963F8FA72CB5AE11969EB48B80B7660294A339DFCC9B6D397EEA77F5BB619`；Release exe 2,116,237 字节，SHA-256 `FE02EF80DA5FA04C73309E21A799116B5B4D4EBA35D9FC654C1590AE90CD6A73`。二者均为裸构建产物，不是可分发安装包。

旧版本地交付记录已于 2026-09-23 被最新方案 B 产品替换。当前状态和文件指纹以 `02_process/task.md`、`05_product/` 下唯一 ZIP 为准；当前程序未访问真实数据。测试数据仅包含 `06_test/` 中隔离的 staging fixture 与本地抓图证据。

2026-09-23 更新：MiSans 正式资源内嵌于 Release 程序，设置中心提供字号和三种字重预览选择。首页无进行中任务时显示“目前没有任务，请添加任务”。Release 设置专项 1/1、Debug 与 Release 全量 CTest 各 19/19 通过；Qt 原生抓图合同覆盖八个页面、日期弹层、下拉控件与 125/150/200% `QT_SCALE_FACTOR`。最新 ZIP 共 63 项，含主程序、Qt/MinGW 依赖和 README；DLL 导入闭包无缺失，隔离启动成功，ZIP 内无测试程序、数据库、日志或截图。哈希见 `task.md`。系统选择器、物理键盘和 OS DPI 未实际验证，也不作为本地打包条件。

2026-09-24 更新：方案 B 任务详情标题/操作布局与分类标志、五个列表页正文字号、首页分类标志、任务管理分类/天数列宽及标题栏拖动命中区完成修正。相关合同 Debug/Release 各 8/8，全量 CTest 各 19/19；详情抓图位于 `06_test/data/v3-staging-debug/task-dialog/task-detail-dialog.png`。从最终 ZIP 解包后使用隔离 Data 和窄 PATH 启动 3 秒通过。最新产品仍使用 `NIUMA-Scheme-B-v3-20260924.zip`，63 项，67,414,934 字节，SHA-256 `FBFAE906702319BBD9AF788DCA67C5661C9E05DA9EDD1B4D0F9DA5C2F1C61247`。本轮未操作真实 Data；Qt 合同不作为物理鼠标拖动或 OS DPI 实测。

2026-09-24 字体/首页/任务详情修正：所有可见控件统一 MiSans，Qt 抓图字体族审计为 0 个不匹配；界面源码显式字体族扫描仅剩主题回退逻辑。任务详情标题与“打开文件夹”位置、分类标志尺寸、重点标记颜色已修正；分类标志复用任务列表的 30×24px、14px 字形和 8px 内边距。首页专注连续添加顺序、固定高度和滚动位置合同通过。Debug/Release 相关合同各 8/8、全量 CTest 各 19/19。从 Release 包目录和最终 ZIP 解包目录分别窄 PATH 隔离启动 3 秒通过；测试数据仅位于 `06_test/`。最终 ZIP 63 项、67,415,028 字节，SHA-256 `6778BD93ACDFC13E18FB85FC4375020724F771A47ABCE89198E9C5E727F3F8D2`。无活动桌面；截图为 Qt `QWidget::grab()` 证据，不代表系统选择器、物理输入或 OS DPI 实测。

2026-09-24 首页专注与任务详情修正：`niuma_v3_home_contract`、`niuma_v3_task_dialog_contract` Debug/Release 各 2/2；Debug 与 Release 完整 CTest 各 19/19。首页合同确认分类标志按 MiSans 字宽扩展、垂直居中，专注列表顶端排列，切换专注任务后标题几何位置、固定卡片高度和滚动值不变。详情合同确认标题为常规 18px 黑色，“打开文件夹”位于办理周期右侧并与完成时间下方对齐。`/simplify` 复核确认星标状态样式共用更新函数，避免全页重建；无其它高置信行为保持简化项。Release staging 与 ZIP 解包程序分别从 `06_test/data` 隔离路径启动 3 秒并创建 fixture 数据库；未操作真实 Data。最新产品 `05_product/NIUMA-Scheme-B-v3-20260924.zip` 共 63 个条目、67,416,209 字节，SHA-256 `D559C6C45A0BFCEFCA79BC86DAEA6A9ADAA9C235C907BD15066D9E1A53CE0C05`；包内无数据库、日志、抓图或合同测试程序，`05_product/` 仅保留此 ZIP。视觉检查使用 Qt `QWidget::grab()`，不代表物理桌面、系统选择器、输入设备或 OS DPI 验收。

2026-09-24 首页分类列与详情标题修正：Debug/Release 的首页、任务列表、任务详情相关合同各 3/3，完整 CTest 各 19/19。详情标题为 20px 常规黑色 MiSans。首页进行中分类表头与行单元格宽度均为 48px；标志按任务列表 pill 规则居中绘制。`/simplify` 未发现额外高置信行为保持简化项。Release ZIP 解包后启动 3 秒通过，数据路径位于 `06_test/data`；未操作真实 Data。最新 ZIP `NIUMA-Scheme-B-v3-20260924.zip` 63 个条目、67,417,375 字节，SHA-256 `37B092C7DD6F6AA9B73AC64D1466300EC981111E12FF6D9C2260E22EB8DEC04E`；无数据库、日志、截图和测试程序，`05_product/` 仅保留该产品包，版本号不变。Qt 抓图不代表物理桌面、系统选择器、输入设备或 OS DPI 验收。

2026-09-24 最新修正：详情标题为 MiSans Normal 22px 黑色。首页分类列与任务管理同为 48px，分类标志共用直接绘制代码。内置 MiSans Normal 与 MiSansLatin Normal 后，Qt 运行时字体合同实测默认字形权重 305；此前为 380。Debug/Release 相关专项各 9/9、完整 CTest 各 19/19。`/simplify` 清除未使用的分类样式映射、无效参数和重复字体遍历。最新唯一交付 ZIP 63 项、72,984,275 字节，SHA-256 `C601F4DAE30885FAA5A95352EB171F2205D2F51E35063A28C0B7911E88FD44B5`；解包后隔离启动 3 秒并在 `06_test/data/v3-package-zip-smoke-20260924-normal/Data` 创建 fixture 数据库。未触碰真实 Data；Qt 抓图和缩放因子不代表物理桌面或 OS DPI 实测。
