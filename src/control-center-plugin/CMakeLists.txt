# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
# SPDX-License-Identifier: GPL-3.0-or-later
# 控制中心插件: meme
#
# 注意: 由于系统中未安装 dde-control-center-dev (CMake config 包),
#       我们手动实现 dcc_install_plugin 宏的等效逻辑:
#       1. 用 qt_add_qml_module 注册 QML 模块
#       2. 直接引用源码树中的 dccfactory.h 头文件
#       3. 安装到 plugins_v1.1/meme/ 目录

set(PLUGIN_NAME "meme")

find_package(Qt6 COMPONENTS Core Qml Gui Multimedia DBus REQUIRED)
find_package(Dtk6 COMPONENTS Core Widget REQUIRED)

# --- C++ 后端源文件 ---
set(PLUGIN_SRCS
    src/memeplugin.h
    src/memeplugin.cpp
    src/memethememanager.h
    src/memethememanager.cpp
)

# --- QML 文件 (直接在根目录,与 accounts 等已安装插件结构一致) ---
set(QML_FILES
    Meme.qml
    MemeMain.qml
    MemePreview.qml
)

# --- QML 模块 (生成 libmeme_qml.so) ---
qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)
cmake_policy(SET CMP0071 NEW)

set(plugin_dirs ${PROJECT_BINARY_DIR}/lib/meme)
qt_add_qml_module(meme_qml
    PLUGIN_TARGET meme_qml
    URI "meme"
    VERSION 1.0
    QML_FILES ${QML_FILES}
    OUTPUT_DIRECTORY ${plugin_dirs}
)

# --- C++ 后端 .so (通过 QPluginLoader 加载) ---
# 控制中心 DccPluginLoader 按 {name}.so 匹配,不能有 lib 前缀
add_library(${PLUGIN_NAME} MODULE ${PLUGIN_SRCS})
set_target_properties(${PLUGIN_NAME} PROPERTIES PREFIX "")

# DccFactory 符号在 libdde-control-center.so 中,插件必须链接它
find_library(DCC_LIBRARY NAMES dde-control-center)
if(NOT DCC_LIBRARY)
    message(FATAL_ERROR "libdde-control-center not found. Install dde-control-center-dev or dde-control-center.")
endif()

target_link_libraries(${PLUGIN_NAME} PRIVATE
    meme-common
    ${DCC_LIBRARY}
    Qt6::Core
    Qt6::Gui
    Qt6::Multimedia
    Qt6::DBus
    Dtk6::Core
    Dtk6::Widget
)

# 包含 dccfactory.h（从控制中心源码树）
target_include_directories(${PLUGIN_NAME} PRIVATE
    ${DCC_INCLUDE_DIR}
)

# --- 安装 ---
# 安装 .so 到插件目录
install(TARGETS ${PLUGIN_NAME}
    LIBRARY DESTINATION ${DCC_PLUGIN_DIR}/${PLUGIN_NAME}
)

# 安装 QML 模块到插件目录(整个 meme/ 目录复制到 plugins_v1.1/meme/)
install(DIRECTORY ${plugin_dirs}
    DESTINATION ${DCC_PLUGIN_DIR}
)
