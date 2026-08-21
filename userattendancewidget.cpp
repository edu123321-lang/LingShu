#include "userattendancewidget.h"
#include "ui_userattendancewidget.h"
#include "faceattendancedialog.h"
#include <QSqlQuery>
#include <QMessageBox>
#include <QSerialPortInfo>
#include <QDateTime>
#include <QTime>
#include <QDebug>

userAttendanceWidget::userAttendanceWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::userAttendanceWidget),
    m_serial(nullptr),
    m_serialTimer(nullptr),
    m_faceDialog(nullptr)
{
    ui->setupUi(this);

    m_serial = new QSerialPort(this);
    connect(m_serial, &QSerialPort::readyRead, this, &userAttendanceWidget::readSerialData);
    m_serialBuffer.clear();

    m_serialTimer = new QTimer(this);
    m_serialTimer->setInterval(200);
    m_serialTimer->setSingleShot(false);
    connect(m_serialTimer, &QTimer::timeout, this, &userAttendanceWidget::onSerialTimeout);

    ui->recordTable->setColumnCount(4);
    ui->recordTable->setHorizontalHeaderLabels({"日期", "时间", "类型", "备注"});
    ui->recordTable->horizontalHeader()->setStretchLastSection(true);
    ui->recordTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    refreshSerialPorts();
    ui->cardStatusLabel->setText("就绪");
}

userAttendanceWidget::~userAttendanceWidget()
{
    if (m_serial && m_serial->isOpen()) {
        m_serial->close();
    }
    delete ui;
}

void userAttendanceWidget::setUserName(const QString &name)
{
    m_userName = name;
    loadData();
}

void userAttendanceWidget::on_refreshBtn_clicked()
{
    loadData();
}

void userAttendanceWidget::on_refreshSerialBtn_clicked()
{
    refreshSerialPorts();
}

void userAttendanceWidget::refreshSerialPorts()
{
    ui->serialPortCombo->clear();
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    if (ports.isEmpty()) {
        ui->serialPortCombo->addItem("无可用串口");
        ui->serialPortCombo->setEnabled(false);
        return;
    }
    foreach (const QSerialPortInfo &info, ports) {
        QString display = QString("%1 (%2)").arg(info.portName()).arg(info.description().isEmpty() ? QString("读卡器") : info.description());
        ui->serialPortCombo->addItem(display, info.portName());
    }
    ui->serialPortCombo->setEnabled(true);
}

void userAttendanceWidget::on_cardAttendanceBtn_clicked()
{
    if (m_userName.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先登录");
        return;
    }

    if (m_serial->isOpen()) {
        closeSerialPort();
        ui->cardAttendanceBtn->setText("💳 开始刷卡考勤");
        ui->cardAttendanceBtn->setStyleSheet("QPushButton {\n"
            "background: qlineargradient(x1:0, y1:0, x2:1, y2:0,\n"
            "    stop:0 #38b2ac,\n"
            "    stop:1 #4fd1c5);\n"
            "color: white;\n"
            "font-size: 15px;\n"
            "font-weight: bold;\n"
            "border-radius: 12px;\n"
            "min-height: 50px;\n"
            "}\n"
            "QPushButton:hover {\n"
            "background: qlineargradient(x1:0, y1:0, x2:1, y2:0,\n"
            "    stop:0 #319795,\n"
            "    stop:1 #38b2ac);\n"
            "}");
        ui->serialPortCombo->setEnabled(true);
        ui->baudRateCombo->setEnabled(true);
        ui->refreshSerialBtn->setEnabled(true);
        ui->cardAttendanceBtn->setEnabled(true);
        return;
    }

    if (ui->serialPortCombo->count() == 0 || ui->serialPortCombo->currentData().toString().isEmpty()) {
        QMessageBox::warning(this, "提示", "请先选择串口，如无可用串口请检查读卡器连接后点击刷新");
        return;
    }

    ui->cardAttendanceBtn->setEnabled(false);
    m_serialBuffer.clear();
    if (!openSerialPort()) {
        QString portName = ui->serialPortCombo->currentData().toString();
        ui->cardAttendanceBtn->setEnabled(true);
        QMessageBox::warning(this, "错误", QString("无法打开串口 %1，请检查读卡器连接").arg(portName));
        return;
    }
    ui->cardAttendanceBtn->setText("⏹️ 停止刷卡考勤");
    ui->cardAttendanceBtn->setStyleSheet("QPushButton {\n"
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:0,\n"
        "    stop:0 #e53e3e,\n"
        "    stop:1 #f56565);\n"
        "color: white;\n"
        "font-size: 15px;\n"
        "font-weight: bold;\n"
        "border-radius: 12px;\n"
        "min-height: 50px;\n"
        "}\n"
        "QPushButton:hover {\n"
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:0,\n"
        "    stop:0 #c53030,\n"
        "    stop:1 #e53e3e);\n"
        "}");
    ui->serialPortCombo->setEnabled(false);
    ui->baudRateCombo->setEnabled(false);
    ui->refreshSerialBtn->setEnabled(false);
    ui->cardAttendanceBtn->setEnabled(true);
    m_serialTimer->start();
    ui->cardStatusLabel->setText(QString("已连接 %1 @ %2，请刷卡...")
        .arg(ui->serialPortCombo->currentData().toString())
        .arg(ui->baudRateCombo->currentText().split(' ').first()));
}

bool userAttendanceWidget::openSerialPort()
{
    QString portName = ui->serialPortCombo->currentData().toString();
    if (portName.isEmpty()) {
        qDebug() << "未选择串口";
        return false;
    }

    if (m_serial->isOpen()) {
        qDebug() << "串口已打开，先关闭:" << m_serial->portName();
        m_serial->close();
    }

    QString baudText = ui->baudRateCombo->currentText().split(' ').first();
    qint32 baudRate = baudText.toInt();
    if (baudRate <= 0) {
        baudRate = QSerialPort::Baud115200;
    }
    qDebug() << "[打开串口] port:" << portName << "baud:" << baudRate;

    m_serial->setPortName(portName);
    m_serial->setBaudRate(baudRate);
    m_serial->setDataBits(QSerialPort::Data8);
    m_serial->setParity(QSerialPort::NoParity);
    m_serial->setStopBits(QSerialPort::OneStop);
    m_serial->setFlowControl(QSerialPort::NoFlowControl);

    if (m_serial->open(QIODevice::ReadWrite)) {
        m_serial->setDataTerminalReady(true);
        m_serial->setRequestToSend(true);
        qDebug() << "[打开成功] DTR=" << m_serial->isDataTerminalReady()
                 << "RTS=" << m_serial->isRequestToSend();
        qDebug() << "  实际配置: baud=" << m_serial->baudRate()
                 << "databits=" << m_serial->dataBits()
                 << "parity=" << m_serial->parity()
                 << "stopbits=" << m_serial->stopBits();
        return true;
    } else {
        qDebug() << "[打开失败]:" << m_serial->errorString();
        return false;
    }
}

void userAttendanceWidget::closeSerialPort()
{
    if (m_serialTimer && m_serialTimer->isActive()) {
        m_serialTimer->stop();
    }
    if (m_serial->isOpen()) {
        qDebug() << "[关闭串口]" << m_serial->portName();
        m_serial->setDataTerminalReady(false);
        m_serial->setRequestToSend(false);
        m_serial->close();
    }
    m_serialBuffer.clear();
    ui->cardStatusLabel->setText("就绪");
}

void userAttendanceWidget::onSerialTimeout()
{
    if (!m_serial || !m_serial->isOpen()) return;

    if (m_serialBuffer.size() > 0) {
        qDebug() << "[Timer] 缓冲区有数据:" << m_serialBuffer.size() << "字节, 强制处理";
        processSerialLine(m_serialBuffer);
        m_serialBuffer.clear();
    }
}

void userAttendanceWidget::readSerialData()
{
    QByteArray newData = m_serial->readAll();
    if (newData.isEmpty()) return;

    qDebug() << "[串口收到原始数据 hex]:" << newData.toHex(' ').toUpper();
    qDebug() << "[串口收到原始数据 ascii]:" << newData;

    m_serialBuffer.append(newData);

    int index = m_serialBuffer.indexOf('\n');
    if (index != -1) {
        while (index != -1) {
            QByteArray line = m_serialBuffer.left(index + 1);
            m_serialBuffer.remove(0, index + 1);
            processSerialLine(line.trimmed());
            index = m_serialBuffer.indexOf('\n');
        }
    } else {
        if (m_serialBuffer.size() > 32) {
            qDebug() << "[无换行符，缓冲区达32字节，强制处理]";
            processSerialLine(m_serialBuffer);
            m_serialBuffer.clear();
        }
    }

    if (m_serialBuffer.size() > 128) {
        qDebug() << "[缓冲区过大(>128)，清空]";
        m_serialBuffer.clear();
    }
}

void userAttendanceWidget::processSerialLine(const QByteArray &line)
{
    if (line.isEmpty()) return;

    qDebug() << "[处理行 hex]:" << line.toHex(' ').toUpper();
    qDebug() << "[处理行 ascii]:" << line;

    QString raw = QString::fromUtf8(line);
    if (raw.contains(QChar::ReplacementCharacter)) {
        raw = QString::fromLatin1(line);
    }
    qDebug() << "[处理行 解码后]:" << raw;

    QString cardNumber;

    if (raw.contains("卡号")) {
        int idx = raw.indexOf("卡号");
        int colonPos = raw.indexOf(':', idx);
        if (colonPos == -1) {
            colonPos = raw.indexOf(QString::fromUtf8("："), idx);
        }
        if (colonPos == -1) {
            colonPos = idx + 2;
        }
        QString after = raw.mid(colonPos + 1).trimmed();
        for (QChar ch : after) {
            if (ch.isLetterOrNumber() || ch == '-') {
                cardNumber.append(ch);
            }
        }
    } else {
        for (QChar ch : raw) {
            if (ch.isLetterOrNumber() || ch == '-') {
                cardNumber.append(ch);
            }
        }
    }

    qDebug() << "[提取到卡号候选]:" << cardNumber;

    QString normalized = MySql::getMySql()->normalizedCardNumber(cardNumber);
    qDebug() << "[标准化后卡号]:" << normalized << "长度:" << normalized.length();

    if (!normalized.isEmpty() && normalized.length() >= 4) {
        ui->cardStatusLabel->setText(QString("📥 识别到卡号：%1，处理中...").arg(normalized));
        processCard(normalized);
    } else {
        if (!cardNumber.isEmpty()) {
            ui->cardStatusLabel->setText(QString("⚠️ 收到数据但卡号无效：原始=\"%1\" 提取=\"%2\"").arg(raw.left(30)).arg(cardNumber.left(20)));
        }
    }
}

void userAttendanceWidget::processCard(const QString &cardNumber)
{
    QString normalizedCard = MySql::getMySql()->normalizedCardNumber(cardNumber);
    MySql *db = MySql::getMySql();

    QString cardUserName = db->getUserNameByCardNumber(normalizedCard);
    if (cardUserName.isEmpty()) {
        ui->cardStatusLabel->setText("❌ 未找到该卡号对应的用户");
        return;
    }

    if (cardUserName != m_userName) {
        ui->cardStatusLabel->setText(QString("❌ 该卡属于用户 %1，与当前登录用户不符").arg(cardUserName));
        QMessageBox::warning(this, "用户不符",
            QString("当前登录用户是：%1\n此卡片属于用户：%2\n请使用自己的卡片").arg(m_userName).arg(cardUserName));
        return;
    }

    int cardId = db->getCardIdByCardNumber(normalizedCard);
    if (cardId == -1) {
        ui->cardStatusLabel->setText("❌ 该用户未开通一卡通");
        return;
    }

    QString today = QDate::currentDate().toString("yyyy-MM-dd");
    QString lastType = db->getLastSignType(cardId, today);
    QString type;
    if (lastType == "签到") {
        type = "签退";
    } else {
        type = "签到";
    }

    QTime now = QTime::currentTime();
    QTime signInDeadline = QTime::fromString(db->getWorkSignInDeadline(), "HH:mm");
    QTime signOffStart = QTime::fromString(db->getWorkSignOffStart(), "HH:mm");
    QString remark;

    if (type == "签到") {
        if (signInDeadline.isValid() && now > signInDeadline) {
            int minsLate = signInDeadline.secsTo(now) / 60;
            remark = QString("迟到 %1 分钟").arg(minsLate);
            int ret = QMessageBox::question(this, "确认迟到",
                QString("%1 已迟到 %2 分钟，是否仍然签到？").arg(m_userName).arg(minsLate),
                QMessageBox::Yes | QMessageBox::No);
            if (ret != QMessageBox::Yes) {
                ui->cardStatusLabel->setText("已取消签到");
                return;
            }
        } else {
            remark = "正常";
        }
    } else if (type == "签退") {
        if (signOffStart.isValid() && now < signOffStart) {
            int minsEarly = now.secsTo(signOffStart) / 60;
            int ret = QMessageBox::question(this, "确认早退",
                QString("%1 还未到签退时间（提前 %2 分钟），是否仍然签退？").arg(m_userName).arg(minsEarly),
                QMessageBox::Yes | QMessageBox::No);
            if (ret != QMessageBox::Yes) {
                ui->cardStatusLabel->setText("已取消签退");
                return;
            }
            remark = QString("早退 %1 分钟").arg(minsEarly);
        } else {
            remark = "正常";
        }
    }

    if (db->addSignRecord(cardId, m_userName, type, remark)) {
        QString timeStr = QTime::currentTime().toString("hh:mm:ss");
        QString msg = QString("✅ %1成功！\n时间：%2\n备注：%3").arg(type).arg(timeStr).arg(remark);
        ui->cardStatusLabel->setText(msg);
        QMessageBox::information(this, "成功", msg);
        loadData();
    } else {
        QString err = db->lastError();
        if (err.contains("UNIQUE", Qt::CaseInsensitive)) {
            ui->cardStatusLabel->setText(QString("❌ 今日已%s，请勿重复").arg(type));
            QMessageBox::warning(this, "重复操作", QString("今日已完成%s，请勿重复操作").arg(type));
        } else {
            ui->cardStatusLabel->setText("❌ 记录失败：" + err);
            QMessageBox::critical(this, "失败", "记录失败：" + err);
        }
    }
}

void userAttendanceWidget::on_faceAttendanceBtn_clicked()
{
    if (m_userName.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先登录");
        return;
    }

    if (!m_faceDialog) {
        m_faceDialog = new FaceAttendanceDialog(this);
        connect(m_faceDialog, &FaceAttendanceDialog::attendanceSuccess,
                this, &userAttendanceWidget::onFaceAttendanceSuccess);
    }
    m_faceDialog->exec();
}

void userAttendanceWidget::onFaceAttendanceSuccess(const QString &cardNumber, const QString &userName,
                                                    const QString &time, const QString &type, const QString &remark)
{
    if (userName != m_userName) {
        QMessageBox::warning(this, "用户不符",
            QString("当前登录用户是：%1\n人脸识别结果为：%2").arg(m_userName).arg(userName));
        return;
    }
    Q_UNUSED(cardNumber);
    Q_UNUSED(time);
    Q_UNUSED(type);
    Q_UNUSED(remark);
    loadData();
    ui->cardStatusLabel->setText(QString("✅ 人脸识别考勤成功"));
}

void userAttendanceWidget::loadData()
{
    if (m_userName.isEmpty()) return;

        QList<QMap<QString, QString>> records = MySql::getMySql()->getUserSignRecords(m_userName);
        ui->recordTable->setRowCount(0);
        for (const auto &rec : records) {
            int row = ui->recordTable->rowCount();
            ui->recordTable->insertRow(row);
            ui->recordTable->setItem(row, 0, new QTableWidgetItem(rec["date"]));
            ui->recordTable->setItem(row, 1, new QTableWidgetItem(rec["time"]));
            ui->recordTable->setItem(row, 2, new QTableWidgetItem(rec["type"]));
            QString remark = rec.value("remark");
            QTableWidgetItem *rk = new QTableWidgetItem(remark);
            if (remark.startsWith("迟到") || remark.startsWith("早退")) {
                rk->setForeground(QColor("#c53030"));
            }
            ui->recordTable->setItem(row, 3, rk);
        }
        ui->recordTable->resizeColumnsToContents();

        updateStatus();
}

void userAttendanceWidget::updateStatus()
{
    if (m_userName.isEmpty()) return;

       QString today = QDate::currentDate().toString("yyyy-MM-dd");
       QString sql = "SELECT sign_time, type, COALESCE(remark,'') FROM sign_in_records WHERE name = :name AND sign_date = :date ORDER BY sign_time DESC LIMIT 1";
       QSqlQuery query;
       query.prepare(sql);
       query.bindValue(":name", m_userName);
       query.bindValue(":date", today);
       if (query.exec() && query.next()) {
           QString time = query.value(0).toString();
           QString type = query.value(1).toString();
           QString remark = query.value(2).toString();
           QString statusText = QString("今日状态: ✅ %1 (%2)").arg(type).arg(time);
           if (remark.startsWith("迟到") || remark.startsWith("早退")) {
               statusText += QString("  ⚠️ %1").arg(remark);
           }
           ui->statusLabel->setText(statusText);
       } else {
           ui->statusLabel->setText("今日状态: ❌ 未签到");
       }
}
