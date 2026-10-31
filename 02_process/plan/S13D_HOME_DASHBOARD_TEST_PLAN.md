# S13-D 首页仪表盘 application 边界测试方案与结果

日期：2026-09-23（Asia/Shanghai）  
状态：已完成  

首页四区读取已收拢到 HomeDashboardUseCase；HomeCommandUseCase 委托今日专注及周期提醒命令。本切片不改变方案 B UI、v2 SQL 口径、真实数据或系统桌面行为。

| 要求 | 验证 |
| --- | --- |
| 四区读取 | HomeDashboard 提供今日专注、进行中页、周期提醒与本月统计。 |
| 列表口径 | 首页 fixture 验证 11 条进行中任务的截止排序、10 条分页、总数与两页。 |
| 月度统计 | fixture 验证创建日期口径的总数 13、完成数 2。 |
| 星标 | 首页合同验证 1/3 和滚动位置；application 合同直接验证 set/unset。 |
| 周期提醒 | HomeCommandUseCase 委托既有服务，MainWindow 保持警告、Toast 和全视图刷新。 |
| 架构门 | HomeView.cpp/h 静态搜索 DAO、DatabaseManager、SQL 和 service 依赖无命中。 |

测试仅使用 staging fixture。无活动桌面时不伪称覆盖系统选择器、物理键盘或 OS DPI；本切片没有视觉变更。

## 结果

2026-09-23：重点首页与 application 合同 2/2 通过；完整构建与 CTest 19/19 通过。simplify 移除了 application DTO 对 PeriodicTaskService 的公开类型泄漏，改为等字段的 HomePeriodicReminder，无行为修改。
