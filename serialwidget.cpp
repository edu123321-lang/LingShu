#include "serialwidget.h"
#include "ui_serialwidget.h"
#include <QDebug>

SerialWidget::SerialWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SerialWidget)
{
    ui->setupUi(this);
    m_buffer.clear();
    serial = new QSerialPort(this);
    // ---- 扫描可用串口并填充下拉框 ----
       ui->comboBoxPort->clear();
       foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
           ui->comboBoxPort->addItem(info.portName());
       }
       if (ui->comboBoxPort->count() == 0) {
           ui->comboBoxPort->addItem("无可用串口");
       }

       // ---- 填充其他下拉框 ----
       ui->comboBoxBaud->addItems({"9600", "19200", "38400", "57600", "115200"});
       ui->comboBoxDataBits->addItems({"8", "7", "6", "5"});
       ui->comboBoxStopBits->addItems({"1", "1.5", "2"});
       ui->comboBoxParity->addItems({"无", "奇", "偶", "标记", "空格"});
       ui->comboBoxFlowControl->addItems({"无", "硬件", "软件"});

       // ---- 信号槽连接 ----
       connect(ui->clearLogBtn, &QPushButton::clicked, this, &SerialWidget::on_clearLogBtn_clicked);
       connect(serial, &QSerialPort::readyRead, this, &SerialWidget::readSerialData);


       // ---- 日志区设置为只读 ----
       ui->plainTextEditLog->setReadOnly(true);
       ui->plainTextEditLog->setLineWrapMode(QPlainTextEdit::WidgetWidth);
       // ---- 按钮初始文字 ----
       ui->openCloseBtn->setText("打开串口");
}

SerialWidget::~SerialWidget()
{
    if (serial->isOpen()) {
        serial->close();
    }
    delete ui;
}

void SerialWidget::on_openCloseBtn_clicked()
{
    qDebug() << "on_openCloseBtn_clicked called, isOpen=" << serial->isOpen();
    if (serial->isOpen())
    {
    serial->close();
    ui->openCloseBtn->setText("打开串口");
    ui->comboBoxPort->setEnabled(true);
    ui->comboBoxBaud->setEnabled(true);
    ui->comboBoxDataBits->setEnabled(true);
    ui->comboBoxStopBits->setEnabled(true);
    ui->comboBoxParity->setEnabled(true);
    ui->comboBoxFlowControl->setEnabled(true);
    appendLog("串口已关闭", false);
    return;
    }
    // 准备打开
    QString portName = ui->comboBoxPort->currentText();
    if (portName.isEmpty() || portName == "无可用串口") {
        QMessageBox::warning(this, "提示", "请选择有效的串口号");
        return;
    }
    serial->setPortName(portName);//串口名
    serial->setBaudRate(ui->comboBoxBaud->currentText().toInt());//波特率

    // 数据位
        int dataBits = ui->comboBoxDataBits->currentText().toInt();
        serial->setDataBits(static_cast<QSerialPort::DataBits>(dataBits));

        // 停止位
        QString stopBitsStr = ui->comboBoxStopBits->currentText();
        if (stopBitsStr == "1") serial->setStopBits(QSerialPort::OneStop);
        else if (stopBitsStr == "1.5") serial->setStopBits(QSerialPort::OneAndHalfStop);
        else if (stopBitsStr == "2") serial->setStopBits(QSerialPort::TwoStop);

        // 校验位
        QString parityStr = ui->comboBoxParity->currentText();
        if (parityStr == "无") serial->setParity(QSerialPort::NoParity);
        else if (parityStr == "奇") serial->setParity(QSerialPort::OddParity);
        else if (parityStr == "偶") serial->setParity(QSerialPort::EvenParity);
        else if (parityStr == "标记") serial->setParity(QSerialPort::MarkParity);
        else if (parityStr == "空格") serial->setParity(QSerialPort::SpaceParity);

        // 流控制
        QString flowStr = ui->comboBoxFlowControl->currentText();
        if (flowStr == "无") serial->setFlowControl(QSerialPort::NoFlowControl);
        else if (flowStr == "硬件") serial->setFlowControl(QSerialPort::HardwareControl);
        else if (flowStr == "软件") serial->setFlowControl(QSerialPort::SoftwareControl);

        if (serial->open(QIODevice::ReadWrite)) {
                ui->openCloseBtn->setText("关闭串口");
                ui->comboBoxPort->setEnabled(false);
                ui->comboBoxBaud->setEnabled(false);
                ui->comboBoxDataBits->setEnabled(false);
                ui->comboBoxStopBits->setEnabled(false);
                ui->comboBoxParity->setEnabled(false);
                ui->comboBoxFlowControl->setEnabled(false);
                appendLog("串口打开成功", false);
            } else {
                QMessageBox::critical(this, "错误", "打开串口失败：" + serial->errorString());
            }
        }


void SerialWidget::on_clearLogBtn_clicked()
{
    ui->plainTextEditLog->clear();
}

void SerialWidget::on_sendButton_clicked()
{
    if (!serial->isOpen()) {
            QMessageBox::warning(this, "提示", "串口未打开，无法发送");
            return;
        }
        QString data = ui->sendLineEdit->text();
        if (data.isEmpty()) {
            QMessageBox::warning(this, "提示", "请输入要发送的数据");
            return;
        }
        QByteArray sendData = data.toLocal8Bit();
        qint64 written = serial->write(sendData);
        if (written == -1) {
            QMessageBox::critical(this, "错误", "发送失败：" + serial->errorString());
        } else {
            appendLog(data, true);
            ui->sendLineEdit->clear();
        }
}

void SerialWidget::readSerialData()
{
    // 追加到缓冲区
      m_buffer.append(serial->readAll());

      // 按行分割（以 \r\n 或 \n 结尾）
      int start = 0;
      int end;
      while ((end = m_buffer.indexOf('\n', start)) != -1) {
          // 提取一行（包含 \n）
          QByteArray line = m_buffer.mid(start, end - start + 1);
          // 去掉末尾换行符
          line = line.trimmed();  // 去掉首尾空白（包括 \r\n）

          if (!line.isEmpty()) {
              // 尝试用 UTF-8 解码
              QString text = QString::fromUtf8(line);
              // 检查解码是否成功（如果包含乱码则表明不是有效 UTF-8）
              if (text.isNull() || text.isEmpty() || text.contains(QChar::ReplacementCharacter)) {
                  // 如果解码失败，显示十六进制
                  QString hex = line.toHex(' ').toUpper();
                  appendLog("HEX: " + hex, false);
              } else {
                  // 显示解码后的文本
                  appendLog("DATA: " + text, false);

              }
          }

          // 移动到下一行
          start = end + 1;
      }

      // 保留未处理完的剩余数据（可能是不完整的行）
      if (start < m_buffer.size()) {
          m_buffer = m_buffer.mid(start);
      } else {
          m_buffer.clear();
      }
}

void SerialWidget::appendLog(const QString &text, bool isSend)
{
    QString prefix = isSend ? "SEND" : "RECV";
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString line = QString("[%1] %2 %3").arg(timestamp).arg(prefix).arg(text);
    ui->plainTextEditLog->appendPlainText(line);
}
