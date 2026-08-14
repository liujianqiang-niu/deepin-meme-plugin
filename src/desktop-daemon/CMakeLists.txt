# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
# SPDX-License-Identifier: GPL-3.0-or-later
# 桌面特效守护进程

find_package(Qt6 COMPONENTS Core Gui Widgets Multimedia MultimediaWidgets DBus REQUIRED)
find_package(Dtk6 COMPONENTS Core Widget REQUIRED)

set(DAEMON_SRCS
    src/main.cpp
    src/effectoverlay.h
    src/effectoverlay.cpp
    src/fileoperationmonitor.h
    src/fileoperationmonitor.cpp
    src/themeresolver.h
    src/themeresolver.cpp
    src/effectplayer.h
    src/effectplayer.cpp
    src/wallpapermanager.h
    src/wallpapermanager.cpp
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
    Dtk6::Core
    Dtk6::Widget
)

target_include_directories(deepin-meme-daemon PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

install(TARGETS deepin-meme-daemon
    RUNTIME DESTINATION bin
)
