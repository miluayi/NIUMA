# 本地测试驱动

`qt_desktop_e2e.py` 是可选的 Windows 桌面验证脚本，不属于正式程序或 CTest。项目中的 `qt-e2e-venv/` 和 `pydeps/` 是可再生的本地环境与依赖缓存，不保留在仓库中。

需要运行该脚本时，可在本目录执行 `python -m venv qt-e2e-venv`，再用虚拟环境中的 `python -m pip install -r requirements-e2e.txt` 安装依赖。运行前先构建 `v3-staging-debug`，并通过 `ctest --preset v3-staging-debug` 在 `06_test/data/` 初始化隔离数据库与虚构工作目录。无活动桌面时，该脚本的截图与物理键盘步骤不能视为已验证。
