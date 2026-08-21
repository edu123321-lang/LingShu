#include "consumedialog.h"
#include <QMessageBox>
#include <QIntValidator>

ConsumeDialog::ConsumeDialog(const QString &userName, double currentBalance, QWidget *parent)
    : QDialog(parent), m_userName(userName), m_balance(currentBalance)
{
    setWindowTitle("凌枢云台 · 消费扣费");
    setMinimumWidth(420);
    setupUI();
    loadTerminals();
}

void ConsumeDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(28, 24, 28, 24);

    QGroupBox *infoBox = new QGroupBox("消费信息", this);
    QVBoxLayout *infoLayout = new QVBoxLayout(infoBox);
    infoLayout->setSpacing(12);

    m_infoLabel = new QLabel(QString("持卡人：<b style='color:#2c3e50;'>%1</b>").arg(m_userName), this);
    m_balanceLabel = new QLabel(QString("当前余额：<b style='color:#27ae60;font-size:12pt;'>¥%1</b>")
                                .arg(m_balance, 0, 'f', 2), this);
    infoLayout->addWidget(m_infoLabel);
    infoLayout->addWidget(m_balanceLabel);
    mainLayout->addWidget(infoBox);

    QGroupBox *formBox = new QGroupBox("请选择消费终端和金额", this);
    QVBoxLayout *formLayout = new QVBoxLayout(formBox);
    formLayout->setSpacing(14);

    // 终端选择
    QHBoxLayout *tRow = new QHBoxLayout();
    QLabel *tLabel = new QLabel("消费终端：", this);
    tLabel->setMinimumWidth(80);
    m_terminalCombo = new QComboBox(this);
    m_terminalCombo->setMinimumHeight(36);
    tRow->addWidget(tLabel);
    tRow->addWidget(m_terminalCombo, 1);
    formLayout->addLayout(tRow);

    // 金额输入
    QHBoxLayout *aRow = new QHBoxLayout();
    QLabel *aLabel = new QLabel("消费金额：", this);
    aLabel->setMinimumWidth(80);
    m_amountSpin = new QDoubleSpinBox(this);
    m_amountSpin->setMinimum(0.01);
    m_amountSpin->setMaximum(m_balance > 0 ? m_balance : 10000.00);
    m_amountSpin->setDecimals(2);
    m_amountSpin->setSuffix(" 元");
    m_amountSpin->setValue(10.00);
    m_amountSpin->setMinimumHeight(36);
    aRow->addWidget(aLabel);
    aRow->addWidget(m_amountSpin, 1);
    formLayout->addLayout(aRow);

    mainLayout->addWidget(formBox);

    // 按钮
    QHBoxLayout *btnRow = new QHBoxLayout();
    btnRow->addStretch();
    m_cancelBtn = new QPushButton("取消", this);
    m_cancelBtn->setMinimumSize(110, 42);
    m_okBtn = new QPushButton("确认扣费", this);
    m_okBtn->setMinimumSize(140, 42);
    m_okBtn->setDefault(true);
    btnRow->addWidget(m_cancelBtn);
    btnRow->addWidget(m_okBtn);
    mainLayout->addLayout(btnRow);

    connect(m_okBtn, &QPushButton::clicked, this, [this](){
        if (m_terminalCombo->count() <= 0) {
            QMessageBox::warning(this, "提示", "没有可用的消费终端，请先在「终端管理」中添加");
            return;
        }
        if (m_amountSpin->value() <= 0) {
            QMessageBox::warning(this, "提示", "消费金额必须大于0");
            return;
        }
        if (m_amountSpin->value() > m_balance + 0.000001) {
            QMessageBox::warning(this, "提示", "消费金额不能超过当前余额");
            return;
        }
        accept();
    });
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void ConsumeDialog::loadTerminals()
{
    m_terminalCombo->clear();
    MySql *db = MySql::getMySql();
    QList<TerminalInfo> list = db->getActiveTerminals();
    if (list.isEmpty()) {
        m_terminalCombo->addItem("（无可用终端，请先新增）", 0);
        return;
    }
    for (const TerminalInfo &info : list) {
        m_terminalCombo->addItem(info.name, info.id);
    }
    if (m_terminalCombo->count() > 0) {
        m_terminalCombo->setCurrentIndex(0);
    }
}

int ConsumeDialog::selectedTerminalId() const
{
    return m_terminalCombo->currentData().toInt();
}

QString ConsumeDialog::selectedTerminalName() const
{
    return m_terminalCombo->currentText();
}

double ConsumeDialog::amount() const
{
    return m_amountSpin->value();
}
