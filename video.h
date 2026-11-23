#pragma once
#include <QQuickFramebufferObject>
#include <memory>

struct mpv_handle;
class Video : public QQuickFramebufferObject {
    Q_OBJECT
    Q_PROPERTY(double position READ position NOTIFY clockChanged)
    Q_PROPERTY(double duration READ duration NOTIFY clockChanged)
public:
    Video();
    Renderer *createRenderer() const override;
    Q_INVOKABLE void play(const QString &video, const QString &audio);
    Q_INVOKABLE void seek(double seconds);
    double position() const { return m_position; }
    double duration() const { return m_duration; }
signals:
    void clockChanged();
    void failed(QString message);
private:
    void poll();
    std::shared_ptr<mpv_handle> m_player;
    double m_position = 0;
    double m_duration = 0;
};
void registerVideo();
