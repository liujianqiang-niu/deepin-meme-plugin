# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
# SPDX-License-Identifier: GPL-3.0-or-later
set(PLUGIN_NAME "meme")

find_package(Qt6 COMPONENTS Core Qml Gui Multimedia DBus REQUIRED)
find_package(Dtk6 COMPONENTS Core Widget REQUIRED)

set(PLUGIN_SRCS
    src/memeplugin.h
    src/memeplugin.cpp
)

set(QML_FILES
    Meme.qml
    MemeMain.qml
)

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

add_library(${PLUGIN_NAME} MODULE ${PLUGIN_SRCS})
set_target_properties(${PLUGIN_NAME} PROPERTIES PREFIX "")

find_library(DCC_LIBRARY NAMES dde-control-center)
if(NOT DCC_LIBRARY)
    message(FATAL_ERROR "libdde-control-center not found.")
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

target_include_directories(${PLUGIN_NAME} PRIVATE
    ${DCC_INCLUDE_DIR}
)

install(TARGETS ${PLUGIN_NAME}
    LIBRARY DESTINATION ${DCC_PLUGIN_DIR}/${PLUGIN_NAME}
)

install(DIRECTORY ${plugin_dirs}
    DESTINATION ${DCC_PLUGIN_DIR}
)
