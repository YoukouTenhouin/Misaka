#include "video.h"
#include <clocale>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QQuickOpenGLUtils>
#include <QQuickWindow>
#include <QTimer>
#include <mpv/client.h>
#include <mpv/render_gl.h>

class VideoRenderer : public QQuickFramebufferObject::Renderer {
public:
    VideoRenderer(std::shared_ptr<mpv_handle> player, Video *item)
        : m_player(std::move(player)), m_item(item) {}
    ~VideoRenderer() override {
        if (m_context) mpv_render_context_free(m_context);
    }
    void render() override {
        if (!m_context) {
            mpv_opengl_init_params gl{[](void *, const char *name) -> void * {
                return reinterpret_cast<void *>(QOpenGLContext::currentContext()->getProcAddress(name));
            }, nullptr};
            mpv_render_param init[] = {
                {MPV_RENDER_PARAM_API_TYPE, const_cast<char *>(MPV_RENDER_API_TYPE_OPENGL)},
                {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &gl}, {}};
            if (!m_player || mpv_render_context_create(&m_context, m_player.get(), init) < 0) {
                QMetaObject::invokeMethod(m_item, "failed", Qt::QueuedConnection,
                                          Q_ARG(QString, QStringLiteral("无法初始化 OpenGL 播放器")));
                return;
            }
            mpv_render_context_set_update_callback(m_context, [](void *item) {
                QMetaObject::invokeMethod(static_cast<Video *>(item), "update", Qt::QueuedConnection);
            }, m_item);
        }
        auto *target = framebufferObject();
        mpv_opengl_fbo fbo{int(target->handle()), target->width(), target->height(), 0};
        mpv_render_param params[] = {{MPV_RENDER_PARAM_OPENGL_FBO, &fbo}, {}};
        mpv_render_context_render(m_context, params);
        QQuickOpenGLUtils::resetOpenGLState();
    }
private:
    std::shared_ptr<mpv_handle> m_player;
    Video *m_item;
    mpv_render_context *m_context = nullptr;
};

Video::Video() : m_player(mpv_create(), mpv_terminate_destroy) {
    if (!m_player) return;
    auto *handle = m_player.get();
    mpv_set_option_string(handle, "vo", "libmpv");
    if (qEnvironmentVariableIsSet("PULSE_SINK")) mpv_set_option_string(handle, "ao", "pulse");
    mpv_set_option_string(handle, "referer", "https://www.bilibili.com/");
    mpv_set_option_string(handle, "user-agent", "Mozilla/5.0");
    mpv_set_option_string(handle, "ytdl", "no");
    if (mpv_initialize(handle) < 0) {
        m_player.reset();
        return;
    }
    auto *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &Video::poll);
    timer->start(100);
}

QQuickFramebufferObject::Renderer *Video::createRenderer() const {
    return new VideoRenderer(m_player, const_cast<Video *>(this));
}

void Video::play(const QString &video, const QString &audio) {
    if (!m_player) {
        emit failed(QStringLiteral("无法初始化播放器"));
        return;
    }
    auto segment = [](const QString &url) {
        const auto bytes = url.toUtf8();
        return "%" + QByteArray::number(bytes.size()) + "%" + bytes;
    };
    const QByteArray source = "edl://" + segment(video) + ";!new_stream;" + segment(audio);
    const char *args[] = {"loadfile", source.constData(), "replace", nullptr};
    if (mpv_command_async(m_player.get(), 0, args) < 0) emit failed(QStringLiteral("无法打开媒体"));
}

void Video::seek(double seconds) {
    if (!m_player || m_duration <= 0) return;
    auto time = QByteArray::number(seconds);
    const char *args[] = {"seek", time.constData(), "absolute+exact", nullptr};
    mpv_command_async(m_player.get(), 0, args);
}

void Video::poll() {
    auto *handle = m_player.get();
    for (;;) {
        auto event = mpv_wait_event(handle, 0);
        if (event->event_id == MPV_EVENT_NONE) break;
        if (event->event_id == MPV_EVENT_END_FILE) {
            auto *end = static_cast<mpv_event_end_file *>(event->data);
            if (end->error < 0) emit failed(QString::fromUtf8(mpv_error_string(end->error)));
        }
    }
    double position = 0, duration = 0;
    mpv_get_property(handle, "time-pos", MPV_FORMAT_DOUBLE, &position);
    mpv_get_property(handle, "duration", MPV_FORMAT_DOUBLE, &duration);
    if (position != m_position || duration != m_duration) {
        m_position = position;
        m_duration = duration;
        emit clockChanged();
    }
}

void registerVideo() {
    // Qt adopts the desktop locale at startup. libmpv requires C numeric parsing.
    std::setlocale(LC_NUMERIC, "C");
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
    qmlRegisterType<Video>("Misaka", 1, 0, "VideoSurface");
}
