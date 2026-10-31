# S11-A 设置页面与配置迁移测试方案

## 范围与边界

本切片迁移 `SettingsView` 的方案 B 页面、卡片和稳定控件标识，验证三字典、导出设置、工作目录显示与备份/恢复/扫描请求信号。不会启动系统文件选择器、备份恢复、扫描导入或软件初始化；这些危险/文件操作留给 S11-B，并只在 fixture 中执行。

## 自动化契约

| 编号 | 场景 | 断言 |
|---|---|---|
| C01 | 页面结构 | 标题、目录、三字典、导出、数据维护与初始化入口可定位。 |
| C02 | 三字典 | 分别新增与删除来源、标签、办理人，并核对数据库和列表同步。 |
| C03 | 导出配置 | 保存标题、勾选、排序和金额单位；恢复默认后持久化默认值。 |
| C04 | 非破坏性请求 | 工作目录显示正确；更改目录、备份、恢复、扫描导入只派发对应信号。 |
| E01 | 桌面冒烟 | pywinauto 验证无框 Qt 程序导航、窗口控制、快捷键和关闭。 |

## 执行命令

```powershell
& 'C:\msys64\mingw64\bin\cmake.exe' --build --preset v3-staging-debug
& 'C:\msys64\mingw64\bin\ctest.exe' --preset v3-staging-debug --output-on-failure
```

## 执行结果（2026-09-22）

- 修复 v2 设置页三字典新增遗漏 `create_time` 的 schema 约束；使用既有 `DateUtil::currentDateTimeStr()`，不改变字典排序或删除语义。
- `v3-staging-debug` 全量构建成功，CTest 12/12 通过；`niuma_v3_settings_contract` 验证 C01–C04。
- pywinauto 无框 Qt E2E 成功：八页导航、940×580/1440×900、`Ctrl+N`、`F5`、`WM_CLOSE` 及窗口控制均通过；证据为 `06_test/evidence/v3-staging-debug/qt-desktop-e2e-session.json`。
- 当前 BitBlt 截图不可用；S11-B 的文件与危险确认操作、以及 `page-7` 像素回归仍未完成。
