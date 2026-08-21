#include "faceattendancedialog.h"
#include "ui_faceattendancedialog.h"
#include "mycamera.h"
#include "facerecognizer.h"
#include "mysql.h"

#include <QMessageBox>
#include <QPainter>
#include <QDebug>
#include <QDate>
#include <QTime>
#include <QInputDialog>
#include <QtConcurrent/QtConcurrentRun>
#include <QSpacerItem>
#include <QApplication>

FaceAttendanceDialog::FaceAttendanceDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FaceAttendanceDialog),
    m_lblCameraSelect(nullptr),
    m_cmbCameraList(nullptr),
    m_btnRefreshCameras(nullptr),
    m_cameraSelectorLayout(nullptr),
    m_cameraSelectorWidget(nullptr)
{
    ui->setupUi(this);
    setWindowTitle("人脸识别考勤");
    setMinimumSize(900, 700);

    m_statusTimer = new QTimer(this);
    connect(m_statusTimer, &QTimer::timeout, this, &FaceAttendanceDialog::onStatusTimeout);

    FaceRecognizer *fr = FaceRecognizer::getInstance();
    std::string modelPath = FaceRecognizer::defaultModelPath();
    if (!fr->init(modelPath)) {
        ui->lblStatus->setText(QStringLiteral("警告：人脸识别引擎初始化失败，请检查模型路径：%1")
                                   .arg(QString::fromStdString(modelPath)));
        ui->lblStatus->setStyleSheet("color: #c53030; font-weight: bold;");
    }

    // 构建摄像头选择区域
    setupCameraSelector();

    MyCamera *cam = MyCamera::getInstance();
    cam->setDisplayLabel(nullptr);
    connect(cam, &MyCamera::frameCaptured, this, &FaceAttendanceDialog::onFrameCaptured);
    connect(cam, &MyCamera::cameraSwitched, this, &FaceAttendanceDialog::onCameraSwitched);
    connect(cam, &MyCamera::initializationFailed, this, &FaceAttendanceDialog::onCameraInitFailed);

    // 刷新摄像头列表并选择当前使用的摄像头
    refreshCameraList();

    // ============ 入口防御：摄像头没准备好时，禁用所有考勤相关按钮 ============
    bool cameraOk = cam->isInitialized();
    updateUiForCameraState(cameraOk);

    if (cameraOk) {
        cam->startCamera();
        ui->lblStatus->setText("请正对摄像头，准备识别...");
    } else {
        ui->lblStatus->setText(QStringLiteral("❌ 未检测到可用摄像头，请插入摄像头后点击『刷新列表』重试"));
        ui->lblStatus->setStyleSheet("color: #c53030; font-weight: bold;");
        ui->lblCamera->setText(QStringLiteral("⚠\n未检测到摄像头设备\n\n请插入摄像头后点击刷新按钮"));
    }
    ui->lblUserName->setText("未识别");
}

FaceAttendanceDialog::~FaceAttendanceDialog()
{
    MyCamera *cam = MyCamera::getInstance();
    cam->stopCamera();
    cam->setDisplayLabel(nullptr);
    disconnect(cam, &MyCamera::frameCaptured, this, &FaceAttendanceDialog::onFrameCaptured);
    disconnect(cam, &MyCamera::cameraSwitched, this, &FaceAttendanceDialog::onCameraSwitched);
    delete ui;
}

void FaceAttendanceDialog::setupCameraSelector()
{
    m_cameraSelectorWidget = new QWidget(this);
    m_cameraSelectorWidget->setStyleSheet(
        "QWidget {"
        "   background-color: #f0f4f8;"
        "   border: 1px solid #cbd5e0;"
        "   border-radius: 8px;"
        "}"
        "QLabel {"
        "   border: none;"
        "   background: transparent;"
        "   font-size: 14px;"
        "   font-weight: 500;"
        "   color: #2d3748;"
        "}"
        "QComboBox {"
        "   padding: 6px 12px;"
        "   border: 1px solid #a0aec0;"
        "   border-radius: 6px;"
        "   background-color: white;"
        "   min-width: 280px;"
        "   font-size: 13px;"
        "}"
        "QComboBox:hover {"
        "   border-color: #4299e1;"
        "}"
        "QComboBox::drop-down {"
        "   border: none;"
        "   width: 24px;"
        "}"
        "QPushButton {"
        "   padding: 6px 16px;"
        "   background-color: #4299e1;"
        "   color: white;"
        "   border: none;"
        "   border-radius: 6px;"
        "   font-size: 13px;"
        "   font-weight: 500;"
        "}"
        "QPushButton:hover {"
        "   background-color: #3182ce;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #2b6cb0;"
        "}"
    );

    m_cameraSelectorLayout = new QHBoxLayout(m_cameraSelectorWidget);
    m_cameraSelectorLayout->setContentsMargins(14, 10, 14, 10);
    m_cameraSelectorLayout->setSpacing(10);

    m_lblCameraSelect = new QLabel("选择摄像头：", m_cameraSelectorWidget);
    m_cameraSelectorLayout->addWidget(m_lblCameraSelect);

    m_cmbCameraList = new QComboBox(m_cameraSelectorWidget);
    m_cmbCameraList->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(m_cmbCameraList, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FaceAttendanceDialog::onCameraSelectionChanged);
    m_cameraSelectorLayout->addWidget(m_cmbCameraList, 1);

    m_btnRefreshCameras = new QPushButton("🔄 刷新列表", m_cameraSelectorWidget);
    connect(m_btnRefreshCameras, &QPushButton::clicked,
            this, &FaceAttendanceDialog::onRefreshCamerasClicked);
    m_cameraSelectorLayout->addWidget(m_btnRefreshCameras);

    // 插入到 verticalLayout 中（在标题之后，主内容之前）
    QVBoxLayout *mainLayout = qobject_cast<QVBoxLayout *>(layout());
    if (mainLayout) {
        mainLayout->insertWidget(1, m_cameraSelectorWidget);
    }
}

void FaceAttendanceDialog::updateUiForCameraState(bool cameraOk)
{
    if (!ui) return;
    if (ui->btnCheckIn)    ui->btnCheckIn->setEnabled(cameraOk);
    if (ui->btnCheckOut)   ui->btnCheckOut->setEnabled(cameraOk);
    if (ui->btnRegister)   ui->btnRegister->setEnabled(cameraOk);
}

void FaceAttendanceDialog::onCameraInitFailed(const QString &reason)
{
    ui->lblStatus->setText(QStringLiteral("⚠ 摄像头初始化失败：%1").arg(reason));
    ui->lblStatus->setStyleSheet("color: #c53030; font-weight: bold;");
    updateUiForCameraState(false);
}

void FaceAttendanceDialog::refreshCameraList()
{
    if (!m_cmbCameraList) return;

    // 断开信号避免触发切换
    disconnect(m_cmbCameraList, QOverload<int>::of(&QComboBox::currentIndexChanged),
               this, &FaceAttendanceDialog::onCameraSelectionChanged);

    m_cmbCameraList->clear();

    QList<QCameraInfo> cameras = MyCamera::getAvailableCameras();
    if (cameras.isEmpty()) {
        m_cmbCameraList->addItem("⚠ 未检测到可用摄像头", -1);
        m_cmbCameraList->setEnabled(false);
        m_btnRefreshCameras->setEnabled(true);
        connect(m_cmbCameraList, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &FaceAttendanceDialog::onCameraSelectionChanged);
        updateUiForCameraState(false);
        return;
    }

    for (int i = 0; i < cameras.size(); ++i) {
        QString displayText = QString("[%1] %2").arg(i).arg(cameras[i].description());
        if (cameras[i].description().isEmpty()) {
            displayText = QString("[%1] %2").arg(i).arg(cameras[i].deviceName());
        }
        m_cmbCameraList->addItem(displayText, i);
    }

    m_cmbCameraList->setEnabled(true);
    m_btnRefreshCameras->setEnabled(true);

    // 选择当前正在使用的摄像头
    MyCamera *cam = MyCamera::getInstance();
    int currentIdx = cam->currentCameraIndex();
    if (currentIdx >= 0 && currentIdx < m_cmbCameraList->count()) {
        m_cmbCameraList->setCurrentIndex(currentIdx);
    } else if (m_cmbCameraList->count() > 0) {
        m_cmbCameraList->setCurrentIndex(0);
    }

    connect(m_cmbCameraList, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FaceAttendanceDialog::onCameraSelectionChanged);
}

void FaceAttendanceDialog::onCameraSelectionChanged(int index)
{
    if (index < 0) return;
    if (!m_cmbCameraList || !ui) return;

    int cameraIndex = m_cmbCameraList->itemData(index).toInt();
    if (cameraIndex < 0) return;

    MyCamera *cam = MyCamera::getInstance();
    if (!cam) return;
    if (cam->currentCameraIndex() == cameraIndex) {
        return;
    }

    ui->lblStatus->setText(QString("正在切换摄像头至: %1 ...").arg(m_cmbCameraList->itemText(index)));
    ui->lblStatus->setStyleSheet("color: #d69e2e; font-weight: bold; background-color: #fffff0; border: 1px solid #ecc94b; border-radius: 10px; padding: 8px; font-size: 14px;");
    QApplication::processEvents();

    bool ok = cam->switchCamera(cameraIndex);

    if (!ok) {
        ui->lblStatus->setText(QString("❌ 切换摄像头失败，请尝试其他摄像头或刷新列表"));
        ui->lblStatus->setStyleSheet("color: #c53030; font-weight: bold;");
        updateUiForCameraState(cam->isInitialized());
        m_statusCounter = 4;
        m_statusTimer->start(1000);
    }
}

void FaceAttendanceDialog::onRefreshCamerasClicked()
{
    if (!ui) return;

    ui->lblStatus->setText("正在重新扫描并初始化摄像头...");
    ui->lblStatus->setStyleSheet("color: #4299e1; font-weight: bold;");
    QApplication::processEvents();

    MyCamera *cam = MyCamera::getInstance();

    // 如果之前未初始化成功，现在尝试重新初始化（可能用户刚插上摄像头）
    bool reinitOk = true;
    if (!cam->isInitialized()) {
        reinitOk = cam->reinitialize();
    }

    refreshCameraList();

    int count = m_cmbCameraList->count();
    bool hasCamera = !(count == 0 || (count == 1 && m_cmbCameraList->itemData(0).toInt() < 0));

    if (hasCamera) {
        // 如果摄像头可用但单例没启动，启动它
        if (cam->isInitialized()) {
            cam->startCamera();
        }
        ui->lblCamera->clear();
        ui->lblStatus->setText(QString("✅ 扫描完成，共检测到 %1 个摄像头").arg(count));
        ui->lblStatus->setStyleSheet("color: #2f855a; font-weight: bold;");
        m_statusCounter = 3;
        m_statusTimer->start(1000);
    } else {
        ui->lblCamera->setText(QStringLiteral("⚠\n未检测到摄像头设备\n\n请插入摄像头后点击刷新按钮"));
        ui->lblStatus->setText("⚠ 未检测到任何摄像头设备，请插入摄像头后重试");
        ui->lblStatus->setStyleSheet("color: #c53030; font-weight: bold;");
    }

    updateUiForCameraState(cam->isInitialized());
    Q_UNUSED(reinitOk);
}

void FaceAttendanceDialog::onCameraSwitched(int index, const QString &description)
{
    Q_UNUSED(index);
    if (!ui) return;
    ui->lblStatus->setText(QString("✅ 已切换至摄像头: %1").arg(description));
    ui->lblStatus->setStyleSheet("color: #2f855a; font-weight: bold;");

    MyCamera *cam = MyCamera::getInstance();
    updateUiForCameraState(cam && cam->isInitialized());

    m_statusCounter = 3;
    m_statusTimer->start(1000);
}

void FaceAttendanceDialog::onStatusTimeout()
{
    m_statusCounter--;
    if (m_statusCounter <= 0) {
        m_statusTimer->stop();
        ui->lblStatus->setText("请正对摄像头，准备识别...");
        ui->lblStatus->setStyleSheet("");
    }
}

void FaceAttendanceDialog::onFrameCaptured(const QImage &image)
{
    if (!ui) return;
    if (image.isNull()) return;

    QImage displayImage = image.copy();
    QRect faceRect;
    bool hasFace = false;
    QString userName;
    QString cardNumber;
    bool isRegistered;

    {
        QMutexLocker locker(&m_cacheMutex);
        faceRect = m_cachedFaceRect;
        hasFace = m_hasFace;
        userName = m_userName;
        cardNumber = m_cardNumber;
        isRegistered = m_isRegistered;
    }

    if (hasFace) {
        QPainter painter(&displayImage);
        painter.setPen(QPen(Qt::green, 3));
        painter.drawRect(faceRect);

        QString displayText = isRegistered ? userName : "未注册用户";
        QFont font("Arial", 14, QFont::Bold);
        painter.setFont(font);
        painter.setPen(QPen(isRegistered ? Qt::darkGreen : Qt::red, 2));
        painter.drawText(faceRect.topLeft() + QPoint(0, -15), displayText);
    }

    if (!displayImage.isNull()) {
        int labelWidth = ui->lblCamera->width();
        float aspectRatio = static_cast<float>(image.width()) / image.height();
        int labelHeight = static_cast<int>(labelWidth / aspectRatio);
        ui->lblCamera->setMinimumHeight(labelHeight);
        ui->lblCamera->setPixmap(
            QPixmap::fromImage(displayImage)
                .scaled(labelWidth, labelHeight, Qt::KeepAspectRatio, Qt::FastTransformation)
        );
    }

    if (hasFace && isRegistered && !userName.isEmpty()) {
        ui->lblUserName->setText(userName + (cardNumber.isEmpty() ? "" : " (" + cardNumber + ")"));
    } else if (hasFace && !isRegistered) {
        ui->lblUserName->setText("未注册用户，请先注册人脸");
    } else {
        ui->lblUserName->setText("未检测到人脸");
    }

    {
        QMutexLocker locker(&m_cacheMutex);
        if (m_isDetecting) {
            return;
        }
        m_isDetecting = true;
    }

    QtConcurrent::run([this, image]() {
        QImage smallImage = image.scaledToWidth(480, Qt::FastTransformation);

        std::vector<float> feature;
        QRect smallRect;

        FaceRecognizer *fr = FaceRecognizer::getInstance();
        bool detected = fr && fr->detectFace(smallImage, feature, smallRect);

        QString userName = "";
        QString cardNumber = "";
        bool isRegistered = false;

        if (detected) {
            MySql *db = MySql::getMySql();
            std::vector<std::tuple<QString, QString, std::vector<float>>> users;
            if (db->getAllFaceFeatures(users) && !users.empty()) {
                float bestScore = 0.0f;
                QString bestName;
                QString bestCard;

                for (const auto& user : users) {
                    float score = fr->compareFeatures(feature, std::get<2>(user));
                    if (score > bestScore) {
                        bestScore = score;
                        bestCard = std::get<0>(user);
                        bestName = std::get<1>(user);
                    }
                }

                if (bestScore >= 0.65f) {
                    cardNumber = bestCard;
                    userName = bestName;
                    isRegistered = true;
                }
            }
        }

        float scaleX = static_cast<float>(image.width()) / smallImage.width();
        float scaleY = static_cast<float>(image.height()) / smallImage.height();
        QRect origRect;
        if (detected) {
            origRect = QRect(
                static_cast<int>(smallRect.x() * scaleX),
                static_cast<int>(smallRect.y() * scaleY),
                static_cast<int>(smallRect.width() * scaleX),
                static_cast<int>(smallRect.height() * scaleY)
            );
        }

        QMetaObject::invokeMethod(this, [this, detected, origRect, userName, cardNumber, isRegistered]() {
            QMutexLocker locker(&m_cacheMutex);
            m_hasFace = detected;
            m_userName = userName;
            m_cardNumber = cardNumber;
            m_isRegistered = isRegistered;
            m_isDetecting = false;
            if (detected) {
                m_cachedFaceRect = origRect;
            }
        }, Qt::QueuedConnection);
    });
}

bool FaceAttendanceDialog::recognizeUser(QString &cardNumber, QString &userName)
{
    MyCamera *cam = MyCamera::getInstance();
    if (!cam || !cam->isInitialized()) {
        return false;
    }
    QImage frame = cam->currentFrame();
    if (frame.isNull()) {
        return false;
    }

    // 缩放到与实时识别一致的宽度，避免大分辨率原图推理阻塞主线程（导致"未响应"）
    QImage smallFrame = frame.scaledToWidth(480, Qt::FastTransformation);

    FaceRecognizer *fr = FaceRecognizer::getInstance();
    std::vector<float> currentFeature;
    if (!fr || !fr->detectFace(smallFrame, currentFeature)) {
        return false;
    }

    MySql *db = MySql::getMySql();
    std::vector<std::tuple<QString, QString, std::vector<float>>> users;
    if (!db->getAllFaceFeatures(users) || users.empty()) {
        return false;
    }

    float bestScore = 0.0f;
    QString bestId;
    QString bestName;

    for (const auto& user : users) {
        const QString& card = std::get<0>(user);
        const QString& name = std::get<1>(user);
        const std::vector<float>& feature = std::get<2>(user);

        float score = fr->compareFeatures(currentFeature, feature);
        if (score > bestScore) {
            bestScore = score;
            bestId = card;
            bestName = name;
        }
    }

    if (bestScore >= 0.75f) {
        cardNumber = bestId;
        userName = bestName;
        return true;
    }

    return false;
}

QString FaceAttendanceDialog::determineSignType(int cardId, const QString &today)
{
    MySql *db = MySql::getMySql();
    QString lastType = db->getLastSignType(cardId, today);
    if (lastType == "签到") {
        return "签退";
    }
    return "签到";
}

QString FaceAttendanceDialog::checkTimeRule(const QString &type, int &minsLateEarly)
{
    MySql *db = MySql::getMySql();
    QTime now = QTime::currentTime();
    QTime signInDeadline = QTime::fromString(db->getWorkSignInDeadline(), "HH:mm");
    QTime signOffStart = QTime::fromString(db->getWorkSignOffStart(), "HH:mm");

    minsLateEarly = 0;

    if (type == "签到") {
        if (signInDeadline.isValid() && now > signInDeadline) {
            minsLateEarly = signInDeadline.secsTo(now) / 60;
            return QString("迟到 %1 分钟").arg(minsLateEarly);
        }
        return "正常";
    } else if (type == "签退") {
        if (signOffStart.isValid() && now < signOffStart) {
            minsLateEarly = now.secsTo(signOffStart) / 60;
            return QString("早退 %1 分钟").arg(minsLateEarly);
        }
        return "正常";
    }
    return "正常";
}

void FaceAttendanceDialog::on_btnCheckIn_clicked()
{
    MyCamera *cam = MyCamera::getInstance();
    if (!cam || !cam->isInitialized()) {
        QMessageBox::warning(this, "提示", "摄像头未就绪，请先插入摄像头并点击『刷新列表』！");
        return;
    }

    QString cardNumber;
    QString userName;

    if (!recognizeUser(cardNumber, userName)) {
        QMessageBox::warning(this, "提示", "未识别到已注册用户，请正对摄像头并确保已注册人脸！");
        return;
    }

    MySql *db = MySql::getMySql();
    int cardId = db->getCardIdByCardNumber(cardNumber);
    if (cardId == -1) {
        QMessageBox::critical(this, "错误", "该用户未开通一卡通！");
        return;
    }

    QString today = QDate::currentDate().toString("yyyy-MM-dd");
    QString lastType = db->getLastSignType(cardId, today);
    if (lastType == "签到") {
        QMessageBox::warning(this, "提示", QString("%1 今日已签到，请勿重复签到！").arg(userName));
        return;
    }

    int mins = 0;
    QString remark = checkTimeRule("签到", mins);
    if (remark.startsWith("迟到")) {
        QMessageBox::StandardButton btn = QMessageBox::question(this, "确认迟到",
            QString("%1 已迟到 %2 分钟，是否仍然签到？").arg(userName).arg(mins),
            QMessageBox::Yes | QMessageBox::No);
        if (btn != QMessageBox::Yes) return;
    }

    if (db->addSignRecord(cardId, userName, "签到", remark)) {
        QString timeStr = QTime::currentTime().toString("hh:mm:ss");
        QString msg = QString("✅ %1 (%2) 签到成功！\n时间：%3\n备注：%4")
                          .arg(userName).arg(cardNumber).arg(timeStr).arg(remark);
        ui->lblStatus->setText(msg);
        ui->lblStatus->setStyleSheet("color: #2f855a; font-weight: bold;");
        m_statusCounter = 5;
        m_statusTimer->start(1000);
        emit attendanceSuccess(cardNumber, userName, timeStr, "签到", remark);
        QMessageBox::information(this, "签到成功", msg);
    } else {
        QMessageBox::critical(this, "错误", "签到记录写入失败：" + db->lastError());
    }
}

void FaceAttendanceDialog::on_btnCheckOut_clicked()
{
    MyCamera *cam = MyCamera::getInstance();
    if (!cam || !cam->isInitialized()) {
        QMessageBox::warning(this, "提示", "摄像头未就绪，请先插入摄像头并点击『刷新列表』！");
        return;
    }

    QString cardNumber;
    QString userName;

    if (!recognizeUser(cardNumber, userName)) {
        QMessageBox::warning(this, "提示", "未识别到已注册用户，请正对摄像头并确保已注册人脸！");
        return;
    }

    MySql *db = MySql::getMySql();
    int cardId = db->getCardIdByCardNumber(cardNumber);
    if (cardId == -1) {
        QMessageBox::critical(this, "错误", "该用户未开通一卡通！");
        return;
    }

    QString today = QDate::currentDate().toString("yyyy-MM-dd");
    QString lastType = db->getLastSignType(cardId, today);
    if (lastType.isEmpty()) {
        QMessageBox::warning(this, "提示", QString("%1 今日尚未签到，无法签退！").arg(userName));
        return;
    }
    if (lastType == "签退") {
        QMessageBox::warning(this, "提示", QString("%1 今日已签退，请勿重复签退！").arg(userName));
        return;
    }

    int mins = 0;
    QString remark = checkTimeRule("签退", mins);
    if (remark.startsWith("早退")) {
        QMessageBox::StandardButton btn = QMessageBox::question(this, "确认早退",
            QString("%1 还未到签退时间（提前 %2 分钟），是否仍然签退？").arg(userName).arg(mins),
            QMessageBox::Yes | QMessageBox::No);
        if (btn != QMessageBox::Yes) return;
    }

    if (db->addSignRecord(cardId, userName, "签退", remark)) {
        QString timeStr = QTime::currentTime().toString("hh:mm:ss");
        QString msg = QString("✅ %1 (%2) 签退成功！\n时间：%3\n备注：%4")
                          .arg(userName).arg(cardNumber).arg(timeStr).arg(remark);
        ui->lblStatus->setText(msg);
        ui->lblStatus->setStyleSheet("color: #2f855a; font-weight: bold;");
        m_statusCounter = 5;
        m_statusTimer->start(1000);
        emit attendanceSuccess(cardNumber, userName, timeStr, "签退", remark);
        QMessageBox::information(this, "签退成功", msg);
    } else {
        QMessageBox::critical(this, "错误", "签退记录写入失败：" + db->lastError());
    }
}

void FaceAttendanceDialog::on_btnRegister_clicked()
{
    MyCamera *cam = MyCamera::getInstance();
    if (!cam || !cam->isInitialized()) {
        QMessageBox::warning(this, "提示", "摄像头未就绪，请先插入摄像头并点击『刷新列表』！");
        return;
    }

    bool ok;
    QString userName = QInputDialog::getText(this, "注册人脸",
        "请输入要注册的用户名（与系统用户一致）：", QLineEdit::Normal, "", &ok);
    if (!ok || userName.trimmed().isEmpty()) {
        return;
    }
    userName = userName.trimmed();

    MySql *db = MySql::getMySql();
    QList<QMap<QString, QString>> users = db->getAllUsers(userName);
    bool userExists = false;
    for (const auto& u : users) {
        if (u["name"] == userName) {
            userExists = true;
            break;
        }
    }
    if (!userExists) {
        QMessageBox::warning(this, "提示", QString("系统中不存在用户 \"%1\"，请先在用户管理中创建该用户！").arg(userName));
        return;
    }

    QImage frame = cam->currentFrame();
    if (frame.isNull()) {
        QMessageBox::warning(this, "提示", "摄像头尚未采集到图像，请稍后再试！");
        return;
    }

    std::vector<float> feature;
    FaceRecognizer *fr = FaceRecognizer::getInstance();
    if (!fr || !fr->detectFace(frame, feature)) {
        QMessageBox::warning(this, "提示", "未检测到人脸，请调整姿势后重试！");
        return;
    }

    if (db->addFaceFeature(userName, feature)) {
        QMessageBox::information(this, "成功", QString("用户 %1 的人脸注册成功！").arg(userName));
    } else {
        QMessageBox::critical(this, "错误", "人脸注册失败：" + db->lastError());
    }
}

void FaceAttendanceDialog::on_btnClose_clicked()
{
    accept();
}
