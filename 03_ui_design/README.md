# NIUMA 方案 B · UI 定稿交付

2026-09-22 起，方案 B 是 v3 唯一的 UI 设计基线。正式程序已复用 Qt Widgets 源码并迁移方案 B 样式；本目录保留 UI 规范、原型和视觉历史基线，方案 A／C 不再作为更新目标。定稿内容是可运行 Qt Widgets 原型源码与 QSS，不是静态效果图或 Qt Designer `.ui` 表单。

## 交付入口

| 内容 | 位置 |
| --- | --- |
| 定稿范围、迁移次序和待决事项 | [FINAL_UI_HANDOFF.md](FINAL_UI_HANDOFF.md) |
| 视觉／组件规范 | [DESIGN_SYSTEM.md](DESIGN_SYSTEM.md) |
| 交互原型入口、假数据和自动验收源码 | `../04_program/ui_preview/v2_source_preview_main.cpp` |
| 方案 B 页面修饰与行为源码 | `../04_program/ui_preview/SchemeBStyler.cpp`、`../04_program/ui_preview/SchemeBStyler.h` |
| 方案 B QSS | `styles/scheme_b.qss` |
| 本次定稿回归材料 | [gallery/scheme-b-final-2026-09-22/README.md](gallery/scheme-b-final-2026-09-22/README.md) |
| v2 原始设计与差异审计 | `SCHEME_B_V2_BASELINE.md`、`SCHEME_B_V2_AUDIT.md`（历史参考，当前规范以本目录新定稿文件为准） |

## 运行真实交互窗口

字体原包仅在本地保留，公开仓库提供 [官方获取和校验说明](MiSans%20Global%20_ALL/README.md)。历史视觉回归材料属于定稿设计依据，继续保留；可再生的测试抓图位于 `06_test`，不进入 Git。

在仓库根目录执行：

```powershell
C:\msys64\mingw64\bin\cmake.exe --preset ui-preview-debug
C:\msys64\mingw64\bin\cmake.exe --build --preset ui-preview-debug --target NiumaV2SourcePreview
& 'D:\project\08_workspace_v3\04_program\tools\run_preview.ps1'
```

`04_program/tools/run_preview.ps1` 配置 Qt／MinGW 运行库路径，默认启动定稿预览。预览源码位于 `04_program/ui_preview/`，编译所需的 v2 原始工程位于 `D:\project\05_workspace_v2`；预览数据库仅位于 `06_test/data/ui-preview-debug/v2-preview-data-b/`，不连接正式数据。截图和预览报告写入 `06_test/evidence/ui-preview-debug/`，这些生成结果可清理重建。字体原始包保留在本目录，正式程序所需字体另由 `04_program/legacy_v2/resources/` 内置。正式产品位于 `05_product/`。
