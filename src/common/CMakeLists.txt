# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
# SPDX-License-Identifier: GPL-3.0-or-later
# 共享库: 主题包 manifest 解析 + DConfig 封装

find_package(Qt6 COMPONENTS Core REQUIRED)
find_package(Dtk6 COMPONENTS Core REQUIRED)

add_library(meme-common STATIC
    src/thememanifest.h
    src/thememanifest.cpp
    src/memeconfig.h
    src/memeconfig.cpp
    src/memedconfig.h
    src/memedconfig.cpp
)

target_link_libraries(meme-common PUBLIC
    Qt6::Core
    Dtk6::Core
)

target_include_directories(meme-common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
