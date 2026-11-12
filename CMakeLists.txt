cmake_minimum_required(VERSION 3.16)

project(ZosnTerm
    VERSION 0.1.0
    DESCRIPTION "个人多协议终端工具（阶段一：串口）"
    LANGUAGES C CXX)

# Set OFF to build only the platform-neutral PuTTY engine + headless smoke
# test. Useful for CI / toolchain validation on machines without Qt 6.
option(ZOSNTERM_BUILD_GUI "Build the Qt 6 / QML application (needs Qt 6.5+)" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
# Disable AUTOMOC predefs header: under Windows Esafenet DLP the generated
# moc_predefs.h (.h) is transparently encrypted and crashes moc.exe (0xC0000006).
set(CMAKE_CXX_COMPILER_PREDEFINES_COMMAND "")
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# PuTTY 0.85 terminal-emulation engine (vendored, MIT). Provides the static
# library `putty_engine`. Platform-neutral: Linux / MinGW / MSVC share one
# source set (see src/3rdparty/putty/CMakeLists.txt).
add_subdirectory(src/3rdparty/putty)

# Discard the large fraction of the PuTTY engine we never call (GUI dialogs,
# backend setup, etc.). The engine was compiled with per-function sections.
function(zosn_enable_dead_code_stripping target)
    if(MSVC)
        target_link_options(${target} PRIVATE /OPT:REF)
    elseif(APPLE)
        target_link_options(${target} PRIVATE -Wl,-dead_strip)
    elseif(UNIX OR MINGW)
        target_link_options(${target} PRIVATE -Wl,--gc-sections)
    endif()
endfunction()

if(ZOSNTERM_BUILD_GUI)
    find_package(Qt6 6.5 REQUIRED COMPONENTS
        Core Gui Quick Qml QuickControls2 SerialPort)
    qt_standard_project_setup()
# Opt into Qt6 modern CMake policies to silence QTP0001 / QTP0004 dev warnings.
qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)

    # 可执行目标名为 ZosnTermApp，避免与 qt_add_qml_module 的 URI(ZosnTerm)
    # 在构建目录生成的 ZosnTerm/ 子目录撞名导致链接失败。
    qt_add_executable(ZosnTermApp
        src/app/main.cpp
        src/app/ZSApplication.h
        src/app/ZSApplication.cpp
        src/terminal/ZSTerminalView.h
        src/terminal/ZSTerminalView.cpp
        src/terminal/ZSTerminalRenderer.h
        src/terminal/ZSTerminalRenderer.cpp
        src/session/ZSSession.h
        src/session/ZSSession.cpp
        src/session/ZSSessionManager.h
        src/session/ZSSessionManager.cpp
        src/session/ZSIConnection.h
        src/session/ZSISession.h
        src/engine/ZSPuTTYBackendAdapter.h
        src/engine/ZSPuTTYBackendAdapter.cpp
        src/engine/ZSQtSeat.h
        src/engine/ZSQtSeat.cpp
        src/platform/ZSPlatformSerial.h
        src/platform/ZSPlatformSerial.cpp
        src/platform/ZSPlatformPaths.h
        src/platform/ZSPlatformPaths.cpp
        src/data/ZSConfigStore.h
        src/data/ZSConfigStore.cpp
        src/data/ZSLogger.h
        src/data/ZSLogger.cpp
        src/data/ZSSessionRepository.h
        src/data/ZSSessionRepository.cpp
    )

    # QML 模块：外壳 QML + 终端视图 C++ 类型。URI=ZosnTerm。
    qt_add_qml_module(ZosnTermApp
        URI ZosnTerm
        VERSION 1.0
        QML_FILES
            src/qml/ZSMainWindow.qml
            src/qml/ZSSettingsDlg.qml
    )

    # 各业务子目录加入包含路径，使 #include "Xxx.h" 裸名可解析
    target_include_directories(ZosnTermApp PRIVATE
        src/app
        src/terminal
        src/session
        src/engine
        src/platform
        src/data
    )

    target_link_libraries(ZosnTermApp PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Quick
        Qt6::Qml
        Qt6::QuickControls2
        Qt6::SerialPort
        putty_engine
    )

    zosn_enable_dead_code_stripping(ZosnTermApp)
endif()

# Headless smoke test: proves the embedded PuTTY engine drives the draw
# callbacks without any Qt/GUI dependency. Links only putty_engine and supplies
# its own zs_view_* stubs (see tests/headless_smoke.cpp).
add_executable(zosn_headless_smoke tests/headless_smoke.cpp)
target_include_directories(zosn_headless_smoke PRIVATE src/engine)
target_link_libraries(zosn_headless_smoke PRIVATE putty_engine)
zosn_enable_dead_code_stripping(zosn_headless_smoke)
