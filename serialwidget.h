#ifndef SERIALWIDGET_H
#define SERIALWIDGET_H

#include <QWidget>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QDateTime>
#include <QMessageBox>

namespace Ui {
class SerialWidget;
}

class SerialWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SerialWidget(QWidget *parent = nullptr);
    ~SerialWidget();

private slots:
    void on_openCloseBtn_clicked();

    void on_clearLogBtn_clicked();

    void on_sendButton_clicked();
    void readSerialData();

private:
    Ui::SerialWidget *ui;
    QSerialPort *serial;
    void appendLog(const QString &text,bool isSend);
     QByteArray m_buffer;   // 缓存未处理的数据
};

#endif // SERIALWIDGET_H
