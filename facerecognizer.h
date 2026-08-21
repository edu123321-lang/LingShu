#ifndef FACERECOGNIZER_H
#define FACERECOGNIZER_H

#include <QObject>
#include <QImage>
#include <QRect>
#include <QMutex>
#include <vector>
#include <string>

class FaceRecognizer : public QObject
{
    Q_OBJECT

public:
    ~FaceRecognizer();

    static FaceRecognizer *getInstance();
    static void releaseInstance();

    bool init(const std::string& modelPath);

    // 按优先级自动查找模型目录（exe 旁 models → 环境变量 SEETA_ROOT → 默认路径），
    // 返回第一个三个模型文件齐全的目录
    static std::string defaultModelPath();

    bool detectFace(const QImage& image, std::vector<float>& feature);

    bool detectFace(const QImage& image, std::vector<float>& feature, QRect& faceRect);

    float compareFeatures(const std::vector<float>& f1, const std::vector<float>& f2);

private:
    explicit FaceRecognizer(QObject *parent = nullptr);
    FaceRecognizer(const FaceRecognizer&) = delete;
    FaceRecognizer& operator=(const FaceRecognizer&) = delete;

    static FaceRecognizer *s_instance;

    class Impl;
    Impl *d;

    QMutex m_mutex;
};

#endif // FACERECOGNIZER_H
