# 04_program

正式源码位于 `legacy_v2/`，使用 C++17、Qt 5.15 Widgets、SQLite 和 CMake。`tests/` 保存 CTest 源码；`ui_preview/` 是设计定稿时的历史预览源码，不进入正式 target。可再生的构建树放在 `build/`，交付包放在 `../05_product/`，测试输出放在 `../06_test/`。

## 当前代码结构

- `legacy_v2/ui/`：主窗口、页面、弹窗和控件。
- `legacy_v2/application/`：任务、资料、设置、回收站等用例。
- `legacy_v2/service/`：目录移动、扫描、备份、info 文件和其他业务服务。
- `legacy_v2/database/`、`models/`、`utils/`：SQLite 访问、数据模型和工具。
- `legacy_v2/resources/`：正式程序的 QSS、图标与字体。

当前仍保留部分旧版 UI 与数据库的直接耦合；不要将历史目标架构描述成已完成的实现。详细现行行为见 `../01_docs/Technical_Design_V3.0.md`。

## 开发边界

1. `03_ui_design/` 的定稿资料是视觉与交互依据；正式 target 不编入预览入口、演示数据库或旧 Designer 表单。
2. 不在本目录放入真实数据库、工作目录、日志或用户备份；测试使用 `../06_test/` 的隔离路径。
3. 修改后先运行相关 CTest，再运行完整 CTest；界面变化检查 `QWidget::grab()`。Qt 抓图不等同于物理桌面或系统 DPI 验证。
4. 构建产物、Python 虚拟环境和本地依赖缓存均可清理重建，不作为正式源码。

需要查看真实实现时，以 `legacy_v2/` 和根目录 `CMakePresets.json` 为准；早期迁移计划存于 `../02_process/` 作为过程记录。
