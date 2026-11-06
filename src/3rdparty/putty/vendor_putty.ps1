# 下载并 vendoring PuTTY 0.85 核心源码到本目录（MIT 许可，锁定版本）。
# 用法: pwsh -File vendor_putty.ps1
$ErrorActionPreference = "Stop"

$Here = Split-Path -Parent $MyInvocation.MyCommand.Definition
Set-Location $Here

$Ver = "0.85"
$Url = "https://the.earth.li/~sgtatham/putty/$Ver/putty-$Ver.tar.gz"
$Archive = "putty-$Ver.tar.gz"
$Dest = "putty-$Ver"

if (Test-Path $Dest) {
    Write-Host "已存在 $Dest/，跳过下载。如需重新拉取请先删除该目录。"
    exit 0
}

Write-Host "==> 下载 PuTTY $Ver : $Url"
try {
    Invoke-WebRequest -Uri $Url -OutFile $Archive -UseBasicParsing
} catch {
    Write-Host "错误：下载失败 - $_" -ForegroundColor Red
    exit 1
}

Write-Host "==> 解包到 $Dest/"
New-Item -ItemType Directory -Path $Dest | Out-Null
# 需要 tar（Windows 10+ 自带）解 .tar.gz
tar -xzf $Archive -C $Dest --strip-components=1
Remove-Item $Archive

Write-Host "==> 完成。源码位于: $Here\$Dest"
Write-Host "    M2 将从中挑选核心文件（serial.c/terminal.c/ldisc.c/...）编译为 libputty。"
