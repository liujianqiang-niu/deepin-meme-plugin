// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
import org.deepin.dcc 1.0

// 模块根对象，注册到个性化下
// 注意: 此文件中不能使用 dccData，根对象为 DccObject
DccObject {
    id: root
    name: "meme"                    // 必须与插件名一致
    parentName: "personalization"   // 挂载到个性化模块下
    displayName: qsTr("动态壁纸")
    icon: "meme_icon"
    weight: 350                     // 在 wallpaper(300) 之后
    visible: true
}
