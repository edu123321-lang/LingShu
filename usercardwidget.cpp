#include "usercardwidget.h"
#include "ui_usercardwidget.h"
#include <QDebug>
#include <QMessageBox>
#include <QSqlQuery>

userCardWidget::userCardWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::userCardWidget),
    m_cardId(-1)
{
    ui->setupUi(this);
    // 设置表格列
    ui->transactionTable->setColumnCount(3);
    ui->transactionTable->setHorizontalHeaderLabels({"时间", "类型", "金额"});
    ui->transactionTable->horizontalHeader()->setStretchLastSection(true);
    ui->transactionTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // 连接刷新按钮
    connect(ui->refreshBtn, &QPushButton::clicked, this, &userCardWidget::on_refreshBtn_clicked);
}




userCardWidget::~userCardWidget()
{
    delete ui;
}

void userCardWidget::setUserName(const QString &name)
{
    m_userName = name;
    loadData();
}

void userCardWidget::loadData()
{
    if (m_userName.isEmpty()) {
            ui->statusLabel->setText("未登录");
            return;
        }

        MySql *db = MySql::getMySql();

        // 1. 获取 card_id
        m_cardId = db->getCardIdByUserName(m_userName);
        if (m_cardId == -1)
        {
            ui->userNameLabel->setText("用户: " + m_userName);
            ui->cardNumberLabel->setText("卡号: 未开通");
            ui->balanceLabel->setText("余额: --");
            ui->statusLabel->setText("状态: 未开通");
            ui->transactionTable->setRowCount(0);
            return;
        }

        // 2. 获取用户卡号和姓名（可从 user 表直接取）
        QSqlQuery query;
        query.prepare("SELECT name, card FROM user WHERE name = :name");
        query.bindValue(":name", m_userName);
        if (query.exec() && query.next())
        {
            ui->userNameLabel->setText("用户: " + query.value(0).toString());
            ui->cardNumberLabel->setText("卡号: " + query.value(1).toString());
        }

        // 3. 更新余额和状态
        updateCardInfo();

            // 4. 加载交易记录
        loadTransactions();

}

void userCardWidget::updateCardInfo()
{
    if (m_cardId == -1) return;

        MySql *db = MySql::getMySql();
        QMap<QString, QString> info = db->getCardInfoByCardId(m_cardId);

        if (info.isEmpty()) {
            ui->balanceLabel->setText("余额: 查询失败");
            ui->statusLabel->setText("状态: 未知");
            return;
        }

        ui->balanceLabel->setText("余额: ￥" + info["balance"]);

        QString status = info["status"];
        ui->statusLabel->setText("状态: " + status);
        // 设置状态颜色（正常=绿色，挂失=红色）
        if (status == "正常") {
            ui->statusLabel->setStyleSheet("color: green;");
        } else if (status == "挂失") {
            ui->statusLabel->setStyleSheet("color: red;");
        } else {
            ui->statusLabel->setStyleSheet("color: gray;");
        }
}

void userCardWidget::loadTransactions()
{
    ui->transactionTable->setRowCount(0);
       if (m_cardId == -1) return;

       MySql *db = MySql::getMySql();
       QList<QMap<QString, QString>> records = db->getCardTransactions(m_cardId);

       for (const auto &rec : records) {
           int row = ui->transactionTable->rowCount();
           ui->transactionTable->insertRow(row);
           ui->transactionTable->setItem(row, 0, new QTableWidgetItem(rec["time"]));
           ui->transactionTable->setItem(row, 1, new QTableWidgetItem(rec["type"]));

           // 金额显示：充值为正，消费为负，保留两位小数
           double amount = rec["amount"].toDouble();
           QString amountStr = QString::number(amount, 'f', 2);
           if (amount > 0) amountStr = "+" + amountStr;
           ui->transactionTable->setItem(row, 2, new QTableWidgetItem(amountStr));
       }
       ui->transactionTable->resizeColumnsToContents();
}

void userCardWidget::on_refreshBtn_clicked()
{
    loadData();
}
