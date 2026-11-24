import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Misaka

ApplicationWindow {
    id: root
    visible: true
    width: 880
    height: 560
    title: "Misaka"
    property int requestId: 0
    property string status: ""

    function playbackTime(seconds) {
        const total = Math.max(0, Math.floor(seconds))
        const hours = Math.floor(total / 3600)
        const minutes = Math.floor(total / 60) % 60
        return [hours, minutes, total % 60].map(part => String(part).padStart(2, "0")).join(":")
    }

    component TransportButton: ToolButton {
        display: AbstractButton.IconOnly
        icon.width: 22
        icon.height: 22
        Accessible.name: text
        ToolTip.visible: hovered
        ToolTip.text: text
    }

    function request(path, serial, callback) {
        const xhr = new XMLHttpRequest()
        xhr.open("GET", apiOrigin + path)
        xhr.onreadystatechange = function() {
            if (xhr.readyState !== XMLHttpRequest.DONE || serial !== root.requestId) return
            if (xhr.status !== 200) {
                root.status = xhr.status === 412 ? "服务拒绝请求 (412)" : "请求失败 (HTTP " + xhr.status + ")"
                return
            }
            try {
                const result = JSON.parse(xhr.responseText)
                if (result.code === -101 && login.item) login.item.expire()
                if (result.code !== 0) throw Error(result.message || "请求失败")
                callback(result.data)
            } catch (e) {
                root.status = String(e.message)
            }
        }
        xhr.send()
    }

    function openP1() {
        const serial = ++requestId
        const bv = entry.text.trim()
        if (!/^BV[0-9A-Za-z]{10}$/.test(bv)) {
            status = "请输入有效的 BV 号"
            return
        }
        status = "正在获取 P1…"
        request("/x/player/pagelist?bvid=" + bv, serial, pages => {
            const page = pages.find(p => p.page === 1)
            if (!page) throw Error("未找到 P1")
            request("/x/player/wbi/playurl?fnval=16&qn=80&bvid=" + bv + "&cid=" + page.cid, serial, data => {
                function track(tracks) {
                    for (const t of tracks || []) {
                        const url = t.baseUrl || t.base_url
                        if (/^https?:\/\//.test(url)) return url
                    }
                    throw Error("缺少可播放的 DASH 轨道")
                }
                video.play(track(data.dash.video), track(data.dash.audio))
                status = "P1 · " + page.part
            })
        })
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 6
        Loader { id: login; active: typeof account !== "undefined"; source: "Login.qml" }
        RowLayout {
            Layout.fillWidth: true
            TextField {
                id: entry
                objectName: "bv"
                Layout.fillWidth: true
                placeholderText: "输入投稿 BV 号"
                onAccepted: root.openP1()
            }
            Button { text: "播放 P1"; onClicked: root.openP1() }
        }
        Label { text: root.status; Layout.fillWidth: true; elide: Text.ElideRight }
        VideoSurface {
            id: video
            objectName: "video"
            Layout.fillWidth: true
            Layout.fillHeight: true
            onFailed: message => root.status = message
        }
        ToolBar {
            Layout.fillWidth: true
            contentItem: ColumnLayout {
                spacing: 0
                Slider {
                    objectName: "timeline"
                    Accessible.name: "播放进度"
                    Layout.fillWidth: true
                    enabled: video.duration > 0
                    to: Math.max(1, video.duration)
                    value: video.position
                    onMoved: video.seek(value)
                }
                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 8
                    Layout.rightMargin: 8
                    TransportButton {
                        objectName: "playPause"
                        enabled: video.duration > 0
                        text: video.paused ? "继续播放" : "暂停"
                        icon.source: video.paused ? "qrc:/icons/play.svg" : "qrc:/icons/pause.svg"
                        onClicked: video.togglePause()
                    }
                    Label {
                        objectName: "clock"
                        font.family: "monospace"
                        text: root.playbackTime(video.position) + " / " + root.playbackTime(video.duration)
                    }
                    Item { Layout.fillWidth: true }
                    TransportButton {
                        objectName: "mute"
                        text: video.muted ? "取消静音" : "静音"
                        icon.source: video.muted ? "qrc:/icons/muted.svg" : "qrc:/icons/volume.svg"
                        onClicked: video.muted = !video.muted
                    }
                    Slider {
                        objectName: "volume"
                        Layout.preferredWidth: 100
                        Accessible.name: "音量"
                        from: 0
                        to: 100
                        stepSize: 1
                        value: video.volume
                        onMoved: video.volume = value
                        ToolTip.visible: hovered || pressed
                        ToolTip.text: Math.round(value) + "%"
                    }
                }
            }
        }
    }
}
