# 方案 B · 2026-09-22 定稿回归材料

以下图片由当日编译的 `NiumaV2SourcePreview.exe` 在独立假数据环境中自动生成。它们是视觉核对材料；设计源代码现位于 `04_program/ui_preview/`，QSS 与方案 B 资源位于 `03_ui_design/styles/`，页面行为基于 v2 Widgets 源码。

| 文件 | 对应界面 |
| --- | --- |
| `page-0.png` | 首页（顶部） |
| `home-bottom-page1.png`、`home-bottom-page2.png` | 首页下半部及分页 |
| `page-1.png`—`page-7.png` | 任务管理、周期任务、参考资料、年度成果、统计、回收站、设置中心 |
| `new-task.png`、`new-task-calendar.png` | 新建任务及日期面板 |
| `new-reference.png` | 新建资料 |
| `task-detail.png`、`task-detail-long.png` | 任务详情及长标题 |
| `calendar.png`、`calendar-input.png` | 中文日期选择器 |
| `combo-popup.png` | 下拉弹层圆角 |
| `refresh-toast.png` | 刷新完成悬浮提示 |
| `focus-before.png`、`focus-after.png` | 首页第一次加入专注任务前后 |

本轮重建、全部截图与自动验证命令的退出码均为 0。`font-and-controls.txt` 显示可见控件 MiSans 字体不匹配数 0、14 个下拉框缺失箭头数 0、9 个勾选框缺失对号数 0。其余 `*.txt` 记录圆角、专注区位置、侧栏缩放、嵌套／横向滚轮、平滑滚动及任务详情的自动检查。

这些检查不覆盖所有编辑、删除确认、导入等弹窗的完整人工交互，也不等同于正式版功能回归。图中的日期和期限随运行日变化，后续对比应关注结构与语义状态而非像素级日期文本。
