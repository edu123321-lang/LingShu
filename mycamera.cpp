#include "mycamera.h"
#include <QMessageBox>
#include <QDebug>
#include <QVideoFrame>
#include <QCameraViewfinderSettings>
#include <QDateTime>

// ======================== MyVideoSurface 实现 ========================

MyVideoSurface::MyVideoSurface(QObject *parent)
    : QAbstractVideoSurface(parent)
{
}

QList<QVideoFrame::PixelFormat> MyVideoSurface::supportedPixelFormats(
    QAbstractVideoBuffer::HandleType handleType) const
{
    Q_UNUSED(handleType);
    QList<QVideoFrame::PixelFormat> formats;
    formats << QVideoFrame::Format_RGB32
            << QVideoFrame::Format_ARGB32
            << QVideoFrame::Format_RGB24
            << QVideoFrame::Format_YUV420P
            << QVideoFrame::Format_YUYV
            << QVideoFrame::Format_NV12
            << QVideoFrame::Format_UYVY;
    return formats;
}

bool MyVideoSurface::start(const QVideoSurfaceFormat &format)
{
    return QAbstractVideoSurface::start(format);
}

void MyVideoSurface::stop()
{
    QAbstractVideoSurface::stop();
}

bool MyVideoSurface::present(const QVideoFrame &frame)
{
    // 始终返回 true，避免 DirectShow 因 present 返回 false 而崩溃
    if (!frame.isValid()) {
        return true;
    }

    // 仅处理 CPU 可访问的帧（非 GPU 纹理）
    if (frame.handleType() != QAbstractVideoBuffer::NoHandle) {
        return true;
    }

    QVideoFrame copy(frame);
    if (!copy.map(QAbstractVideoBuffer::ReadOnly)) {
        return true;
    }

    QImage image;
    QVideoFrame::PixelFormat pf = copy.pixelFormat();
    QImage::Format imgFormat = QVideoFrame::imageFormatFromPixelFormat(pf);

    if (imgFormat != QImage::Format_Invalid) {
        // 格式可直接转换为 QImage（RGB32/ARGB32/RGB24 等）
        image = QImage(copy.bits(), copy.width(), copy.height(),
                       copy.bytesPerLine(), imgFormat).copy();
    } else if (pf == QVideoFrame::Format_YUYV) {
        // YUYV (YUY2) 是 USB 摄像头在 Windows 上最常见的格式，手动转 RGB32
        int w = copy.width();
        int h = copy.height();
        image = QImage(w, h, QImage::Format_RGB32);
        const uchar *src = copy.bits();
        QRgb *dst = (QRgb *)image.bits();
        for (int y = 0; y < h; ++y) {
            const uchar *row = src + y * copy.bytesPerLine();
            for (int x = 0; x < w - 1; x += 2) {
                int Y0 = row[x * 2];
                int U  = row[x * 2 + 1];
                int Y1 = row[x * 2 + 2];
                int V  = row[x * 2 + 3];

                // YUV → RGB (BT.601)
                int C0 = Y0 - 16, D0 = U - 128, E0 = V - 128;
                int C1 = Y1 - 16, D1 = U - 128, E1 = V - 128;

                int r0 = (298 * C0 + 409 * E0 + 128) >> 8;
                int g0 = (298 * C0 - 100 * D0 - 208 * E0 + 128) >> 8;
                int b0 = (298 * C0 + 516 * D0 + 128) >> 8;

                int r1 = (298 * C1 + 409 * E1 + 128) >> 8;
                int g1 = (298 * C1 - 100 * D1 - 208 * E1 + 128) >> 8;
                int b1 = (298 * C1 + 516 * D1 + 128) >> 8;

                dst[y * w + x] = qRgb(qBound(0, r0, 255), qBound(0, g0, 255), qBound(0, b0, 255));
                dst[y * w + x + 1] = qRgb(qBound(0, r1, 255), qBound(0, g1, 255), qBound(0, b1, 255));
            }
            // 奇数宽度最后一列
            if (w % 2 == 1) {
                int x = w - 1;
                int Y0 = row[x * 2];
                int U  = row[x * 2 + 1];
                int V  = row[x * 2 + 3];
                int C0 = Y0 - 16, D0 = U - 128, E0 = V - 128;
                int r0 = (298 * C0 + 409 * E0 + 128) >> 8;
                int g0 = (298 * C0 - 100 * D0 - 208 * E0 + 128) >> 8;
                int b0 = (298 * C0 + 516 * D0 + 128) >> 8;
                dst[y * w + x] = qRgb(qBound(0, r0, 255), qBound(0, g0, 255), qBound(0, b0, 255));
            }
        }
    } else if (pf == QVideoFrame::Format_NV12) {
        // NV12: Y 平面 + 交错的 UV 平面
        int w = copy.width();
        int h = copy.height();
        image = QImage(w, h, QImage::Format_RGB32);
        const uchar *yPlane = copy.bits();
        const uchar *uvPlane = yPlane + w * h;
        QRgb *dst = (QRgb *)image.bits();
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                int Y = yPlane[y * w + x];
                int U = uvPlane[(y / 2) * w + (x / 2) * 2] - 128;
                int V = uvPlane[(y / 2) * w + (x / 2) * 2 + 1] - 128;
                int C = Y - 16;
                int r = (298 * C + 409 * V + 128) >> 8;
                int g = (298 * C - 100 * U - 208 * V + 128) >> 8;
                int b = (298 * C + 516 * U + 128) >> 8;
                dst[y * w + x] = qRgb(qBound(0, r, 255), qBound(0, g, 255), qBound(0, b, 255));
            }
        }
    }
    // 其他无法识别的格式跳过，不 emit

    copy.unmap();

    if (image.isNull()) {
        return true; // 仍然返回 true
    }

    // 镜像（USB 摄像头通常需要水平镜像）
    image = image.mirrored(true, false);

    emit frameAvailable(image);
    return true;
}

// ======================== MyCamera 实现 ========================

MyCamera *MyCamera::s_instance = nullptr;
QString MyCamera::s_preferredCameraName;
int MyCamera::s_preferredCameraIndex = -1;

void MyCamera::setPreferredCameraByName(const QString &nameKeyword)
{
    s_preferredCameraName = nameKeyword;
}

void MyCamera::setPreferredCameraByIndex(int index)
{
    s_preferredCameraIndex = index;
}

void MyCamera::listAllCameras()
{
    QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    qDebug() << "========== 系统检测到" << cameras.size() << "个摄像头 ==========";
    for (int i = 0; i < cameras.size(); ++i) {
        qDebug() << QString("[%1] 设备名: %2").arg(i).arg(cameras[i].deviceName());
        qDebug() << QString("    描述(description): %1").arg(cameras[i].description());
    }
    qDebug() << "=========================================================";
}

QList<QCameraInfo> MyCamera::getAvailableCameras()
{
    return QCameraInfo::availableCameras();
}

int MyCamera::currentCameraIndex() const
{
    return m_currentCameraIndex;
}

bool MyCamera::isInitialized() const
{
    return m_isInitialized;
}

MyCamera::MyCamera(QObject *parent) : QObject(parent)
    , m_camera(nullptr)
    , m_videoSurface(nullptr)
    , m_displayLabel(nullptr)
    , m_currentCameraIndex(-1)
    , m_isInitialized(false)
    , m_isCapturing(false)
    , m_lastFrameTime(0)
    , m_frameSkipCount(0)
{
    if (!initCamera()) {
        m_isInitialized = false;
        qDebug() << "摄像头初始化失败，可能没有摄像头设备";
        emit initializationFailed(QStringLiteral("未检测到可用摄像头设备"));
    } else {
        m_isInitialized = true;
    }
}

MyCamera::~MyCamera()
{
    stopCamera();
    cleanupCamera();
}

void MyCamera::cleanupCamera()
{
    if (m_videoSurface) {
        m_videoSurface->disconnect();
        if (m_videoSurface->isActive()) {
            m_videoSurface->stop();
        }
        delete m_videoSurface;
        m_videoSurface = nullptr;
    }
    if (m_camera) {
        m_camera->blockSignals(true);
        if (m_camera->state() != QCamera::UnloadedState) {
            m_camera->stop();
        }
        m_camera->unload();
        m_camera->blockSignals(false);
        delete m_camera;
        m_camera = nullptr;
    }
    m_currentCameraIndex = -1;
    m_isInitialized = false;
}

bool MyCamera::reinitialize()
{
    stopCamera();
    cleanupCamera();

    if (!initCamera()) {
        m_isInitialized = false;
        emit initializationFailed(QStringLiteral("重新扫描后仍未检测到可用摄像头"));
        return false;
    }
    m_isInitialized = true;
    return true;
}

bool MyCamera::initCamera()
{
    listAllCameras();

    QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    if (cameras.isEmpty()) {
        qDebug() << "没有检测到摄像头";
        return false;
    }

    int selectedIndex = 0;
    bool foundByName = false;

    if (!s_preferredCameraName.isEmpty()) {
        qDebug() << "[摄像头选择] 优先按名称关键词匹配:" << s_preferredCameraName;
        for (int i = 0; i < cameras.size(); ++i) {
            QString devName = cameras[i].deviceName();
            QString desc = cameras[i].description();
            if (devName.contains(s_preferredCameraName, Qt::CaseInsensitive)
                || desc.contains(s_preferredCameraName, Qt::CaseInsensitive)) {
                selectedIndex = i;
                foundByName = true;
                qDebug() << QString("[摄像头选择] 按名称匹配成功 → index=%1 描述=%2").arg(i).arg(desc);
                break;
            }
        }
        if (!foundByName) {
            qDebug() << "[摄像头选择] 没有找到包含" << s_preferredCameraName
                     << "的摄像头，回退到 index 指定/默认 0";
        }
    }

    if (!foundByName && s_preferredCameraIndex >= 0) {
        if (s_preferredCameraIndex < cameras.size()) {
            selectedIndex = s_preferredCameraIndex;
            qDebug() << "[摄像头选择] 按索引选择 → index=" << s_preferredCameraIndex
                     << "描述=" << cameras[selectedIndex].description();
        } else {
            qDebug() << "[摄像头选择] 指定的 index=" << s_preferredCameraIndex
                     << "超出范围(共" << cameras.size() << "个)，回退到 index=0";
            selectedIndex = 0;
        }
    }

    if (selectedIndex >= cameras.size()) {
        selectedIndex = 0;
    }

    return switchCamera(selectedIndex);
}

bool MyCamera::switchCamera(int index)
{
    QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    if (cameras.isEmpty()) {
        qDebug() << "[切换摄像头失败：没有检测到摄像头";
        m_isInitialized = false;
        return false;
    }

    if (index < 0 || index >= cameras.size()) {
        qDebug() << "[切换摄像头失败] 索引" << index << "超出范围 (0 -" << cameras.size() - 1 << ")";
        return false;
    }

    bool wasCapturing = m_isCapturing;
    stopCamera();
    cleanupCamera();

    qDebug() << "[切换摄像头] index=" << index
             << "设备名=" << cameras[index].deviceName()
             << "描述=" << cameras[index].description();

    m_currentCameraIndex = index;
    m_camera = new QCamera(cameras[index]);

    // 创建 VideoSurface 并设置为 viewfinder
    // 这是关键改动：用 QAbstractVideoSurface 替代 QCameraViewfinder
    // 避免了隐藏 widget 导致 DirectShow 渲染管道崩溃的问题
    m_videoSurface = new MyVideoSurface(this);
    // present() 由摄像头内部线程调用，必须用 QueuedConnection 回到 GUI 线程
    connect(m_videoSurface, &MyVideoSurface::frameAvailable,
            this, &MyCamera::onFrameAvailable, Qt::QueuedConnection);
    m_camera->setViewfinder(m_videoSurface);

    // 尝试设置分辨率（即使 supportedViewfinderSettings 返回空也不影响）
    QList<QCameraViewfinderSettings> supported = m_camera->supportedViewfinderSettings();
    qDebug() << "[摄像头] 支持的 Viewfinder 配置数量:" << supported.size();

    if (supported.isEmpty()) {
        qDebug() << "[摄像头] 无法获取支持的分辨率，使用 Qt 默认设置";
    } else {
        QCameraViewfinderSettings settings;
        bool found = false;
        // 优先 640x480
        for (const auto &s : supported) {
            QSize r = s.resolution();
            if (r.width() == 640 && r.height() == 480) {
                settings = s;
                found = true;
                break;
            }
        }
        if (!found) {
            settings = supported.first();
        }
        qDebug() << "[摄像头] 使用分辨率:" << settings.resolution();
        m_camera->setViewfinderSettings(settings);
    }

    m_camera->start();

    QCamera::State s = m_camera->state();
    QCamera::Error e = m_camera->error();
    qDebug() << "[摄像头] start() 之后 state=" << s << "error=" << e
             << "errorString=" << m_camera->errorString();

    if (s != QCamera::ActiveState && e != QCamera::NoError) {
        qDebug() << "[摄像头] 启动失败，清理资源";
        cleanupCamera();
        m_isInitialized = false;
        emit initializationFailed(QStringLiteral("摄像头启动失败: %1")
                                  .arg(m_camera ? m_camera->errorString() : QStringLiteral("未知错误")));
        return false;
    }

    m_isInitialized = true;
    emit cameraSwitched(index, cameras[index].description());

    if (wasCapturing) {
        startCamera();
    }

    return true;
}

MyCamera *MyCamera::getInstance()
{
    if (!s_instance) {
        s_instance = new MyCamera();
    }
    return s_instance;
}

void MyCamera::releaseInstance()
{
    if (s_instance) {
        delete s_instance;
        s_instance = nullptr;
    }
}

void MyCamera::startCamera()
{
    if (!m_isInitialized) {
        qDebug() << "[startCamera] 跳过：摄像头未成功初始化";
        return;
    }
    if (!m_camera) {
        qDebug() << "[startCamera] 跳过：m_camera 为空";
        return;
    }
    if (m_camera->state() != QCamera::ActiveState) {
        qDebug() << "[startCamera] 摄像头状态不是 ActiveState，尝试启动...";
        m_camera->start();
        if (m_camera->state() != QCamera::ActiveState) {
            qDebug() << "[startCamera] 摄像头无法进入 ActiveState";
            return;
        }
    }
    m_isCapturing = true;
    qDebug() << "[startCamera] 摄像头采集已启动（通过 VideoSurface 自动接收帧）";
}

void MyCamera::stopCamera()
{
    m_isCapturing = false;
}

void MyCamera::setDisplayLabel(QLabel *label)
{
    m_displayLabel = label;
}

QImage MyCamera::currentFrame() const
{
    return m_currentFrame;
}

void MyCamera::onFrameAvailable(const QImage &image)
{
    if (image.isNull()) {
        return;
    }

    // 帧率限流：最多 15fps，防止事件队列被淹没导致 UI 冻结
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (m_lastFrameTime > 0 && (now - m_lastFrameTime) < 66) {
        m_frameSkipCount++;
        return;
    }
    m_lastFrameTime = now;

    m_currentFrame = image;

    if (m_displayLabel) {
        m_displayLabel->setPixmap(
            QPixmap::fromImage(image)
                .scaled(m_displayLabel->size(), Qt::KeepAspectRatio, Qt::FastTransformation)
        );
    }

    emit frameCaptured(image);
}
