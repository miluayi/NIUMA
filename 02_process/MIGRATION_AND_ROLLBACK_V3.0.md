# V2 → V3 迁移与回滚

## 总原则

UI 重构期不改变 v2 的数据库 schema、状态字符串、编号规则、工作目录结构或 `.niuma` 格式。v3 不直接打开或修改唯一一份真实数据；所有验证先在副本上完成。

## 分阶段迁移

### Phase 0：冻结基线

- 保存 v2 可执行文件、依赖清单、CMake 缓存摘要与关键流程证据。
- 生成一份不可覆盖的 pre-v3 备份，并建立脱敏 v2 数据 fixture。
- 记录 schema 指纹：表、列、索引、`settings.db_version`、`PRAGMA user_version`、完整性与外键检查。
- 明确实际工作目录名与 `.niuma` 目录包契约，不能只依赖 PRD 描述。

### Phase 1：安全骨架

- 原样冻结可复用的 domain/database/service/utils 行为。
- 拆分 CMake target；引入可注入 `DataPathProvider`，区分真实、便携、测试路径。
- 将现有 Phase 3–8 流程改造成可重复 CTest 基线。

### Phase 2：设计系统与母版

- 批准主窗口、首页、列表、表单、详情与系统消息母版。
- 完成 token/QSS、四态数据容器、键盘与 DPI 基线。

### Phase 3：任务垂直切片

- 迁移 MainWindow、任务列表、创建、详情、编辑。
- 先用 application facade 隔离 DAO，再接 v2 fixture 跑创建→查找→编辑→完成→取消完成→删除→恢复。
- 旧/新页面保留短期切换能力，便于阶段回滚。

### Phase 4：其余页面

按首页 → 周期任务 → 参考资料 → 年度成果 → 统计 → 回收站 → 设置迁移。每页必须通过功能、键盘、状态、视觉和数据兼容验收后再替换旧页。

### Phase 5：数据与备份加固

若未来必须变更 schema，引入 `schema_migrations(id, checksum, applied_at)`，先按真实表/列指纹识别 v2 baseline，再事务迁移。恢复流程必须先验证 manifest/hash、支持的 schema、`integrity_check` 与 `foreign_key_check`。

### Phase 6：发布切换

- 在真实数据的副本上完整演练升级、启动、关键流程、备份、恢复与失败回滚。
- 验证中文路径、Qt 插件、不同 DPI、Win7 SP1 目标环境和绿色版部署。
- 通过后才允许 v3 使用正式 Data。

## 首次读取真实库的安全流程

```text
真实库保持关闭
  → 创建不可覆盖 pre-v3 snapshot
  → 复制到 staging
  → 校验 hash / schema 指纹 / integrity / foreign keys
  → 在 staging 执行迁移
  → 启动与业务 smoke test
  → 原子替换或切换数据指针
  → 失败则丢弃 staging，真实库不变
```

禁止原地试探性迁移。若 schema 发生 forward-only 变化，v2 不得再打开已迁移库；回滚只能恢复 pre-v3 snapshot。

## 生产数据禁区

不得从 v2 复制以下内容到 v3 源码、原型或普通测试目录：`05_product/Data/`、`database.db`、WAL/SHM、日志、自动/手动备份、工作目录与附件。只有经过人工审查的虚构/脱敏 fixture 可以位于 `06_test/fixtures/`。

## 已知风险

1. Win7 与 Qt 6 不可兼得；本轮选择 Qt 5 兼容路线并接受维护成本。
2. v2 UI 直连 DAO，迁移界面时可能意外复制业务编排。
3. v2 `db_version` 与真实 schema 演进不完全一致，不能只看版本值。
4. 备份 manifest 缺少 schema 版本，恢复前验证不足。
5. 文档声明 MSVC 静态发布，当前实际开发构建是 MinGW 动态 Qt；发布工具链需另行冻结和验证。

