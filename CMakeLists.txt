cmake_minimum_required(VERSION 3.16)

project(ZosnTerm
    VERSION 0.1.0
    DESCRIPTION "个人多协议终端工具（阶段一：串口）"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ---- Qt6 ----
find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core Gui Quick Qml QuickControls2 SerialPort)
qt_standard_project_setup()

# ---- 应用可执行文件（C++ 业务代码，均带 ZS 前缀）----
qt_add_executable(ZosnTerm
    src/app/main.cpp
    src/app/ZSApplication.h
    src/app/ZSApplication.cpp
    src/terminal/ZSTerminalView.h
    src/terminal/ZSTerminalView.cpp
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

# ---- QML 模块：外壳 QML + 终端视图 C++ 类型 ----
# ZSTerminalView 通过 qt_add_qml_module 随模块编译，QML 中以 <ZSTerminalView> 使用。
qt_add_qml_module(ZosnTerm
    URI ZosnTerm
    VERSION 1.0
    QML_FILES
        src/qml/ZSMainWindow.qml
)

target_include_directories(ZosnTerm PRIVATE src)

target_link_libraries(ZosnTerm PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Qml
    Qt6::QuickControls2
    Qt6::SerialPort
)

# ---- PuTTY 核心（vendored 0.85, MIT）----
# M1 仅 vendoring 源码（见 src/3rdparty/putty/）。
# libputty 静态库编译与可执行文件链接在 M2 引擎适配阶段启用：
# add_subdirectory(src/3rdparty/putty)
