# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
# SPDX-License-Identifier: GPL-3.0-or-later
cmake_minimum_required(VERSION 3.23)

project(deepin-meme-plugin
    VERSION 3.0.0
    DESCRIPTION "Deepin dynamic wallpaper plugin"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(DCC_SOURCE_ROOT "/home/uos/work-ljq/work-v25/code-work/ai-work/dde-control-center")
set(DCC_INCLUDE_DIR "${DCC_SOURCE_ROOT}/include")
set(DCC_PLUGIN_DIR "/usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}/dde-control-center/plugins_v1.1")
set(DFM_SOURCE_ROOT "/home/uos/work-ljq/work-v25/code-work/ai-work/dde-file-manager")
set(DFM_INCLUDE_DIR "${DFM_SOURCE_ROOT}/include")
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
