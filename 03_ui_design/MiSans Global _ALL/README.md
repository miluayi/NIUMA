# MiSans 字体准备

本地开发资料保留原始字体；公开 Git 仓库仅包含本说明及 `font-sources.json`，不单独再分发字体原包。

从 [MiSans 官方下载页面](https://hyperos.mi.com/font/zh/download/) 获取 MiSans、MiSans Latin 和 MiSans L3，阅读并遵守 [官方知识产权许可协议](https://hyperos.mi.com/font-download/MiSans字体知识产权许可协议.pdf)。将原包解压到本目录，保持 `font-sources.json` 所列相对仓库根目录的路径。正式资源入口为 `04_program/legacy_v2/resources/fonts.qrc`，共引用 9 个文件。

构建前可在仓库根目录执行以下只读校验；清单中的字节数与 SHA-256 对应本项目当前使用的字体版本。官方下载版本若变化，先核实来源和字体行为，再更新清单。

```powershell
$fonts = Get-Content -LiteralPath '03_ui_design/MiSans Global _ALL/font-sources.json' -Raw | ConvertFrom-Json
foreach ($font in $fonts) {
    $file = Get-Item -LiteralPath $font.path -ErrorAction Stop
    if ($file.Length -ne $font.bytes -or (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash -ne $font.sha256) {
        throw "字体校验失败：$($font.path)"
    }
}
```

MiSans 为小米提供的第三方字体。使用、嵌入和分发时应保留相关版权及许可信息，并按官方协议标明 MiSans；本说明不替代原始许可。
