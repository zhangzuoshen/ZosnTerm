# 第三方：PuTTY 0.85（vendored，MIT）

ZosnTerm 的串口/终端仿真引擎复用 PuTTY 的核心实现。

## 许可
- PuTTY 为 **MIT 许可**（非 GPL），内嵌无 copyleft 传染，仅需保留版权声明。
- 源码树根目录 `putty-0.85/LICENCE` 即原始 MIT 文本，已随源码一并 vendoring。
- 项目根 `NOTICE` 文件对本项目使用 PuTTY 做了致谢说明。

## 版本
- **锁定 0.85**（2026-08-16，当前最新稳定版）。升级需单列任务，见架构文档 D8。

## 同步方式
执行本目录下的脚本把官方源码拉取并解包到 `putty-0.85/`：

```bash
# Linux / macOS / Windows(Git Bash)
bash vendor_putty.sh

# 或 Windows PowerShell
pwsh -File vendor_putty.ps1
```

脚本从 PuTTY 官方镜像下载 `putty-0.85.tar.gz` 并解包到当前目录，下载源：
`https://the.earth.li/~sgtatham/putty/0.85/putty-0.85.tar.gz`

## 编译（M2 阶段启用）
PuTTY 自带构建系统（mkfiles / CMake）与 GUI 前端，我们**只取核心**，不编译其
`windows/`、`unix/` GUI 前端。M2 引擎适配阶段由根 `CMakeLists.txt` 通过
`add_subdirectory(src/3rdparty/putty)` 启用本目录的 `CMakeLists.txt`，把以下核心
源文件编译为静态库 `libputty`：

- `serial.c`（含 `serial_win32_init` / `serial_unix_init`，跨平台串口）
- `terminal.c`（VT100/xterm 仿真、屏幕缓冲、绘制回调）
- `ldisc.c`（本地回显、特殊键处理）
- `settings.c` / `config.c`（PuTTY `Config` 结构体）
- `timing.c` / `network.c`（后端需要的定时器与 socket 抽象）
- `misc.c` / `tree234.c` / `utils.c` 等通用工具

关键编译定义（CMake `target_compile_definitions`）：
- Linux: `UNIX_UNIX`；Windows: `WINDOWS`
- 关闭 PuTTY 自带前端：不定义 GUI 相关宏，由本项目 `ZSQtSeat` 充当 `Seat`。

## 目录结构
```
src/3rdparty/putty/
  README.md            # 本文件
  vendor_putty.sh      # 下载并解包 PuTTY 0.85（bash）
  vendor_putty.ps1     # 下载并解包 PuTTY 0.85（PowerShell）
  CMakeLists.txt       # M2 启用：编译 libputty 静态库（当前为占位）
  putty-0.85/          # 由脚本生成的官方源码树（含 LICENCE）
```

> 注意：`putty-0.85/` 由脚本生成，已纳入版本管理（vendoring 不随网络漂移）。
> 若需重新拉取，删除 `putty-0.85/` 后重跑脚本即可。
