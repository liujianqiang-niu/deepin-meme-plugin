# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
# SPDX-License-Identifier: GPL-3.0-or-later
set(EDGE_SOURCES
    plugin.cpp
    engine.cpp
    videoproxy.cpp
    decoder.cpp
    menu.cpp
    config.cpp
)

add_library(dd-meme-wallpaper SHARED ${EDGE_SOURCES} meme_videowallpaper.json)

set_target_properties(dd-meme-wallpaper PROPERTIES
    OUTPUT_NAME "dd-meme-wallpaper-plugin"
    PREFIX "lib"
    VERSION ${PROJECT_VERSION}
    SOVERSION 3
)

target_include_directories(dd-meme-wallpaper PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/util
    ${DFM_INCLUDE_DIR}
    ${FFMPEG_INCLUDE_DIRS}
)

target_link_libraries(dd-meme-wallpaper PRIVATE
    Qt6::Core Qt6::Widgets Qt6::Gui Qt6::DBus
    Dtk6::Core
    ${DFM6_BASE_LIB}
    ${DFM6_FRAMEWORK_LIB}
    ${FFMPEG_LIBRARIES}
)

target_link_options(dd-meme-wallpaper PRIVATE -Wl,--no-undefined)

install(TARGETS dd-meme-wallpaper
    LIBRARY DESTINATION ${EDGE_PLUGIN_DIR}
)
