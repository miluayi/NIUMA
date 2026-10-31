# legacy_v2 受控快照清单

建立日期：2026-09-22。来源：`D:/project/05_workspace_v2/04_program`。本快照仅供 v3 正式迁移构建，不含 `build/`、`Data/`、日志、备份、工作目录或附件。

## 完整性核对

- 来源非 `build`/`Data` 文件：141 个；其中 `CMakeLists.txt` 有意不复制，v3 使用 `04_program/CMakeLists.txt` 的显式 source list。
- 已复制文件：140 个；其中 138 个与来源 SHA-256 一致。
- 受控差异仅有两项：
  - `utils/PathUtil.cpp`：`NIUMA_DATA_DIR` 非空时返回规范化的注入 staging 数据目录；为空时保持 v2 `{appDir}/Data` 行为。
  - `tests/phase3_e2e_test.cpp`：`NIUMA_TEST_ROOT` 非空时使用注入 staging 目录；为空时保持 v2 Qt 临时目录行为。
- 快照中不存在 `build/` 或 `Data/` 目录。

## 验证命令

在仓库根目录运行以下只读比对；预期缺失仅 `CMakeLists.txt`，差异仅上述两项：

```powershell
$sourceRoot='D:\project\05_workspace_v2\04_program'
$snapshotRoot='D:\project\08_workspace_v3\04_program\legacy_v2'
Get-ChildItem -LiteralPath $sourceRoot -Recurse -File |
  Where-Object { $_.FullName -notmatch '\\(build|Data)(\\|$)' }
```

任一额外差异必须先登记来源、理由、负责阶段和回归证据，才能继续迁移。

## 2026-10-31 仓库清理补记

以上为 2026-09-22 的原始快照记录，不是当前源码的逐文件一致性声明；后续正式迁移与修复见 `02_process/task.md`。本轮移除未被现行 CMake 引用的 phase2、phase4、phase5、phase6_7、phase8 旧测试文件，以及已由统一 SVG 构建生成替代的旧 `resources/icons/app-icon.ico`。继续保留现行 phase3 staging 测试、`04_program/tests` 全部有效合同与历史设计预览源码。清理范围及验证见 `02_process/REPOSITORY_CLEANUP_20261031.md`。
