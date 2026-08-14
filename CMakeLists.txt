# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
# SPDX-License-Identifier: GPL-3.0-or-later
cmake_minimum_required(VERSION 3.23)

project(deepin-meme-plugin
    VERSION 1.0.0
    DESCRIPTION "Deepin dynamic wallpaper effect plugin"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# 控制中心源码头文件路径（dccfactory.h 所在）
set(DCC_SOURCE_ROOT "/home/uos/work-ljq/work-v25/code-work/ai-work/dde-control-center")
set(DCC_INCLUDE_DIR "${DCC_SOURCE_ROOT}/include")

# 插件安装目录（与系统已安装插件一致）
set(DCC_PLUGIN_DIR "/usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}/dde-control-center/plugins_v1.1")

# 主题包安装目录
set(MEME_THEME_DIR "/usr/share/deepin-meme-themes")

find_package(Qt6 COMPONENTS Core Gui Widgets Multimedia DBus LinguistTools REQUIRED)
find_package(Dtk6 COMPONENTS Core Widget REQUIRED)
find_package(Dtk6DConfig REQUIRED)

# 子模块
add_subdirectory(src/common)
add_subdirectory(src/control-center-plugin)
add_subdirectory(src/desktop-daemon)

# 安装所有主题包
install(DIRECTORY data/themes/laser-hero data/themes/plant-guard data/themes/ninja-slash
               data/themes/cosmic-star data/themes/firework-burst
    DESTINATION ${MEME_THEME_DIR}
    PATTERN "manifest.schema.json" EXCLUDE)

# DConfig schema: dtk_add_config_meta_files 由 find_package(Dtk6DConfig) 自动导入
dtk_add_config_meta_files(APPID org.deepin.meme FILES data/configs/org.deepin.meme.json)

# 国际化: lupdate 提取 -> lrelease 编译 .qm
qt_add_lupdate(LTS_FILES
    SOURCE_TARGETS meme_qml
    TS_FILES translations/deepin-meme-plugin_zh_CN.ts)
qt_add_lrelease(QM_FILES
    TS_FILES translations/deepin-meme-plugin_zh_CN.ts)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/deepin-meme-plugin_zh_CN.qm
    DESTINATION /usr/share/deepin-meme-plugin/translations)

install(FILES data/dbus/org.deepin.meme.daemon.service
    DESTINATION /usr/share/dbus-1/services)

# autostart desktop 文件
install(FILES data/autostart/deepin-meme-daemon.desktop
    DESTINATION /usr/share/applications)

# systemd user service
install(FILES data/systemd/deepin-meme-daemon.service
    DESTINATION /usr/share/systemd/user)

# 图标
install(FILES data/icons/meme_icon.svg
    DESTINATION /usr/share/icons/hicolor/scalable/apps)
