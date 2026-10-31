# S10 回收站迁移与 E2E 测试方案

## 范围与边界

S10 只迁移 v2 `RecycleBinView` 的方案 B 卡片、表格、空态和可定位交互元素；恢复、永久删除、清空与自动清理由既有 `TaskService` 执行。所有写入和目录操作限定在 `06_test/data/v3-staging-debug/recycle-bin`。

## 自动化契约

| 编号 | 场景 | 断言 |
|---|---|---|
| C01 | 空态 | 表格隐藏、空态可见，批量/清空操作禁用。 |
| C02 | 恢复 | 勾选恢复后恢复原状态，清空 `original_status` 与删除时间。 |
| C03 | 批量永久删除 | 二次确认取消不删除，确认后才物理删除。 |
| C04 | 单项永久删除 | 单项二次确认分别覆盖取消和确认。 |
| C05 | 30 天清理 | 删除满 30 天任务保留；超过 30 天才被清理。 |
| C06 | 清空 | 清空确认取消不改数据，确认后回收站为空。 |
| E01 | 桌面冒烟 | pywinauto 驱动无框 Qt 程序验证导航、窗口控制、快捷键和关闭。 |

## 执行命令

```powershell
& 'C:\msys64\mingw64\bin\cmake.exe' --build --preset v3-staging-debug
& 'C:\msys64\mingw64\bin\ctest.exe' --preset v3-staging-debug --output-on-failure
```

当前桌面驱动不依赖浏览器；若 BitBlt 截图不可用，保留 JSON 操作证据，`page-6` 像素回归仍由 S12 完成。

## 执行结果（2026-09-22）

- `v3-staging-debug` 全量构建成功，CTest 11/11 通过；`niuma_v3_recycle_bin_contract` 在隔离数据库和虚构工作目录验证 C01–C06。
- pywinauto 无框 Qt E2E 成功：八页导航、940×580/1440×900、`Ctrl+N`、`F5`、`WM_CLOSE` 及窗口最小化/最大化/还原/关闭均通过；证据见 `06_test/evidence/v3-staging-debug/qt-desktop-e2e-session.json`。
- BitBlt 截图仍不可用；结构化操作证据不替代 `page-6` 的像素验收，该门禁留在 S12。
