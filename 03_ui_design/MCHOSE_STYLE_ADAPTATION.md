# NIUMA HUB 设计适配 v0.2（待视觉定稿）

本文件是当前有效的视觉方向，基于 `REUSE_GUIDE_for_AI.md` 的设计范式适配到 NIUMA；不使用 MCHOSE 的商标、图标、产品渲染或其他品牌资产。

## 产品与任务

- 产品：面向个人/小团队的本地任务台账与工作记录工具。
- 主要任务：查看今日优先事项、筛选任务、查看状态和截止时间、随后进入任务详情与表单。
- 约束：正式产品与默认设计模拟器均为 C++17 + Qt 5.15 Widgets；使用假种子数据，不接入生产数据，直至视觉方案确认。

## 视觉拨盘

| 维度 | 当前选择 |
|---|---|
| 密度 | 中等：表格保留信息密度，首页保留视觉留白 |
| 对比度 | 浅灰工作区 + 白色表面 + 深色正文 |
| 几何 | 4 / 6 / 8 / 16px 圆角；大面板 16px |
| 温度 | 冷静蓝灰，单一品牌蓝 |
| 动效 | 只保留 0.2–0.3s 的状态反馈，当前原型不以动效传达信息 |
| 页面中心 | “今日任务”作为实体，由日期圆牌、档案卡和状态组成 |

## 设计令牌

| 类型 | Token | 值 |
|---|---|---:|
| primitive | `brand` | `#0053E2` |
| primitive | `brandHover` | `#005CE6` |
| primitive | `canvas` | `#F2F3F7` |
| primitive | `surface` | `#FFFFFF` |
| primitive | `textMain` | `#151515` |
| primitive | `textSecondary` | `rgba(21,21,21,.5)` |
| primitive | `border` | `#D7DEE3` |
| primitive | `hover` | `#E5E9F0` |
| semantic | `selection` | `#D8DAE3` |
| semantic | `success` | `#32AA70` |
| semantic | `danger` | `#EA5E56` |
| semantic | `warning` | `#F59C25` |

控件高度统一 40px（Qt 的样式内为 38px 内容高度 + 2px 边框），字体采用 `Microsoft YaHei / PingFang SC / Arial`。

## 组件契约

- 侧边导航：272px；项目高 40px；选中项为灰蓝底 + 品牌蓝文字；键盘 focus 有品牌描边。
- 输入/下拉：白底、6px 圆角、40px 节奏；hover 加深边框，focus 为 2px 品牌蓝边。
- 按钮：104px 最小宽度、8px 圆角；每个局部决策区最多一个蓝色 primary。
- 任务对象卡：首页顶端大圆角白卡；日期圆牌为视觉实体，档案 01/02 是当前可执行对象，空档案给出补入动作。
- 表格：白表面、40px 表头、45px 行高；hover/selected/focus 都可见；状态同时提供文字和颜色。
- 状态面：任务页保持 content / empty / loading / error；错误状态给出原因与重试动作，不能只显示错误。

## 响应式与可访问性

- 1440×900：左侧导航 + 首页对象卡 + 表格/辅助信息双栏。
- 940×580：底部工作区摘要隐藏；导航保持完整；首页内容纵向可达；任务表隐藏非关键列，不依赖横向滚动。
- 原型与 Designer 都必须保留关键控件的 accessible name 和 Tab 顺序；focus 不能被颜色或静态装饰替代。

## 设计边界

本轮可更新 `forms/MainWindow.ui`、`styles/theme.qss`、Qt5 模拟器和设计文档；确认后才接入生产代码。绝不改变生产数据结构、业务服务、DAO 或文件归档逻辑。

V2 功能保留的完整清单与证据见 `../02_process/V2_FEATURE_PRESERVATION_MATRIX.md`。新增顶栏工具只能映射既有能力；当前是刷新（F5）和设置中心，未凭空增加主题切换、云同步等业务能力。
