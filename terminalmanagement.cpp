#include "terminalmanagement.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QPushButton>
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>
#include <QLabel>
#include <QGroupBox>
#include <QTableWidgetItem>
#include <QBrush>
#include <QColor>
#include <QDateTime>

TerminalManagement::TerminalManagement(QWidget *parent) : QWidget(parent)
{
    setupUI();
    refresh();
}

void TerminalManagement::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(40, 40, 40, 40);
    mainLayout->setSpacing(20);

    // 顶部标题
    QLabel *title = new QLabel("终端管理", this);
    QFont titleFont;
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 6);
    title->setFont(titleFont);
    QLabel *sub = new QLabel("管理所有消费终端（食堂、超市、打印店等），支持新增、重命名、启用/停用", this);
    mainLayout->addWidget(title);
    mainLayout->addWidget(sub);

    // 操作栏
    QGroupBox *actionBox = new QGroupBox("操作", this);
    QHBoxLayout *actionLayout = new QHBoxLayout(actionBox);
    actionLayout->setSpacing(12);
    actionLayout->setContentsMargins(18, 16, 18, 16);

    m_addBtn = new QPushButton("新增终端", this);
    m_addBtn->setMinimumHeight(42);
    m_addBtn->setMinimumWidth(110);

    m_renameBtn = new QPushButton("修改名称", this);
    m_renameBtn->setMinimumHeight(42);
    m_renameBtn->setMinimumWidth(110);
    m_renameBtn->setEnabled(false);

    m_toggleBtn = new QPushButton("启用/停用", this);
    m_toggleBtn->setMinimumHeight(42);
    m_toggleBtn->setMinimumWidth(110);
    m_toggleBtn->setEnabled(false);

    m_refreshBtn = new QPushButton("刷新列表", this);
    m_refreshBtn->setMinimumHeight(42);
    m_refreshBtn->setMinimumWidth(110);

    actionLayout->addWidget(m_addBtn);
    actionLayout->addWidget(m_renameBtn);
    actionLayout->addWidget(m_toggleBtn);
    actionLayout->addSpacing(12);
    actionLayout->addStretch();
    actionLayout->addWidget(m_refreshBtn);

    mainLayout->addWidget(actionBox);

    // 提示
    m_hintLabel = new QLabel("新增终端示例：二食堂新窗口、澡堂、校车充值点等", this);
    mainLayout->addWidget(m_hintLabel);

    // 列表
    m_tableWidget = new QTableWidget(this);
    m_tableWidget->setColumnCount(5);
    m_tableWidget->setHorizontalHeaderLabels(QStringList() << "ID" << "终端名称" << "状态" << "创建时间" << "今日笔数预览");
    m_tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_tableWidget->verticalHeader()->setVisible(false);
    m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableWidget->setAlternatingRowColors(true);
    mainLayout->addWidget(m_tableWidget, 1);

    connect(m_addBtn, &QPushButton::clicked, this, &TerminalManagement::onAdd);
    connect(m_renameBtn, &QPushButton::clicked, this, &TerminalManagement::onRename);
    connect(m_toggleBtn, &QPushButton::clicked, this, &TerminalManagement::onToggleStatus);
    connect(m_refreshBtn, &QPushButton::clicked, this, &TerminalManagement::refresh);
    connect(m_tableWidget, &QTableWidget::itemSelectionChanged, this, &TerminalManagement::onSelectionChanged);
}

void TerminalManagement::refresh()
{
    m_tableWidget->setRowCount(0);
    MySql *db = MySql::getMySql();
    QList<TerminalInfo> list = db->getAllTerminals();
    QString today = QDate::currentDate().toString("yyyy-MM-dd");

    for (const TerminalInfo &info : list) {
        int row = m_tableWidget->rowCount();
        m_tableWidget->insertRow(row);

        DailySummary sum = db->getDailySummary(info.id, today);

        QTableWidgetItem *idItem = new QTableWidgetItem(QString::number(info.id));
        idItem->setData(Qt::UserRole, info.id);
        idItem->setTextAlignment(Qt::AlignCenter);
        QTableWidgetItem *nameItem = new QTableWidgetItem(info.name);
        QTableWidgetItem *statusItem = new QTableWidgetItem(info.status);
        statusItem->setTextAlignment(Qt::AlignCenter);
        if (info.status == "停用") {
            QFont italic;
            italic.setItalic(true);
            nameItem->setFont(italic);
            statusItem->setFont(italic);
        }
        QTableWidgetItem *timeItem = new QTableWidgetItem(info.createTime);
        QTableWidgetItem *previewItem = new QTableWidgetItem(QString::number(sum.totalCount) + " 笔 / ¥" + QString::number(sum.totalAmount, 'f', 2));
        previewItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        m_tableWidget->setItem(row, 0, idItem);
        m_tableWidget->setItem(row, 1, nameItem);
        m_tableWidget->setItem(row, 2, statusItem);
        m_tableWidget->setItem(row, 3, timeItem);
        m_tableWidget->setItem(row, 4, previewItem);
    }
    onSelectionChanged();
}

int TerminalManagement::selectedTerminalId()
{
    int row = m_tableWidget->currentRow();
    if (row < 0) return -1;
    QTableWidgetItem *idItem = m_tableWidget->item(row, 0);
    if (!idItem) return -1;
    return idItem->data(Qt::UserRole).toInt();
}

void TerminalManagement::onSelectionChanged()
{
    int id = selectedTerminalId();
    bool enable = (id > 0);
    m_renameBtn->setEnabled(enable);
    m_toggleBtn->setEnabled(enable);
    if (enable) {
        int row = m_tableWidget->currentRow();
        QString status = m_tableWidget->item(row, 2)->text();
        if (status == "停用") {
            m_toggleBtn->setText("启用终端");
        } else {
            m_toggleBtn->setText("停用终端");
        }
    } else {
        m_toggleBtn->setText("启用/停用");
    }
}

void TerminalManagement::onAdd()
{
    bool ok = false;
    QString name = QInputDialog::getText(this, "新增终端", "请输入终端名称（如：二食堂新窗口）：",
                                         QLineEdit::Normal, "", &ok);
    if (!ok) return;
    if (name.trimmed().isEmpty()) {
        QMessageBox::warning(this, "提示", "终端名称不能为空");
        return;
    }
    MySql *db = MySql::getMySql();
    if (db->addTerminal(name.trimmed())) {
        QMessageBox::information(this, "成功", QString("已新增终端：%1").arg(name.trimmed()));
        refresh();
    } else {
        QMessageBox::critical(this, "失败", db->lastError());
    }
}

void TerminalManagement::onRename()
{
    int id = selectedTerminalId();
    if (id <= 0) {
        QMessageBox::warning(this, "提示", "请先选中一行终端记录");
        return;
    }
    int row = m_tableWidget->currentRow();
    QString oldName = m_tableWidget->item(row, 1)->text();
    bool ok = false;
    QString newName = QInputDialog::getText(this, "修改终端名称", "请输入新的终端名称：",
                                            QLineEdit::Normal, oldName, &ok);
    if (!ok) return;
    if (newName.trimmed().isEmpty()) {
        QMessageBox::warning(this, "提示", "终端名称不能为空");
        return;
    }
    MySql *db = MySql::getMySql();
    if (db->renameTerminal(id, newName.trimmed())) {
        QMessageBox::information(this, "成功", QString("已重命名：%1 → %2").arg(oldName, newName.trimmed()));
        refresh();
    } else {
        QMessageBox::critical(this, "失败", db->lastError());
    }
}

void TerminalManagement::onToggleStatus()
{
    int id = selectedTerminalId();
    if (id <= 0) {
        QMessageBox::warning(this, "提示", "请先选中一行终端记录");
        return;
    }
    int row = m_tableWidget->currentRow();
    QString curStatus = m_tableWidget->item(row, 2)->text();
    QString targetStatus = (curStatus == "停用") ? "正常" : "停用";
    QString terminalName = m_tableWidget->item(row, 1)->text();

    QString warnText;
    if (targetStatus == "停用") {
        warnText = QString("确定停用终端「%1」吗？\n\n停用后，该终端将不会出现在消费下拉列表中，\n但已有的历史消费记录不会被删除。").arg(terminalName);
    } else {
        warnText = QString("确定重新启用终端「%1」吗？").arg(terminalName);
    }
    auto ret = QMessageBox::question(this, targetStatus == "停用" ? "停用确认" : "启用确认", warnText,
                                     QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (ret != QMessageBox::Yes) return;

    MySql *db = MySql::getMySql();
    if (db->setTerminalStatus(id, targetStatus)) {
        QMessageBox::information(this, "成功", QString("终端「%1」已%2").arg(terminalName, targetStatus));
        refresh();
    } else {
        QMessageBox::critical(this, "失败", db->lastError());
    }
}
