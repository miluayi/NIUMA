# S00 · v2 与方案 B 冻结基线

冻结日期：2026-09-22。用途：S01 创建受控 v2 源码快照前的只读比对，不是生产代码复制清单。源仓库为 `D:/project/05_workspace_v2/04_program`；不得复制 `build/`、`Data/`、日志、备份、工作目录或附件。

## 受控源码范围

| 目录/文件 | S01 处理 | 原因 |
| --- | --- | --- |
| `src/main.cpp` | 逐项移植为 v3 正式入口 | 含数据库初始化、工作目录、备份、跨天专注、回收站清理与退出导出；不能使用预览入口替代。 |
| `ui/`、`ui/widgets/` | 受控快照 | 八页、详情、弹窗、标题栏、消息、表格委托、图表、拖放与分页。 |
| `models/`、`database/`、`service/`、`utils/` | 受控快照 | 维持 v2 schema、状态、编号、文件与业务语义；后续逐域拆分。 |
| `resources/resources.qrc`、`resources/icons/`、`resources/styles/neutral_modern.qss`、`resources/app.rc` | 受控快照 | 方案 B 以 v2 基础 QSS/资源为起点。 |
| `tests/phase3_e2e_test.cpp` 至 `phase8_e2e_test.cpp` | 只读迁移候选 | S01 评估并重接到隔离 fixture；不是已通过的 v3 测试。 |
| `CMakeLists.txt` | 只读参考 | S01 改用 v3 显式 source list，不复制 v2 的 glob/重复 target 结构。 |

## 冻结指纹

| 文件 | SHA-256 |
| --- | --- |
| v2 `CMakeLists.txt` | `7251AAB0AC46D1EE309BF59418E62C7E09B1CF9747294F8672EC17B2618DC626` |
| v2 `src/main.cpp` | `2DDFA3E8BC171FE17BDD67DA1663CD7654AEF6623F5C1583110D0CE0DF1810F7` |
| v2 `resources/resources.qrc` | `D4113BDD03B376900B0B05DA2E16AE9C4BD797AD582A4BBB050CDB74AE9E14BF` |
| v2 `resources/styles/neutral_modern.qss` | `E804F296C53C639643C1C686D1DEB91B8D1D52124A6F7E13503161A569E69FAA` |
| B `04_program/ui_preview/SchemeBStyler.cpp` | `EB9B815C461A28D6E205C372FA271C368F378DA7D05A895A26427C209C60D253` |
| B `04_program/ui_preview/SchemeBStyler.h` | `125F98770C61C6F447C5C8518CBC2919C8FEE5FC09BAE3FF920C2790B669615E` |
| B `04_program/ui_preview/v2_source_preview_main.cpp` | `B35C0A6D30D5A30762A56260E08DD5ADAE7AF288948F8C30AD1B4024EDE24C12` |
| B `styles/scheme_b.qss` | `3E279DFC5966A9C3A601C4DFA16065072D5F6371DC4EF77287407CD8ABC2A06F` |

## 数据与功能契约

- 不改 SQLite schema、`进行中/已完成/已删除/他人办理` 等状态字符串、任务编号、日期/金额存储、六个工作目录或 `.niuma` 目录包。
- 真实数据始终关闭；预览入口 `v2_source_preview_main.cpp:49-114` 会清空并写入专属假库，只能用于预览/测试。
- 正式入口必须保留 v2 `src/main.cpp:46-118` 的初始化和退出语义，并在 staging 演练；S16 前不允许对真实库原地操作。
- 功能范围是八个页面、任务/周期/资料/成果/扫描的全部弹窗、确认框和系统文件/目录选择链，详见 `03_ui_design/SCHEME_B_PRODUCTION_MIGRATION_CHECKLIST.md`。

## 验证状态与下一动作

- 已验证：上述 B 四项指纹与 `FINAL_UI_HANDOFF.md` 一致；v2 主窗体、页面、弹窗的静态对应关系已盘点。
- 基线阶段未验证：v3 正式构建、v3 fixture、v3 E2E；后续的本地开发/测试状态以 `task.md` 与 S16 执行结果为准。
- S01 首先重新计算快照文件哈希并写入受控 manifest；若任一冻结文件已变化，先复审 B 视觉/功能影响，再复制源码。
