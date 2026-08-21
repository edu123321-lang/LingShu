#include "gmcardwidget.h"
#include "ui_gmcardwidget.h"
#include <QMessageBox>
#include <QInputDialog>
#include <QDebug>
#include "consumedialog.h"
#include "dailysummarydialog.h"

GmCardWidget::GmCardWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GmCardWidget),
    m_currentCardId(-1)
    {
        ui->setupUi(this);
        // 设置表格列
        ui->userTable->setColumnCount(4);
        ui->userTable->setHorizontalHeaderLabels({"姓名", "卡号", "余额", "状态"});
        ui->userTable->horizontalHeader()->setStretchLastSection(true);
        ui->userTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        ui->userTable->setSelectionBehavior(QAbstractItemView::SelectRows);

        ui->transTable->setColumnCount(4);
        ui->transTable->setHorizontalHeaderLabels({"时间", "类型", "金额", "终端/来源"});
        ui->transTable->horizontalHeader()->setStretchLastSection(true);
        ui->transTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

        // 连接信号槽
        connect(ui->userTable, &QTableWidget::itemClicked, this, &GmCardWidget::on_userTable_itemClicked);
        // 初始加载所有数据
        loadCardData();
    }

GmCardWidget::~GmCardWidget()
{
    delete ui;
}

//搜索按钮
void GmCardWidget::on_searchBtn_clicked()
{
    QString keyword = ui->searchEdit->text().trimmed();
    loadCardData(keyword);
}

//刷新按钮
void GmCardWidget::on_refreshBtn_clicked()
{
    ui->searchEdit->clear();
    loadCardData();
    if (m_currentCardId != -1)
    {
        loadTransaction(m_currentCardId);
        updateBalanceStatus(m_currentCardId);
    }
}

//充值按钮
void GmCardWidget::on_topUpBtn_clicked()
{
    if (m_currentCardId == -1) {
            QMessageBox::warning(this, "提示", "请先选择一位用户");
            return;
        }

        // 检查卡片状态
        QMap<QString, QString> cardInfo = MySql::getMySql()->getCardInfoByCardId(m_currentCardId);
        if (cardInfo["status"] == "挂失") {
            QMessageBox::warning(this, "提示", "该卡已挂失，无法充值");
            return;
        }

        bool ok;
        double amount = QInputDialog::getDouble(this, "充值", "请输入充值金额：", 0, 0, 100000, 2, &ok);
        if (!ok) return;
        if (amount <= 0) {
            QMessageBox::warning(this, "错误", "金额必须大于0");
            return;
        }

        MySql *db = MySql::getMySql();
        if (db->topUp(m_currentCardId, amount)) {
            QMessageBox::information(this, "成功", "充值成功");
            refreshData();
        } else {
            QString errorMsg = db->lastError();
            if (errorMsg.isEmpty())
                errorMsg = "充值失败，未知错误";
            QMessageBox::critical(this, "错误", errorMsg);
        }
}

//扣费按钮
void GmCardWidget::on_consumeBtn_clicked()
{
    if (m_currentCardId == -1) {
            QMessageBox::warning(this, "提示", "请先选择一位用户");
            return;
        }

        // 检查卡片状态
        QMap<QString, QString> cardInfo = MySql::getMySql()->getCardInfoByCardId(m_currentCardId);
        if (cardInfo["status"] == "挂失") {
            QMessageBox::warning(this, "提示", "该卡已挂失，无法扣费");
            return;
        }

        // 获取当前持卡人姓名
        MySql *db = MySql::getMySql();
        QString userName = db->getUserNameByCardId(m_currentCardId);
        double balance = cardInfo["balance"].toDouble();

        // 弹出选择终端+金额对话框（不再手动输入文字）
        ConsumeDialog dialog(userName, balance, this);
        if (dialog.exec() != QDialog::Accepted) return;

        int terminalId = dialog.selectedTerminalId();
        double amount = dialog.amount();
        QString terminalName = dialog.selectedTerminalName();

        if (terminalId <= 0) {
            QMessageBox::warning(this, "提示", "请选择有效的消费终端，可先在「终端管理」中新增");
            return;
        }
        if (amount <= 0) {
            QMessageBox::warning(this, "错误", "金额必须大于0");
            return;
        }

        if (db->consume(m_currentCardId, amount, terminalId)) {
            QMessageBox::information(this, "成功",
                QString("扣费成功\n\n终端：%1\n金额：¥%2\n当前余额：¥%3")
                    .arg(terminalName)
                    .arg(amount, 0, 'f', 2)
                    .arg(db->getCardInfoByCardId(m_currentCardId)["balance"].toDouble(), 0, 'f', 2));
            refreshData();
        } else {
            QMessageBox::critical(this, "错误", db->lastError().isEmpty() ? QString("扣费失败，可能余额不足") : db->lastError());
        }
}

void GmCardWidget::on_dailySummaryBtn_clicked()
{
    DailySummaryDialog dialog(this);
    dialog.exec();
}

//挂失
void GmCardWidget::on_reportLossBtn_clicked()
{
    if (m_currentCardId == -1) {
            QMessageBox::warning(this, "提示", "请先选择一位用户");
            return;
        }
        int ret = QMessageBox::question(this, "挂失", "确定要挂失该卡吗？", QMessageBox::Yes | QMessageBox::No);
        if (ret == QMessageBox::Yes) {
            if (MySql::getMySql()->reportLoss(m_currentCardId)) {
                QMessageBox::information(this, "成功", "挂失成功");
                refreshData();
            } else {
                QMessageBox::critical(this, "错误", "挂失失败");
            }
        }
}

//补办
void GmCardWidget::on_reissueCard_clicked()
{
    if (m_currentCardId == -1) {
            QMessageBox::warning(this, "提示", "请先选择一位用户");
            return;
        }
        bool ok;
        QString newCard = QInputDialog::getText(this, "补办", "请输入新卡号：", QLineEdit::Normal, "", &ok);
        if (!ok || newCard.isEmpty()) return;
        // 格式化卡号（与注册一致）
        newCard = MySql::getMySql()->normalizedCardNumber(newCard);
        if (MySql::getMySql()->reissueCard(m_currentCardId, newCard)) {
            QMessageBox::information(this, "成功", "补办成功，新卡号已更新");
            refreshData();
        } else {
            QMessageBox::critical(this, "错误", "补办失败，可能卡号已被使用或数据库错误");
        }
}

//点击表格项
void GmCardWidget::on_userTable_itemClicked(QTableWidgetItem *item)
{
    int row = item->row();
    int cardId = ui->userTable->item(row, 0)->data(Qt::UserRole).toInt();
    if (cardId == -1) return;
    m_currentCardId = cardId;
    loadTransaction(cardId);
    updateBalanceStatus(cardId);
}

//加载卡片列表
void GmCardWidget::loadCardData(const QString &keyword)
{
    qDebug() << "loadCardData 被调用，keyword=" << keyword;   // 新增
    ui->userTable->setRowCount(0);
    QList<QMap<QString, QString>> cards;
    if (keyword.isEmpty()) {
        cards = MySql::getMySql()->getAllCardInfo();
    } else {
        cards = MySql::getMySql()->searchCards(keyword);
    }

    for (const auto &map : cards)
    {
        int row = ui->userTable->rowCount();
        ui->userTable->insertRow(row);
        ui->userTable->setItem(row, 0, new QTableWidgetItem(map["name"]));
        ui->userTable->setItem(row, 1, new QTableWidgetItem(map["card_number"]));
        ui->userTable->setItem(row, 2, new QTableWidgetItem(map["balance"]));
        ui->userTable->setItem(row, 3, new QTableWidgetItem(map["status"]));
        // 隐藏存储 card_id
        ui->userTable->item(row, 0)->setData(Qt::UserRole, map["card_id"].toInt());
    }
    ui->userTable->resizeColumnsToContents();
    qDebug() << "查询到卡片数量：" << cards.size();
}

//加载交易记录
void GmCardWidget::loadTransaction(int cardId)
{
    ui->transTable->setRowCount(0);
    auto records = MySql::getMySql()->getCardTransactions(cardId);
   for (const auto &rec : records)
   {
        int row = ui->transTable->rowCount();
        ui->transTable->insertRow(row);
        ui->transTable->setItem(row, 0, new QTableWidgetItem(rec["time"]));
        ui->transTable->setItem(row, 1, new QTableWidgetItem(rec["type"]));
        QTableWidgetItem *amtItem = new QTableWidgetItem(rec["amount"]);
        amtItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        ui->transTable->setItem(row, 2, amtItem);
        QTableWidgetItem *termItem = new QTableWidgetItem(rec["terminal"]);
        ui->transTable->setItem(row, 3, termItem);
   }
    ui->transTable->resizeColumnsToContents();
}

//更新余额和状态显示
void GmCardWidget::updateBalanceStatus(int cardId)
{
    if (cardId == -1)
    {
        ui->balanceLabel->setText("请选择用户");
        return;
    }
    auto cards = MySql::getMySql()->getAllCardInfo(); // 也可单独查询，但简单起见用这个
    for (const auto &map : cards)
    {
        if (map["card_id"].toInt() == cardId)
        {
            QString text = QString("余额：%1\n状态：%2").arg(map["balance"]).arg(map["status"]);
            ui->balanceLabel->setText(text);
            return;
        }
    }
    ui->balanceLabel->setText("未找到该卡");
}

void GmCardWidget::showMessage(const QString &title, const QString &text, bool isError)
{
    if (isError) {
            QMessageBox::critical(this, title, text);
        } else {
            QMessageBox::information(this, title, text);
        }
}

void GmCardWidget::refreshData()
{
    loadCardData(ui->searchEdit->text().trimmed());
    if (m_currentCardId != -1)
    {
        loadTransaction(m_currentCardId);
        updateBalanceStatus(m_currentCardId);
    }
}
