# NIUMA V3.1.4 仓库

本仓库包含 NIUMA（铁军牛马任务日志）V3.1 的产品文档、定稿设计、Qt5 Widgets 正式源码和测试源码。公开仓库：[miluayi/NIUMA](https://github.com/miluayi/NIUMA)。当前产品行为见 [01_docs](01_docs/README.md)，逐轮验证记录见 [02_process/task.md](02_process/task.md)。

## 目录

| 目录 | 内容 |
| --- | --- |
| `01_docs/` | 当前 V3.0 产品、技术、备份恢复和维护文档 |
| `02_process/` | 历史决策、开发过程和已验证的任务记录 |
| `03_ui_design/` | 已定稿的 UI 规范、预览源码依据和设计回归材料 |
| `04_program/legacy_v2/` | 正式 C++17 / Qt 5.15 Widgets 源码 |
| `04_program/tests/` | CTest 合同测试源码 |
| `05_product/` | 本地保留唯一最新交付 ZIP；公开仓库仅保存交付说明 |
| `06_test/` | 隔离测试输入与可再生的生成结果 |

## 构建与验证

首次克隆后，先按 [字体准备说明](03_ui_design/MiSans%20Global%20_ALL/README.md) 从官方取得字体并放回指定路径。原始字体包不随公开仓库分发；校验清单记录当前构建所用的 9 个字体文件。

在仓库根目录使用 `CMakePresets.json` 中的 `v3-staging-debug` 或 `v3-staging-release` 配置、构建和运行 CTest。`04_program/build/` 为可再生构建树；`06_test/data/`、`06_test/evidence/` 和 `06_test/reports/` 为可再生测试输出，清理后测试会重新创建。
构建需要 Qt5 Svg 开发模块；图标工具从统一 SVG 在构建目录中生成 ICO，不修改源码中的二进制资源。关闭图标采用 Microsoft Fluent UI，许可说明随交付包附带。
Windows 构建另需与 Qt5 Gui 配套的私有开发头文件，用于同步自定义边框的 Qt 几何缓存；交付包不增加运行时 DLL。

真实用户 `Data/`、工作目录和备份不得进入仓库、测试目录或交付 ZIP。当前交付包见 `05_product/NIUMA-V3.1.4-20261031.zip`，运行文件始终为 `Niuma.exe`，不随版本变化。当前为 3.1.4；后续优化依次递增至 3.1.9，再升级为 3.2.0。

Windows 11 使用系统圆角，Windows 7/8/10 使用带抗锯齿透明边缘的软件圆角及边框；窗口控制按钮完整自绘。兼容分支与静态导入审计属于本地检查，Windows 7 SP1 的实际主题、驱动及原生文件选择器仍需目标系统验证。

## 仓库维护

保留前期准备资料、历史决策、定稿设计与有效测试源码；构建树、隔离测试输出、系统缓存、真实数据和本地交付包不进入 Git。清理范围见 [2026-10-31 清理记录](02_process/REPOSITORY_CLEANUP_20261031.md)。仓库公开不代表授予额外的软件许可；第三方资源遵循各自许可，Fluent 图标许可保留于源码。
