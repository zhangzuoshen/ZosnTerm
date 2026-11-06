# ZosnTerm — vendored PuTTY 0.85 静态库（libputty）
#
# 本文件在 M1 阶段不参与编译；M2 引擎适配阶段由根 CMakeLists.txt 通过
#   add_subdirectory(src/3rdparty/putty)
# 启用。
#
# 编译范围（仅核心，不含 windows/ unix/ GUI 前端）：
#   serial.c terminal.c ldisc.c settings.c config.c timing.c
#   network.c misc.c tree234.c utils.c sshkeys.c(未来) ...
#
# 关键编译定义（target_compile_definitions）：
#   - UNIX_UNIX (Linux) / WINDOWS (Windows)
#   - 关闭 PuTTY 自带前端：不定义 GUI 相关宏
#
# 许可：PuTTY 为 MIT，保留 LICENCE 与源码头部声明，禁止修改其许可。

# ---- M2 启用示例（届时取消注释并按依赖裁剪文件列表）----
# add_library(putty STATIC
#     putty-0.85/serial.c
#     putty-0.85/terminal.c
#     putty-0.85/ldisc.c
#     putty-0.85/settings.c
#     putty-0.85/config.c
#     putty-0.85/timing.c
#     putty-0.85/network.c
#     putty-0.85/misc.c
#     putty-0.85/tree234.c
#     putty-0.85/utils.c
# )
# target_include_directories(putty PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/putty-0.85)
# if(UNIX)
#     target_compile_definitions(putty PRIVATE UNIX_UNIX)
# else()
#     target_compile_definitions(putty PRIVATE WINDOWS)
# endif()
# target_link_libraries(ZosnTerm PRIVATE putty)
