#include "attendancewidget.h"
#include "ui_attendancewidget.h"
#include "mysql.h"
#include <QMessageBox>
#include <QSqlQuery>
#include <QDateTime>
#include <QTime>
#include <QDate>
#include <QDebug>
#include <QInputDialog>
#include <QComboBox>
#include <QTimeEdit>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>

AttendanceWidget::AttendanceWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AttendanceWidget)
{
    ui->setupUi(this);

    ui->tableWidget->setColumnCount(6);
    ui->tableWidget->setHorizontalHeaderLabels({"卡号", "姓名", "日期", "时间", "类型", "备注"});
    ui->tableWidget->horizontalHeader()->setStretchLastSection(true);
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);

    ui->editRecordBtn->setEnabled(false);
    ui->deleteRecordBtn->setEnabled(false);

    ui->dateEdit->setCalendarPopup(true);
    ui->dateEdit->setDate(QDate::currentDate());

    connect(ui->dateEdit, &QDateEdit::dateChanged, this, &AttendanceWidget::on_dateEdit_dateChanged);
    connect(ui->tableWidget, &QTableWidget::itemSelectionChanged, this, &AttendanceWidget::on_tableWidget_itemSelectionChanged);

    loadTimeRules();
    loadRecordsByDate(QDate::currentDate());
}

AttendanceWidget::~AttendanceWidget()
{
    delete ui;
}

void AttendanceWidget::on_clearTableBtn_clicked()
{
    ui->tableWidget->setRowCount(0);
}

void AttendanceWidget::on_saveTimeRuleBtn_clicked()
{
    QTime signInDeadline = ui->signInTimeEdit->time();
    QTime signOffStart = ui->signOffTimeEdit->time();
    if (signOffStart <= signInDeadline) {
        QMessageBox::warning(this, "提示", "签退起始时间必须晚于签到截止时间");
        return;
    }
    MySql *db = MySql::getMySql();
    QString signInStr = signInDeadline.toString("HH:mm");
    QString signOffStr = signOffStart.toString("HH:mm");
    bool ok = db->setWorkSignInDeadline(signInStr) && db->setWorkSignOffStart(signOffStr);
    if (ok) {
        QMessageBox::information(this, "成功",
            QString("已保存时间规则：\n签到截止 %1\n签退起始 %2").arg(signInStr).arg(signOffStr));
    } else {
        QMessageBox::critical(this, "失败", "保存失败：" + db->lastError());
    }
}

void AttendanceWidget::on_dateEdit_dateChanged(const QDate &date)
{
    loadRecordsByDate(date);
}

void AttendanceWidget::on_refreshBtn_clicked()
{
    loadRecordsByDate(ui->dateEdit->date());
}

void AttendanceWidget::on_tableWidget_itemSelectionChanged()
{
    bool hasSelection = ui->tableWidget->currentRow() >= 0;
    ui->editRecordBtn->setEnabled(hasSelection);
    ui->deleteRecordBtn->setEnabled(hasSelection);
}

int AttendanceWidget::getSelectedRecordId()
{
    int row = ui->tableWidget->currentRow();
    if (row < 0) return -1;
    QTableWidgetItem *item = ui->tableWidget->item(row, 0);
    if (!item) return -1;
    bool ok;
    int id = item->data(Qt::UserRole).toInt(&ok);
    return ok ? id : -1;
}

void AttendanceWidget::on_editRecordBtn_clicked()
{
    int recordId = getSelectedRecordId();
    if (recordId <= 0) {
        QMessageBox::warning(this, "提示", "请先选择一条记录");
        return;
    }

    int row = ui->tableWidget->currentRow();
    QString currentType = ui->tableWidget->item(row, 4)->text();
    QString currentTime = ui->tableWidget->item(row, 3)->text();
    QString currentRemark = ui->tableWidget->item(row, 5)->text();
    QString userName = ui->tableWidget->item(row, 1)->text();

    QDialog dialog(this);
    dialog.setWindowTitle("修改签到记录 - " + userName);
    dialog.setMinimumWidth(350);

    QFormLayout *layout = new QFormLayout(&dialog);

    QLabel *nameLabel = new QLabel(userName, &dialog);
    layout->addRow("姓名：", nameLabel);

    QComboBox *typeCombo = new QComboBox(&dialog);
    typeCombo->addItem("签到");
    typeCombo->addItem("签退");
    typeCombo->setCurrentText(currentType);
    layout->addRow("类型：", typeCombo);

    QTimeEdit *timeEdit = new QTimeEdit(&dialog);
    timeEdit->setDisplayFormat("HH:mm:ss");
    timeEdit->setTime(QTime::fromString(currentTime, "HH:mm:ss"));
    layout->addRow("时间：", timeEdit);

    QLineEdit *remarkEdit = new QLineEdit(currentRemark, &dialog);
    remarkEdit->setPlaceholderText("如：正常、迟到 15 分钟、早退 5 分钟等");
    layout->addRow("备注：", remarkEdit);

    QDialogButtonBox *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
    layout->addRow(buttons);

    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) return;

    QString newType = typeCombo->currentText();
    QString newTime = timeEdit->time().toString("HH:mm:ss");
    QString newRemark = remarkEdit->text().trimmed();
    if (newRemark.isEmpty()) newRemark = "正常";

    MySql *db = MySql::getMySql();
    if (db->updateSignRecord(recordId, newType, newTime, newRemark)) {
        QMessageBox::information(this, "成功", "签到记录修改成功！");
        loadRecordsByDate(ui->dateEdit->date());
    } else {
        QMessageBox::critical(this, "失败", "修改失败：" + db->lastError());
    }
}

void AttendanceWidget::on_deleteRecordBtn_clicked()
{
    int recordId = getSelectedRecordId();
    if (recordId <= 0) {
        QMessageBox::warning(this, "提示", "请先选择一条记录");
        return;
    }

    int row = ui->tableWidget->currentRow();
    QString userName = ui->tableWidget->item(row, 1)->text();
    QString type = ui->tableWidget->item(row, 4)->text();
    QString dateStr = ui->tableWidget->item(row, 2)->text();
    QString timeStr = ui->tableWidget->item(row, 3)->text();

    QString msg = QString("确定要删除这条签到记录吗？\n\n"
                          "姓名：%1\n日期：%2\n时间：%3\n类型：%4")
                      .arg(userName).arg(dateStr).arg(timeStr).arg(type);

    if (QMessageBox::question(this, "确认删除", msg,
                              QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
        return;
    }

    MySql *db = MySql::getMySql();
    if (db->deleteSignRecord(recordId)) {
        QMessageBox::information(this, "成功", "签到记录已删除");
        loadRecordsByDate(ui->dateEdit->date());
    } else {
        QMessageBox::critical(this, "失败", "删除失败：" + db->lastError());
    }
}

void AttendanceWidget::loadTimeRules()
{
    MySql *db = MySql::getMySql();
    QTime signIn = QTime::fromString(db->getWorkSignInDeadline(), "HH:mm");
    QTime signOff = QTime::fromString(db->getWorkSignOffStart(), "HH:mm");
    if (signIn.isValid()) ui->signInTimeEdit->setTime(signIn);
    if (signOff.isValid()) ui->signOffTimeEdit->setTime(signOff);
}

void AttendanceWidget::addRecordToTable(int id, const QString &cardNumber, const QString &userName,
                                        const QString &time, const QString &type, const QString &remark)
{
    int row = ui->tableWidget->rowCount();
    ui->tableWidget->insertRow(row);

    QTableWidgetItem *cardItem = new QTableWidgetItem(cardNumber);
    cardItem->setData(Qt::UserRole, id);
    ui->tableWidget->setItem(row, 0, cardItem);

    ui->tableWidget->setItem(row, 1, new QTableWidgetItem(userName));
    ui->tableWidget->setItem(row, 2, new QTableWidgetItem(ui->dateEdit->date().toString("yyyy-MM-dd")));
    ui->tableWidget->setItem(row, 3, new QTableWidgetItem(time));
    ui->tableWidget->setItem(row, 4, new QTableWidgetItem(type));
    QTableWidgetItem *remarkItem = new QTableWidgetItem(remark);
    if (remark.startsWith("迟到") || remark.startsWith("早退")) {
        remarkItem->setForeground(QColor("#c53030"));
    }
    ui->tableWidget->setItem(row, 5, remarkItem);
}

void AttendanceWidget::loadRecordsByDate(const QDate &date)
{
    ui->tableWidget->setRowCount(0);
    MySql *db = MySql::getMySql();
    QString dateStr = date.toString("yyyy-MM-dd");
    QList<QMap<QString, QString>> records = db->getSignRecordsByDate(dateStr);

    for (const QMap<QString, QString> &record : records) {
        int recordId = record["id"].toInt();
        int cardId = record["cardId"].toInt();
        QString cardNumber = "";
        QSqlQuery query;
        query.prepare("SELECT u.card FROM cards c JOIN \"user\" u ON c.holder_user_name = u.name WHERE c.card_id = :id");
        query.bindValue(":id", cardId);
        if (query.exec() && query.next()) {
            cardNumber = query.value(0).toString();
        }

        QString remark = record.value("remark");
        if (remark.isEmpty()) remark = "正常";

        addRecordToTable(recordId, cardNumber, record["name"], record["time"], record["type"], remark);
    }
    ui->tableWidget->resizeColumnsToContents();
}
