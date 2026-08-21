#ifndef GMWIDGET_H
#define GMWIDGET_H

#include <QWidget>
#include <QSerialPort>
#include <QSerialPortInfo>

namespace Ui {
class Gmwidget;
}

class Gmwidget : public QWidget
{
    Q_OBJECT

public:
    explicit Gmwidget(QWidget *parent = nullptr);
    ~Gmwidget();

private slots:
    void on_registeAccountBtn_clicked();

    void on_allinoneBtn_clicked();

    void on_attendanceBtn_clicked();

    void on_serialPortConfigurationBtn_clicked();

    void on_noticeBtn_clicked();

    void on_terminalBtn_clicked();

    void on_registerBtn_clicked();


    // ----- 新增槽函数 -----
    void onScanHintLabelClicked();   // 点击“刷卡获取卡号”标签
    void readCardSerialData();       // 串口数据到达时的处理

    void on_logoutBtn_clicked();

    void on_userManageBtn_clicked();

private:
    Ui::Gmwidget *ui;
    QSerialPort *m_serialCard;
    QByteArray m_serialBuffer;

    // 辅助函数：尝试打开串口（自动选择可用端口）
    bool openSerialForCard();
    void processSerialLine(const QByteArray &line); // 新增：处理一行数据
    void resetLabelToClickable();   // 将标签恢复为可点击的超链接状态

signals:
    void logoutRequested();
};

#endif // GMWIDGET_H
