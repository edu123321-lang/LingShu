#ifndef FACEATTENDANCEDIALOG_H
#define FACEATTENDANCEDIALOG_H

#include <QDialog>
#include <QImage>
#include <QRect>
#include <QMutex>
#include <QTimer>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QCameraInfo>

namespace Ui {
class FaceAttendanceDialog;
}

class FaceAttendanceDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FaceAttendanceDialog(QWidget *parent = nullptr);
    ~FaceAttendanceDialog();

signals:
    void attendanceSuccess(const QString &cardNumber, const QString &userName,
                           const QString &time, const QString &type, const QString &remark);

private slots:
    void onFrameCaptured(const QImage &image);
    void on_btnCheckIn_clicked();
    void on_btnCheckOut_clicked();
    void on_btnRegister_clicked();
    void on_btnClose_clicked();
    void onStatusTimeout();
    void onCameraSelectionChanged(int index);
    void onRefreshCamerasClicked();
    void onCameraSwitched(int index, const QString &description);
    void onCameraInitFailed(const QString &reason);

private:
    void setupCameraSelector();
    void refreshCameraList();
    void updateUiForCameraState(bool cameraOk);  // 根据摄像头状态启用/禁用考勤按钮
    bool recognizeUser(QString &cardNumber, QString &userName);
    QString determineSignType(int cardId, const QString &today);
    QString checkTimeRule(const QString &type, int &minsLateEarly);

private:
    Ui::FaceAttendanceDialog *ui;

    // 摄像头选择相关UI
    QLabel *m_lblCameraSelect;
    QComboBox *m_cmbCameraList;
    QPushButton *m_btnRefreshCameras;
    QHBoxLayout *m_cameraSelectorLayout;
    QWidget *m_cameraSelectorWidget;

    QRect m_cachedFaceRect;
    bool m_hasFace = false;
    QString m_userName;
    QString m_cardNumber;
    bool m_isRegistered = false;
    bool m_isDetecting = false;
    QMutex m_cacheMutex;

    QTimer *m_statusTimer;
    int m_statusCounter = 0;
};

#endif // FACEATTENDANCEDIALOG_H
