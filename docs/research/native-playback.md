# Linux/KDE 原生播放与保存路径

## 首版建议

在创建第一个窗口前用 QQuickWindow::setGraphicsApi 选择 OpenGL，让 QQuickFramebufferObject::Renderer 把 libmpv 的输出画入 Qt Quick 的帧缓冲。Wayland 管理顶层窗口，视频作为 QML 场景中的普通元素参与缩放和叠加。Qt 将此类标为仅兼容 OpenGL 的旧接口。在只面向一台 Linux/KDE 桌面的前提下，建议接受这个限制。[Qt 窗口图形后端](https://doc.qt.io/qt-6/qquickwindow.html#setGraphicsApi) · [Qt 帧缓冲元素](https://doc.qt.io/qt-6/qquickframebufferobject.html)

在 Qt 渲染线程、OpenGL 上下文为当前上下文时创建和销毁 mpv_render_context，渲染时传入当前帧缓冲的编号和实际像素尺寸。mpv 的更新回调只投递重绘通知。暂停、跳转和播放位置由 libmpv 客户端接口管理。窗口资源重建时重新取得帧缓冲。构建和运行所需组件包括 Qt 6 Quick、OpenGL、Qt Wayland 平台插件、带 OpenGL 渲染支持的 libmpv；保存需要 ffmpeg 可执行文件。[mpv 渲染接口](https://github.com/mpv-player/mpv/blob/master/include/mpv/render.h) · [OpenGL 接口](https://github.com/mpv-player/mpv/blob/master/include/mpv/render_gl.h) · [Qt 渲染器](https://doc.qt.io/qt-6/qquickframebufferobject-renderer.html)

## 一组来源，两种用途

解析一个分 P 时记录所选画面和声音的来源、所需请求头及清晰度。在线播放把视频 URL 传给 libmpv，并通过文件级 audio-files 选项附上同一分 P 的音频。对本身含声画的文件直接打开。建议只把长期有效的投稿和分 P 标识写入列表，在播放或保存开始时取得当次媒体地址。[mpv 选项与 loadfile 命令](https://mpv.io/manual/stable/)

保存时可用应用的网络层将选中的两条流下载到临时文件，再调用系统 ffmpeg。对于一个视频输入和一个音频输入，参数示例为 `ffmpeg -i video.m4s -i audio.m4s -map 0:v:0 -map 1:a:0 -c copy result.mkv`。这条路径保留原始编码，目标封装须支持所选轨道。建议完成后在目标目录发布最终文件并登记本地路径。遇到空间不足、取消或复用失败时，保留已有成品并提示失败。离线从列表打开本地路径。[FFmpeg 流选择与流复制](https://ffmpeg.org/ffmpeg.html#Streamcopy)

## 首次真机验证

建议在实际 KDE Wayland 会话用一段带独立音轨的分 P 检查首帧、声音同步、跳转、全屏以及缩放后的清晰度。切换分 P 后确认上一条音轨被清除。再保存同一分 P，断网后分别从列表和其他播放器打开、跳转到片尾。以普通 SDR 文件建立基线，高码率样本单独验证解码性能。本次是上游接口调查，真机图形驱动、在线媒体和具体编码组合尚待验收。
