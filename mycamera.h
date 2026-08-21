#ifndef MYCAMERA_H
#define MYCAMERA_H

#include <QObject>
#include <QCamera>
#include <QCameraInfo>
#include <QAbstractVideoSurface>
#include <QVideoSurfaceFormat>
#include <QImage>
#include <QLabel>
#include <QString>
#include <QList>

// 视频帧接收表面，替代 QCameraViewfinder + QCameraImageCapture
// 直接从摄像头视频流获取帧，无需显示 widget，避免 DirectShow 渲染管道崩溃
class MyVideoSurface : public QAbstractVideoSurface
{
    Q_OBJECT
public:
    explicit MyVideoSurface(QObject *parent = nullptr);

    QList<QVideoFrame::PixelFormat> supportedPixelFormats(
        QAbstractVideoBuffer::HandleType handleType = QAbstractVideoBuffer::NoHandle) const;

    bool start(const QVideoSurfaceFormat &format);
    void stop();

signals:
    void frameAvailable(const QImage &image);

protected:
    bool present(const QVideoFrame &frame);
};

class MyCamera : public QObject
{
    Q_OBJECT
private:
    explicit MyCamera(QObject *parent = nullptr);
    ~MyCamera();
    bool initCamera();
    void cleanupCamera();

private:
    static MyCamera *s_instance;
    static QString s_preferredCameraName;
    static int s_preferredCameraIndex;

    QCamera *m_camera;
    MyVideoSurface *m_videoSurface;
    QImage m_currentFrame;
    QLabel *m_displayLabel;
    int m_currentCameraIndex;
    bool m_isInitialized;
    bool m_isCapturing;
    qint64 m_lastFrameTime;      // 上一帧时间戳，用于限流
    int m_frameSkipCount;        // 跳帧计数

public:
    static MyCamera *getInstance();
    static void releaseInstance();

    static void setPreferredCameraByName(const QString &nameKeyword);
    static void setPreferredCameraByIndex(int index);
    static void listAllCameras();
    static QList<QCameraInfo> getAvailableCameras();

    bool reinitialize();

    void startCamera();
    void stopCamera();
    void setDisplayLabel(QLabel *label);
    bool switchCamera(int index);
    int currentCameraIndex() const;
    bool isInitialized() const;

    QImage currentFrame() const;

signals:
    void frameCaptured(const QImage &image);
    void cameraSwitched(int index, const QString &description);
    void initializationFailed(const QString &reason);

private slots:
    void onFrameAvailable(const QImage &image);
};

#endif // MYCAMERA_H
