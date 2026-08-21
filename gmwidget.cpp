#include "gmwidget.h"
#include "ui_gmwidget.h"
#include "mysql.h"
#include <QMessageBox>
#include <QDebug>
#include <QSqlQuery>
#include <QSerialPortInfo>
#include <QGraphicsDropShadowEffect>
#include <QButtonGroup>

Gmwidget::Gmwidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Gmwidget)
{
    ui->setupUi(this);
    m_serialCard = new QSerialPort(this);
    connect(m_serialCard, &QSerialPort::readyRead, this, &Gmwidget::readCardSerialData);

    ui->scanHintLabel->setText("<a href=\"#\">点击此处刷卡获取卡号</a>");
    ui->scanHintLabel->setAlignment(Qt::AlignCenter);
    connect(ui->scanHintLabel, &QLabel::linkActivated, this, &Gmwidget::onScanHintLabelClicked);
    m_serialBuffer.clear();

    ui->registeAccountBtn->setCheckable(true);
    ui->allinoneBtn->setCheckable(true);
    ui->attendanceBtn->setCheckable(true);
    ui->serialPortConfigurationBtn->setCheckable(true);
    ui->noticeBtn->setCheckable(true);
    ui->terminalBtn->setCheckable(true);
    ui->userManageBtn->setCheckable(true);
    ui->registeAccountBtn->setChecked(true);

    QButtonGroup *navGroup = new QButtonGroup(this);
    navGroup->setExclusive(true);
    navGroup->addButton(ui->registeAccountBtn);
    navGroup->addButton(ui->allinoneBtn);
    navGroup->addButton(ui->attendanceBtn);
    navGroup->addButton(ui->serialPortConfigurationBtn);
    navGroup->addButton(ui->noticeBtn);
    navGroup->addButton(ui->terminalBtn);
    navGroup->addButton(ui->userManageBtn);

    setMinimumSize(1200, 760);
}

Gmwidget::~Gmwidget()
{
    delete ui;
}

void Gmwidget::on_registeAccountBtn_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
}

void Gmwidget::on_allinoneBtn_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
}

void Gmwidget::on_attendanceBtn_clicked()
{
    ui->stackedWidget->setCurrentIndex(2);
}

void Gmwidget::on_serialPortConfigurationBtn_clicked()
{
    ui->stackedWidget->setCurrentIndex(3);
}

void Gmwidget::on_noticeBtn_clicked()
{
    ui->stackedWidget->setCurrentIndex(4);
}

void Gmwidget::on_terminalBtn_clicked()
{
    ui->stackedWidget->setCurrentIndex(5);
}

void Gmwidget::on_registerBtn_clicked()
{
    QString name = ui->userNameLineEdit->text();
       QString pwd = ui->userpwdLineEdit->text();
       QString age = ui->userAgeLineEdit->text();
       QString card = ui->userCardLineEdit->text();
       QString sex;
       if (ui->maleRadioButton->isChecked()) {
           sex = "男";
       } else if (ui->femaleRadioButton->isChecked()) {
           sex = "女";
       } else {
           QMessageBox::warning(this, "提示", "请选择性别");
           return;
       }

       if (name.trimmed().isEmpty() || pwd.trimmed().isEmpty() || card.trimmed().isEmpty()) {
           QMessageBox::warning(this, "提示", "用户名、密码和卡号不能为空");
           return;
       }

       MySql *db = MySql::getMySql();
       QString normalizedCard = db->normalizedCardNumber(card);

       // 检查卡号是否已被占用
       QSqlQuery checkQuery;
       checkQuery.prepare("SELECT name FROM user WHERE card = :card");
       checkQuery.bindValue(":card", normalizedCard);
       if (checkQuery.exec() && checkQuery.next()) {
           QString existingUser = checkQuery.value(0).toString();
           QMessageBox::warning(this, "提示",
               QString("卡号 %1 已被用户 %2 占用，请使用其他卡号").arg(card).arg(existingUser));
           return; // 卡号已存在，不执行插入
       }

       // 执行插入
       db->insertData(name, pwd, age, card, sex);

       // 验证是否插入成功
       QSqlQuery verifyQuery;
       verifyQuery.prepare("SELECT name FROM user WHERE card = :card");
       verifyQuery.bindValue(":card", normalizedCard);
       if (verifyQuery.exec() && verifyQuery.next()) {
           QMessageBox::information(this, "成功", "用户注册成功！");
           // 可选：清空输入框
       } else {
           QMessageBox::warning(this, "错误", "注册失败，请检查数据库或重试");
       }
    }

//点击此处刷卡获取卡号
void Gmwidget::onScanHintLabelClicked()
{
    if (m_serialCard->isOpen()) {
            m_serialCard->close();
            resetLabelToClickable();
        }

        m_serialBuffer.clear();

        if (!openSerialForCard()) {
            resetLabelToClickable();
            QMessageBox::warning(this, "错误", "无法打开串口，请检查连接");
            return;
        }

        ui->scanHintLabel->setText("请刷卡...");   // 变为普通文本，不可点击
        ui->userCardLineEdit->clear();
        qDebug() << "串口已打开，等待数据...";
}

//读取串口数据解析卡号
void Gmwidget::readCardSerialData()
{
    QByteArray newData = m_serialCard->readAll();
        if (newData.isEmpty()) return;

        m_serialBuffer.append(newData);

        // 尝试按行处理
        int index = m_serialBuffer.indexOf('\n');
        if (index != -1) {
            while (index != -1) {
                QByteArray line = m_serialBuffer.left(index + 1);
                m_serialBuffer.remove(0, index + 1);
                processSerialLine(line.trimmed());
                index = m_serialBuffer.indexOf('\n');
            }
        } else {
            // 没有换行，如果缓冲区大于32字节，强制处理整个缓冲区
            if (m_serialBuffer.size() > 32) {
                processSerialLine(m_serialBuffer);
                m_serialBuffer.clear();
            }
        }

        // 如果缓冲区过大（超过128字节）仍未处理，清空并恢复
        if (m_serialBuffer.size() > 128) {
            qDebug() << "缓冲区过大，清空";
            m_serialBuffer.clear();
            ui->scanHintLabel->setText("点击此处刷卡获取卡号");
            if (m_serialCard->isOpen()) m_serialCard->close();
        }
    }


//自动选择一个可用的串口
bool Gmwidget::openSerialForCard()
{
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
       if (ports.isEmpty()) {
           qDebug() << "没有可用串口";
           return false;
       }

       foreach (const QSerialPortInfo &info, ports) {
           qDebug() << "尝试打开串口:" << info.portName();
           m_serialCard->setPortName(info.portName());
           m_serialCard->setBaudRate(QSerialPort::Baud115200);
           m_serialCard->setDataBits(QSerialPort::Data8);
           m_serialCard->setParity(QSerialPort::NoParity);
           m_serialCard->setStopBits(QSerialPort::OneStop);
           m_serialCard->setFlowControl(QSerialPort::NoFlowControl);

           if (m_serialCard->open(QIODevice::ReadOnly)) {
               qDebug() << "成功打开串口:" << info.portName();
               return true;
           } else {
               qDebug() << "打开串口失败:" << info.portName() << m_serialCard->errorString();
           }
       }
       return false;
}

void Gmwidget::processSerialLine(const QByteArray &line)
{
    if (line.isEmpty()) return;

        qDebug() << "处理行 (hex):" << line.toHex();
        qDebug() << "处理行 (ASCII):" << line;

        QString raw = QString::fromUtf8(line);
        if (raw.contains(QChar::ReplacementCharacter)) {
            raw = QString::fromLatin1(line);
        }

        // 只处理包含“卡号”的行
        if (!raw.contains("卡号")) {
            qDebug() << "该行不包含'卡号'，忽略（保持串口打开）";
            return;
        }

        int idx = raw.indexOf("卡号");
        if (idx == -1) return;

        // 找冒号（中文和英文）
        int colonPos = raw.indexOf(':', idx);
        if (colonPos == -1) {
            colonPos = raw.indexOf(QString::fromUtf8("："), idx);
        }
        if (colonPos == -1) {
            colonPos = idx + 2;   // 默认跳过“卡号”两字
        }

        QString after = raw.mid(colonPos + 1).trimmed();

        // 提取字母、数字、横线
        QString cardNumber;
        for (QChar ch : after) {
            if (ch.isLetterOrNumber() || ch == '-') {
                cardNumber.append(ch);
            }
        }

        if (cardNumber.isEmpty()) {
            qDebug() << "提取卡号为空，忽略（保持串口打开）";
            return;
        }

        QString normalized = MySql::getMySql()->normalizedCardNumber(cardNumber);
        if (!normalized.isEmpty() && normalized.length() >= 4) {
            ui->userCardLineEdit->setText(normalized);
            resetLabelToClickable();   // 恢复超链接
            if (m_serialCard->isOpen()) {
                m_serialCard->close();
            }
            ui->userpwdLineEdit->setFocus();
            qDebug() << "卡号填入成功:" << normalized;
        } else {
            qDebug() << "卡号无效，忽略（保持串口打开）";
        }
}

void Gmwidget::resetLabelToClickable()
{
    ui->scanHintLabel->setText("<a href=\"#\">点击此处刷卡获取卡号</a>");
    ui->scanHintLabel->setAlignment(Qt::AlignCenter);
}



void Gmwidget::on_logoutBtn_clicked()
{
    if (QMessageBox::question(this, "退出系统", "确定要退出系统吗？",
                                  QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
            emit logoutRequested();
        }
}

void Gmwidget::on_userManageBtn_clicked()
{
     ui->stackedWidget->setCurrentIndex(6);
}
