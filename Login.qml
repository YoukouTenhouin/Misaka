import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: login
    objectName: "accountPanel"
    enum Session { Guest, Unverified, Authenticated, Expired }
    property int session: Login.Guest
    property string username: ""
    property url avatarUrl: ""
    readonly property string identity: session === Login.Authenticated ? "已登录 · " + username
        : session === Login.Unverified ? "会话待验证" : session === Login.Expired ? "会话已过期" : "未登录"
    property string notice: ""
    property string qr: ""
    property string key: ""
    property int generation: 0
    property var pending: null
    function cancelRequest() {
        ++generation
        if (pending) pending.abort()
        pending = null
    }
    property bool validating: false
    property bool ready: false
    property bool persistAfterVerify: false
    function expire() {
        ++generation
        account.clearSession()
        session = Login.Expired
        username = ""
        avatarUrl = ""
        validating = false
        persistAfterVerify = false
        notice = ""
    }
    function get(url, done) {
        const serial = generation
        const xhr = new XMLHttpRequest()
        pending = xhr
        xhr.open("GET", url)
        xhr.onreadystatechange = function() {
            if (xhr.readyState !== XMLHttpRequest.DONE || serial !== generation) return
            pending = null
            if (xhr.status !== 200) {
                notice = xhr.status === 412 ? "服务拒绝请求 (412)" : "网络请求失败"
                return
            }
            try { done(JSON.parse(xhr.responseText)) }
            catch (error) { notice = "响应无效" }
        }
        xhr.send()
    }
    function verify() {
        validating = true
        notice = "正在验证"
        get(apiOrigin + "/x/web-interface/nav", result => {
            if (result.code === -101 || (result.code === 0 && result.data.isLogin === false)) {
                expire()
            } else if (result.code === 0 && result.data.isLogin === true) {
                username = result.data.uname
                avatarUrl = result.data.face || ""
                session = Login.Authenticated
                validating = false
                notice = ""
                if (persistAfterVerify) account.save()
                persistAfterVerify = false
            } else notice = "验证失败"
        })
    }
    function generate() {
        cancelRequest()
        timer.stop()
        qr = ""
        notice = "正在获取二维码"
        get(passportOrigin + "/x/passport-login/web/qrcode/generate", result => {
            if (result.code !== 0 || !result.data.url || !result.data.qrcode_key) throw Error()
            qr = account.qrImage(result.data.url)
            if (!qr) throw Error()
            key = result.data.qrcode_key
            notice = "等待扫码"
            timer.start()
        })
    }
    Timer {
        id: timer
        interval: 1000
        onTriggered: login.get(passportOrigin + "/x/passport-login/web/qrcode/poll?qrcode_key=" + encodeURIComponent(login.key), result => {
            if (result.code !== 0) throw Error()
            switch (result.data.code) {
            case 86101: case 86090:
                login.notice = result.data.code === 86101 ? "等待扫码" : "请在手机上确认"
                timer.start()
                break
            case 86038: login.notice = "二维码已失效"; login.qr = ""; break
            case 0:
                dialog.close()
                login.session = Login.Unverified
                login.persistAfterVerify = true
                login.verify()
                break
            default: throw Error()
            }
        })
    }
    Connections {
        target: account
        function onRestored(hasCookies) {
            login.ready = true
            if (hasCookies) { login.session = Login.Unverified; login.verify() }
        }
        function onStorageError(message) { login.notice = message }
    }
    Component.onCompleted: account.restore()
    RowLayout {
        Item {
            visible: login.session === Login.Authenticated
            Layout.preferredWidth: 32
            Layout.preferredHeight: 32
            Image {
                id: avatar
                objectName: "accountAvatar"
                anchors.fill: parent
                source: login.avatarUrl
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
            }
            Label {
                anchors.centerIn: parent
                visible: avatar.status !== Image.Ready
                text: Array.from(login.username)[0] || ""
            }
        }
        Label { text: login.identity }
        Button {
            objectName: "accountAction"
            enabled: login.ready
            text: login.validating ? "重试验证" : login.session === Login.Expired ? "重新登录" : "扫码登录"
            visible: login.session !== Login.Authenticated
            onClicked: {
                if (login.validating) { login.cancelRequest(); login.verify() }
                else { dialog.open(); login.generate() }
            }
        }
        Label { visible: !dialog.visible; text: login.notice }
    }
    Dialog {
        id: dialog
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        title: "使用哔哩哔哩客户端扫码"
        standardButtons: Dialog.Close
        onAboutToHide: { login.cancelRequest(); timer.stop(); login.qr = ""; login.notice = "" }
        ColumnLayout {
            Image { source: login.qr; Layout.preferredWidth: 240; Layout.preferredHeight: 240; smooth: false }
            Label { text: login.notice }
            Button { text: "刷新二维码"; onClicked: login.generate() }
        }
    }
}
