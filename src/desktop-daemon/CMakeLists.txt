# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
# SPDX-License-Identifier: GPL-3.0-or-later
find_package(Qt6 COMPONENTS Core Gui Widgets Multimedia MultimediaWidgets DBus Quick Qml REQUIRED)

set(DAEMON_SRCS
    src/main.cpp
    src/wallpaperplayer.h
    src/wallpaperplayer.cpp
    wallpaper.qrc
)

add_executable(deepin-meme-daemon ${DAEMON_SRCS})

target_link_libraries(deepin-meme-daemon PRIVATE
    meme-common
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::MultimediaWidgets
    Qt6::DBus
    Qt6::Quick
    Qt6::Qml
)

target_include_directories(deepin-meme-daemon PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

install(TARGETS deepin-meme-daemon
    RUNTIME DESTINATION bin
)
