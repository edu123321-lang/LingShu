#include "facerecognizer.h"

#include <seeta/FaceDetector.h>
#include <seeta/FaceLandmarker.h>
#include <seeta/FaceRecognizer.h>
#include <seeta/Common/Struct.h>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QStringList>
#include <cmath>

FaceRecognizer *FaceRecognizer::s_instance = nullptr;

class FaceRecognizer::Impl
{
public:
    Impl() : detector(nullptr), landmarker(nullptr), recognizer(nullptr) {}
    ~Impl() {
        delete detector;
        delete landmarker;
        delete recognizer;
    }

    seeta::FaceDetector *detector;
    seeta::FaceLandmarker *landmarker;
    seeta::FaceRecognizer *recognizer;
};

FaceRecognizer::FaceRecognizer(QObject *parent)
    : QObject(parent)
    , d(new Impl())
{
}

FaceRecognizer::~FaceRecognizer()
{
    delete d;
}

FaceRecognizer *FaceRecognizer::getInstance()
{
    if (!s_instance) {
        s_instance = new FaceRecognizer();
    }
    return s_instance;
}

void FaceRecognizer::releaseInstance()
{
    if (s_instance) {
        delete s_instance;
        s_instance = nullptr;
    }
}

bool FaceRecognizer::init(const std::string& modelPath)
{
    // 清理上一次可能残留的部分初始化对象（例如上次中途失败）
    delete d->detector;
    d->detector = nullptr;
    delete d->landmarker;
    d->landmarker = nullptr;
    delete d->recognizer;
    d->recognizer = nullptr;

    std::string detectorPath = modelPath + "/face_detector.csta";
    std::string landmarkerPath = modelPath + "/face_landmarker_pts5.csta";
    std::string recognizerPath = modelPath + "/face_recognizer.csta";

    try {
        seeta::ModelSetting detectorSetting(detectorPath, seeta::ModelSetting::CPU, 0);
        d->detector = new seeta::FaceDetector(detectorSetting);
        d->detector->set(seeta::FaceDetector::PROPERTY_MIN_FACE_SIZE, 20);

        seeta::ModelSetting landmarkerSetting(landmarkerPath, seeta::ModelSetting::CPU, 0);
        d->landmarker = new seeta::FaceLandmarker(landmarkerSetting);

        seeta::ModelSetting recognizerSetting(recognizerPath, seeta::ModelSetting::CPU, 0);
        d->recognizer = new seeta::FaceRecognizer(recognizerSetting);
    } catch (const std::exception &e) {
        qDebug() << "SeetaFace 引擎初始化异常:" << e.what();
        return false;
    } catch (...) {
        qDebug() << "SeetaFace 引擎初始化未知异常";
        return false;
    }

    if (!d->detector || !d->landmarker || !d->recognizer) {
        qDebug() << "SeetaFace 引擎初始化失败！";
        return false;
    }

    qDebug() << "SeetaFace 引擎初始化成功！";
    return true;
}

std::string FaceRecognizer::defaultModelPath()
{
    QStringList candidates;
    candidates << QCoreApplication::applicationDirPath() + "/models";

    QByteArray env = qgetenv("SEETA_ROOT");
    if (!env.isEmpty()) {
        candidates << QString::fromLocal8Bit(env) + "/models";
    }
    candidates << QStringLiteral("D:/tools/SeetaFace6/models");

    for (const QString &p : candidates) {
        if (QFile::exists(p + "/face_detector.csta")
            && QFile::exists(p + "/face_landmarker_pts5.csta")
            && QFile::exists(p + "/face_recognizer.csta")) {
            return p.toStdString();
        }
    }
    return candidates.last().toStdString();
}

bool FaceRecognizer::detectFace(const QImage& image, std::vector<float>& feature)
{
    QRect rect;
    return detectFace(image, feature, rect);
}

bool FaceRecognizer::detectFace(const QImage& image, std::vector<float>& feature, QRect& faceRect)
{
    QMutexLocker locker(&m_mutex);

    if (!d->detector || !d->landmarker || !d->recognizer) {
        return false;
    }

    if (image.isNull()) {
        return false;
    }

    // 统一转为 32 位格式后再按 QRgb 读取，避免 RGB24 等格式越界
    QImage img = (image.format() == QImage::Format_RGB32 || image.format() == QImage::Format_ARGB32)
                     ? image : image.convertToFormat(QImage::Format_RGB32);
    if (img.isNull()) {
        return false;
    }

    int width = img.width();
    int height = img.height();

    std::vector<unsigned char> grayData(width * height);

    for (int y = 0; y < height; ++y) {
        const QRgb* scanLine = reinterpret_cast<const QRgb*>(img.scanLine(y));
        for (int x = 0; x < width; ++x) {
            QRgb pixel = scanLine[x];
            int gray = static_cast<int>(
                qRed(pixel) * 0.299 +
                qGreen(pixel) * 0.587 +
                qBlue(pixel) * 0.114 + 0.5);
            grayData[y * width + x] = static_cast<unsigned char>(gray);
        }
    }

    SeetaImageData grayImg;
    grayImg.data = grayData.data();
    grayImg.width = width;
    grayImg.height = height;
    grayImg.channels = 1;

    auto faces = d->detector->detect(grayImg);

    if (faces.size == 0) {
        return false;
    }

    SeetaRect rect = faces.data[0].pos;
    faceRect = QRect(rect.x, rect.y, rect.width, rect.height);

    std::vector<SeetaPointF> landmarks(5);
    d->landmarker->mark(grayImg, rect, landmarks.data());

    std::vector<unsigned char> bgrData(width * height * 3);

    for (int y = 0; y < height; ++y) {
        const QRgb* scanLine = reinterpret_cast<const QRgb*>(img.scanLine(y));
        for (int x = 0; x < width; ++x) {
            QRgb pixel = scanLine[x];
            int idx = (y * width + x) * 3;
            bgrData[idx + 0] = static_cast<unsigned char>(qBlue(pixel));
            bgrData[idx + 1] = static_cast<unsigned char>(qGreen(pixel));
            bgrData[idx + 2] = static_cast<unsigned char>(qRed(pixel));
        }
    }

    SeetaImageData bgrImg;
    bgrImg.data = bgrData.data();
    bgrImg.width = width;
    bgrImg.height = height;
    bgrImg.channels = 3;

    int featureSize = d->recognizer->GetExtractFeatureSize();
    feature.resize(featureSize);

    d->recognizer->Extract(bgrImg, landmarks.data(), feature.data());

    return true;
}

float FaceRecognizer::compareFeatures(const std::vector<float>& f1, const std::vector<float>& f2)
{
    QMutexLocker locker(&m_mutex);

    if (!d->recognizer) {
        return 0.0f;
    }

    return d->recognizer->CalculateSimilarity(f1.data(), f2.data());
}
