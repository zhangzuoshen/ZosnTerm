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

find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core Gui Quick Qml QuickControls2 SerialPort)
qt_standard_project_setup()

# 可执行目标名为 ZosnTermApp，避免与 qt_add_qml_module 的 URI(ZosnTerm)
# 在构建目录生成的 ZosnTerm/ 子目录撞名导致链接失败。
qt_add_executable(ZosnTermApp
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

# QML 模块：外壳 QML + 终端视图 C++ 类型。URI=ZosnTerm，QML 中以 <ZSTerminalView> 使用。
qt_add_qml_module(ZosnTermApp
    URI ZosnTerm
    VERSION 1.0
    QML_FILES
        src/qml/ZSMainWindow.qml
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
)

# PuTTY 核心（vendored 0.85, MIT）。M1 仅 vendoring 源码。
# M2 引擎适配启用 libputty 静态库：add_subdirectory(src/3rdparty/putty)
