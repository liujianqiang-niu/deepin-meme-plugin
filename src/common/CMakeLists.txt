# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
# SPDX-License-Identifier: GPL-3.0-or-later
find_package(Qt6 COMPONENTS Core REQUIRED)
find_package(Dtk6 COMPONENTS Core REQUIRED)

add_library(meme-common STATIC
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
