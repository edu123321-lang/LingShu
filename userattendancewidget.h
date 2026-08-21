#ifndef USERATTENDANCEWIDGET_H
#define USERATTENDANCEWIDGET_H

#include <QWidget>
#include <QSerialPort>
#include <QTimer>
#include "mysql.h"

namespace Ui {
class userAttendanceWidget;
}

class FaceAttendanceDialog;

class userAttendanceWidget : public QWidget
{
    Q_OBJECT

public:
    explicit userAttendanceWidget(QWidget *parent = nullptr);
    ~userAttendanceWidget();

    void setUserName(const QString &name);

private slots:
    void on_refreshBtn_clicked();

    // 刷卡考勤
    void on_cardAttendanceBtn_clicked();
    void on_refreshSerialBtn_clicked();
    void readSerialData();
    void onSerialTimeout();

    // 刷脸考勤
    void on_faceAttendanceBtn_clicked();
    void onFaceAttendanceSuccess(const QString &cardNumber, const QString &userName,
                                  const QString &time, const QString &type, const QString &remark);

private:
    Ui::userAttendanceWidget *ui;
    QString m_userName;
    QSerialPort *m_serial;
    QByteArray m_serialBuffer;
    QTimer *m_serialTimer;
    FaceAttendanceDialog *m_faceDialog;

    void loadData();
    void updateStatus();
    void processCard(const QString &cardNumber);
    void refreshSerialPorts();
    void processSerialLine(const QByteArray &line);
    bool openSerialPort();
    void closeSerialPort();
};

#endif // USERATTENDANCEWIDGET_H
