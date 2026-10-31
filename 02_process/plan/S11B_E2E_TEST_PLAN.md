# S11-B 设置维护危险操作测试方案

## 范围与安全边界

所有文件、数据库、备份包和扫描目录都限制在 `06_test/data/v3-staging-debug/settings-maintenance`。不调用系统文件选择器，也不读取、复制或覆盖真实 `Data`。

## 自动化契约

| 编号 | 场景 | 断言 |
|---|---|---|
| C05 | 备份与恢复 | 显式 fixture 目录生成 `.niuma`；错误路径失败不改数据；删除数据后恢复备份，基线任务回归。 |
| C06 | 扫描导入 | 虚构目录扫描为待导入项，预览表正确，确认导入后任务入库。 |
| C07 | 初始化 | 二次确认取消保持数据；确认后才清空任务与设置。 |
| E01 | 桌面冒烟 | pywinauto 验证无框窗口导航、窗口控制、快捷键和关闭。 |

## 执行命令

```powershell
& 'C:\msys64\mingw64\bin\cmake.exe' --build --preset v3-staging-debug
& 'C:\msys64\mingw64\bin\ctest.exe' --preset v3-staging-debug --output-on-failure
```

## 执行结果（2026-09-23）

- `NiumaV3SettingsMaintenanceContractTest` 在 `staging-fixture/settings-maintenance` 内生成备份包、删除并恢复 fixture 数据库，扫描虚构工作目录并导入一项任务，最后验证软件初始化的取消与确认路径。
- `v3-staging-debug` 全量构建成功，CTest 13/13 通过；pywinauto 无框 Qt E2E 成功，结构化证据见 `06_test/evidence/v3-staging-debug/qt-desktop-e2e-session.json`。
- BitBlt 截图仍不可用；`page-7` 与扫描/恢复/初始化确认的像素验收留 S12。
