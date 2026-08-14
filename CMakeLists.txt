# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
# SPDX-License-Identifier: GPL-3.0-or-later
cmake_minimum_required(VERSION 3.23)

project(deepin-meme-plugin
    VERSION 2.0.0
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
set(MEME_WALLPAPER_DIR "/usr/share/deepin-meme-wallpapers")

find_package(Qt6 COMPONENTS Core Gui Widgets Multimedia DBus LinguistTools REQUIRED)
find_package(Dtk6 COMPONENTS Core Widget REQUIRED)
find_package(Dtk6DConfig REQUIRED)

add_subdirectory(src/common)
add_subdirectory(src/control-center-plugin)
add_subdirectory(src/desktop-daemon)

# 安装 5 个动态壁纸 mp4
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

# D-Bus service
install(FILES data/dbus/org.deepin.meme.daemon.service
    DESTINATION /usr/share/dbus-1/services)

# systemd user service
install(FILES data/systemd/deepin-meme-daemon.service
    DESTINATION /usr/share/systemd/user)

# 图标
install(FILES data/icons/meme_icon.svg
    DESTINATION /usr/share/icons/hicolor/scalable/apps)
