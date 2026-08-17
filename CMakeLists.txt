# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
# SPDX-License-Identifier: GPL-3.0-or-later
cmake_minimum_required(VERSION 3.23)

project(deepin-meme-plugin
    VERSION 3.1.0
    DESCRIPTION "Deepin dynamic wallpaper plugin"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# ─── DCC (dde-control-center) 头文件路径 ───
# 默认使用系统安装的 dde-control-center-dev 包提供的 /usr/include/dde-control-center。
# 如需用 in-tree 源码 checkout 的头文件（开发期拿到最新未发布版本），请显式提供：
#   cmake -B build -DDCC_SOURCE_ROOT=/path/to/dde-control-center
#   或: export DCC_SOURCE_ROOT=/path/to/dde-control-center
if(DEFINED DCC_SOURCE_ROOT AND EXISTS "${DCC_SOURCE_ROOT}/include")
    set(DCC_INCLUDE_DIR "${DCC_SOURCE_ROOT}/include")
elseif(DEFINED ENV{DCC_SOURCE_ROOT} AND EXISTS "$ENV{DCC_SOURCE_ROOT}/include")
    set(DCC_INCLUDE_DIR "$ENV{DCC_SOURCE_ROOT}/include")
else()
    find_path(DCC_INCLUDE_DIR
        NAMES dccfactory.h
        PATHS /usr/include/dde-control-center
        REQUIRED)
endif()

set(DCC_PLUGIN_DIR "/usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}/dde-control-center/plugins_v1.1")

# ─── DFM (dde-file-manager) 头文件路径 ───
# 默认使用系统安装的 dde-file-manager-dev 包提供的 /usr/include/dfm-base
# 和 /usr/include/dfm-framework。如需用 in-tree 源码 checkout 的头文件，
# 请显式提供：
#   cmake -B build -DDFM_SOURCE_ROOT=/path/to/dde-file-manager
#   或: export DFM_SOURCE_ROOT=/path/to/dde-file-manager
if(DEFINED DFM_SOURCE_ROOT AND EXISTS "${DFM_SOURCE_ROOT}/include")
    set(DFM_INCLUDE_DIR "${DFM_SOURCE_ROOT}/include")
elseif(DEFINED ENV{DFM_SOURCE_ROOT} AND EXISTS "$ENV{DFM_SOURCE_ROOT}/include")
    set(DFM_INCLUDE_DIR "$ENV{DFM_SOURCE_ROOT}/include")
else()
    find_path(DFM_INCLUDE_DIR
        NAMES dfm-base/dfm_log_defines.h
        PATHS /usr/include
        REQUIRED)
endif()

set(EDGE_PLUGIN_DIR "/usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}/dde-file-manager/plugins/desktop-edge")
set(MEME_WALLPAPER_DIR "/usr/share/deepin-meme-wallpapers")

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui DBus Concurrent Network LinguistTools)
find_package(Dtk6 REQUIRED COMPONENTS Core)
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED libavformat libavcodec libavutil libswscale)
find_library(DFM6_BASE_LIB dfm6-base REQUIRED)
find_library(DFM6_FRAMEWORK_LIB dfm6-framework REQUIRED)

add_subdirectory(src/desktop-edge)
add_subdirectory(src/control-center-plugin)

# 预置视频
install(DIRECTORY data/res/
    DESTINATION ${MEME_WALLPAPER_DIR}
    FILES_MATCHING PATTERN "*.mp4")

# DConfig schema
dtk_add_config_meta_files(APPID org.deepin.meme FILES data/configs/org.deepin.meme.json)

# 翻译
qt_add_lupdate(LTS_FILES
    SOURCE_TARGETS meme_qml
    TS_FILES translations/deepin-meme-plugin_zh_CN.ts)
qt_add_lrelease(QM_FILES
    TS_FILES translations/deepin-meme-plugin_zh_CN.ts)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/deepin-meme-plugin_zh_CN.qm
    DESTINATION /usr/share/deepin-meme-plugin/translations)

# 图标
install(FILES data/icons/meme_icon.svg
    DESTINATION /usr/share/icons/hicolor/scalable/apps)
