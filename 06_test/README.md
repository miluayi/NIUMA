# 06_test

只存隔离测试输入与可再生的测试结果。禁止读取、复制或修改真实用户 `Data/`。

```text
data/<build-profile>/       各 CTest 的 staging fixture 和数据库
evidence/<build-profile>/   Qt 抓图与桌面测试结构化证据
reports/                    CTest 日志和验证报告
```

CTest 路径由 `04_program/CMakeLists.txt` 指向 `06_test/data` 与 `06_test/evidence`。`data/`、`evidence/`、`reports/` 和测试工作目录可在测试结束后清理；再次运行测试会重新创建。测试程序源码保留在 `04_program/tests`。

Windows 的 native_input 和 shell 合同包含系统鼠标输入，须在可交互桌面串行运行，不同时操作其他应用；点击前会校验命中窗口属于测试进程。仅调用控件 click 或发送窗口消息不能替代这些检查。

legacy_chrome/legacy_navigation/legacy_visual 合同在测试进程强制进入 Windows 7/8/10 的软件绘制分支，验证透明渐变圆角、旧主题下的窗口按钮、缩放还原及输入。windows7_import_audit 检查程序和交付依赖的静态导入及 PE 子系统版本；均不能替代 Windows 7 实机验证。
