#include "dailysummarydialog.h"
#include <QHeaderView>
#include <QDate>
#include <QMessageBox>

DailySummaryDialog::DailySummaryDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("终端日结汇总单");
    setMinimumSize(640, 460);
    resize(720, 520);
    setupUI();
    loadTerminals();
    showSummary();
}

void DailySummaryDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(28, 24, 28, 24);

    // 条件区
    QGroupBox *condBox = new QGroupBox("查询条件", this);
    QHBoxLayout *condLayout = new QHBoxLayout(condBox);
    condLayout->setSpacing(14);

    QLabel *tLabel = new QLabel("终端：", this);
    m_terminalCombo = new QComboBox(this);
    m_terminalCombo->setMinimumHeight(34);

    QLabel *dLabel = new QLabel("日期：", this);
    m_dateEdit = new QDateEdit(QDate::currentDate(), this);
    m_dateEdit->setCalendarPopup(true);
    m_dateEdit->setDisplayFormat("yyyy-MM-dd");
    m_dateEdit->setMinimumHeight(34);

    m_queryBtn = new QPushButton("查询该终端日结", this);
    m_queryBtn->setMinimumHeight(38);

    m_allBtn = new QPushButton("一键查看全部终端当日汇总", this);
    m_allBtn->setMinimumHeight(38);

    condLayout->addWidget(tLabel);
    condLayout->addWidget(m_terminalCombo, 1);
    condLayout->addSpacing(12);
    condLayout->addWidget(dLabel);
    condLayout->addWidget(m_dateEdit);
    condLayout->addSpacing(12);
    condLayout->addWidget(m_queryBtn);
    condLayout->addWidget(m_allBtn);

    mainLayout->addWidget(condBox);

    // 单终端汇总卡片
    m_singleBox = new QGroupBox("终端日结单", this);
    QGridLayout *grid = new QGridLayout(m_singleBox);
    grid->setSpacing(18);
    grid->setContentsMargins(28, 24, 28, 24);

    QLabel *nTip = new QLabel("终端名称：", this);
    m_terminalNameLabel = new QLabel("-", this);
    QFont bold;
    bold.setBold(true);
    bold.setPointSize(bold.pointSize() + 4);
    m_terminalNameLabel->setFont(bold);

    QLabel *dTip = new QLabel("日结日期：", this);
    m_dateLabel = new QLabel("-", this);

    QLabel *cTip = new QLabel("总笔数：", this);
    m_countLabel = new QLabel("0", this);
    QFont big;
    big.setBold(true);
    big.setPointSize(big.pointSize() + 8);
    m_countLabel->setFont(big);

    QLabel *aTip = new QLabel("总金额：", this);
    m_amountLabel = new QLabel("¥ 0.00", this);
    m_amountLabel->setFont(big);

    grid->addWidget(nTip, 0, 0);
    grid->addWidget(m_terminalNameLabel, 0, 1);
    grid->addWidget(dTip, 0, 2);
    grid->addWidget(m_dateLabel, 0, 3);
    grid->addWidget(cTip, 1, 0);
    grid->addWidget(m_countLabel, 1, 1);
    grid->addWidget(aTip, 1, 2);
    grid->addWidget(m_amountLabel, 1, 3);
    grid->setColumnStretch(1, 1);
    grid->setColumnStretch(3, 1);

    mainLayout->addWidget(m_singleBox);

    // 多终端汇总表格
    m_tableBox = new QGroupBox("当日全部终端汇总一览", this);
    QVBoxLayout *tbLayout = new QVBoxLayout(m_tableBox);
    m_tableWidget = new QTableWidget(this);
    m_tableWidget->setColumnCount(4);
    m_tableWidget->setHorizontalHeaderLabels(QStringList() << "终端名称" << "状态" << "总笔数" << "总金额（元）");
    m_tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableWidget->verticalHeader()->setVisible(false);
    m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableWidget->setAlternatingRowColors(true);
    tbLayout->addWidget(m_tableWidget);
    m_tableBox->setVisible(false);
    mainLayout->addWidget(m_tableBox, 1);

    // 关闭按钮
    QHBoxLayout *btnRow = new QHBoxLayout();
    btnRow->addStretch();
    m_closeBtn = new QPushButton("关闭", this);
    m_closeBtn->setMinimumSize(120, 40);
    btnRow->addWidget(m_closeBtn);
    mainLayout->addLayout(btnRow);

    connect(m_queryBtn, &QPushButton::clicked, this, [this](){
        m_singleBox->setVisible(true);
        m_tableBox->setVisible(false);
        showSummary();
    });
    connect(m_allBtn, &QPushButton::clicked, this, [this](){
        m_singleBox->setVisible(false);
        m_tableBox->setVisible(true);
        showAllSummary(m_dateEdit->date().toString("yyyy-MM-dd"));
    });
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::accept);
}

void DailySummaryDialog::loadTerminals()
{
    m_terminalCombo->clear();
    MySql *db = MySql::getMySql();
    QList<TerminalInfo> list = db->getAllTerminals();
    if (list.isEmpty()) {
        m_terminalCombo->addItem("（暂无终端）", 0);
        return;
    }
    for (const TerminalInfo &info : list) {
        QString txt = info.name + (info.status != "正常" ? QString("（%1）").arg(info.status) : "");
        m_terminalCombo->addItem(txt, info.id);
    }
    m_terminalCombo->setCurrentIndex(0);
}

void DailySummaryDialog::showSummary()
{
    int tid = m_terminalCombo->currentData().toInt();
    QString date = m_dateEdit->date().toString("yyyy-MM-dd");

    if (tid <= 0) {
        m_terminalNameLabel->setText("未选择终端");
        m_dateLabel->setText(date);
        m_countLabel->setText("0");
        m_amountLabel->setText("¥ 0.00");
        QMessageBox::warning(this, "提示", "没有可用的消费终端");
        return;
    }

    MySql *db = MySql::getMySql();
    DailySummary sum = db->getDailySummary(tid, date);

    m_terminalNameLabel->setText(sum.terminalName);
    m_dateLabel->setText(sum.date);
    m_countLabel->setText(QString::number(sum.totalCount));
    m_amountLabel->setText(QString("¥ %1").arg(sum.totalAmount, 0, 'f', 2));
}

void DailySummaryDialog::showAllSummary(const QString &date)
{
    MySql *db = MySql::getMySql();
    QList<TerminalInfo> list = db->getAllTerminals();
    m_tableWidget->setRowCount(0);

    int totalCountAll = 0;
    double totalAmountAll = 0.0;

    for (const TerminalInfo &info : list) {
        DailySummary sum = db->getDailySummary(info.id, date);
        int row = m_tableWidget->rowCount();
        m_tableWidget->insertRow(row);

        QTableWidgetItem *nameItem = new QTableWidgetItem(sum.terminalName);
        QTableWidgetItem *statusItem = new QTableWidgetItem(info.status);
        QTableWidgetItem *countItem = new QTableWidgetItem(QString::number(sum.totalCount));
        QTableWidgetItem *amountItem = new QTableWidgetItem(QString::number(sum.totalAmount, 'f', 2));

        countItem->setTextAlignment(Qt::AlignCenter);
        amountItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        if (info.status == "停用") {
            QFont italic;
            italic.setItalic(true);
            nameItem->setFont(italic);
        }

        m_tableWidget->setItem(row, 0, nameItem);
        m_tableWidget->setItem(row, 1, statusItem);
        m_tableWidget->setItem(row, 2, countItem);
        m_tableWidget->setItem(row, 3, amountItem);

        totalCountAll += sum.totalCount;
        totalAmountAll += sum.totalAmount;
    }

    // 合计行
    int row = m_tableWidget->rowCount();
    m_tableWidget->insertRow(row);
    QTableWidgetItem *totalName = new QTableWidgetItem("合计");
    QFont boldFont;
    boldFont.setBold(true);
    totalName->setFont(boldFont);
    QTableWidgetItem *totalStatus = new QTableWidgetItem("-");
    QTableWidgetItem *totalCount = new QTableWidgetItem(QString::number(totalCountAll));
    totalCount->setFont(boldFont);
    totalCount->setTextAlignment(Qt::AlignCenter);
    QTableWidgetItem *totalAmt = new QTableWidgetItem(QString::number(totalAmountAll, 'f', 2));
    totalAmt->setFont(boldFont);
    totalAmt->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_tableWidget->setItem(row, 0, totalName);
    m_tableWidget->setItem(row, 1, totalStatus);
    m_tableWidget->setItem(row, 2, totalCount);
    m_tableWidget->setItem(row, 3, totalAmt);
}
