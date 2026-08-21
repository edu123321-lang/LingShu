#include "usermanagement.h"
#include "ui_usermanagement.h"
#include <QMessageBox>
#include <QInputDialog>

userManagement::userManagement(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::userManagement)
{
    ui->setupUi(this);

    ui->tableWidget->horizontalHeader()->setStretchLastSection(true);
    ui->tableWidget->setColumnWidth(0, 160);
    ui->tableWidget->setColumnWidth(1, 200);
    ui->tableWidget->setColumnWidth(2, 80);
    ui->tableWidget->setColumnWidth(3, 80);

    ui->changePwdBtn->setEnabled(false);
    ui->resetPwdBtn->setEnabled(false);
    ui->deleteUserBtn->setEnabled(false);

    loadUsers();
}

userManagement::~userManagement()
{
    delete ui;
}

void userManagement::on_searchBtn_clicked()
{
    loadUsers(ui->searchEdit->text().trimmed());
}

void userManagement::on_refreshBtn_clicked()
{
    ui->searchEdit->clear();
        loadUsers();
}

void userManagement::on_changePwdBtn_clicked()
{
    QString userName = getSelectedUserName();
        if (userName.isEmpty()) return;

        bool ok;
        QString newPwd = QInputDialog::getText(this, "修改密码", "请输入新密码：",
                                               QLineEdit::Password, "", &ok);
        if (!ok || newPwd.isEmpty()) return;
        QString confirm = QInputDialog::getText(this, "确认密码", "请再次输入新密码：",
                                                QLineEdit::Password, "", &ok);
        if (!ok || confirm.isEmpty()) return;
        if (newPwd != confirm) {
            QMessageBox::warning(this, "错误", "两次输入密码不一致");
            return;
        }
        MySql *db = MySql::getMySql();
        if (db->updateUserPassword(userName, newPwd)) {
            QMessageBox::information(this, "成功", "密码修改成功");
        } else {
            QMessageBox::critical(this, "失败", db->lastError().isEmpty() ? "修改密码失败" : db->lastError());
        }
}

void userManagement::on_resetPwdBtn_clicked()
{
    QString userName = getSelectedUserName();
        if (userName.isEmpty()) return;

        if (QMessageBox::question(this, "重置密码",
                                  QString("确定将用户 %1 的密码重置为默认密码 123456 吗？").arg(userName),
                                  QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
            MySql *db = MySql::getMySql();
            if (db->resetUserPassword(userName)) {
                QMessageBox::information(this, "成功", "密码已重置为 123456");
            } else {
                QMessageBox::critical(this, "失败", db->lastError().isEmpty() ? "重置密码失败" : db->lastError());
            }
        }
}

void userManagement::on_tableWidget_itemSelectionChanged()
{
    bool hasSelection = !getSelectedUserName().isEmpty();
       ui->changePwdBtn->setEnabled(hasSelection);
       ui->resetPwdBtn->setEnabled(hasSelection);
       ui->deleteUserBtn->setEnabled(hasSelection);
}


void userManagement::on_deleteUserBtn_clicked()
{
    QString userName = getSelectedUserName();
        if (userName.isEmpty()) return;

        // 输入用户名确认
        bool ok;
        QString confirmName = QInputDialog::getText(this, "确认删除",
            QString("请输入要删除的用户名 '%1' 以确认：").arg(userName),
            QLineEdit::Normal, "", &ok);
        if (!ok) return;
        if (confirmName.trimmed() != userName) {
            QMessageBox::warning(this, "错误", "用户名输入不匹配，删除取消");
            return;
        }

        // 二次确认
        if (QMessageBox::question(this, "最终确认",
            QString("您即将永久删除用户 %1 及其所有关联数据（卡片、余额、交易流水、考勤记录），此操作不可恢复！\n确定继续吗？").arg(userName),
            QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
            return;
        }

        MySql *db = MySql::getMySql();
        if (db->deleteUser(userName)) {
            QMessageBox::information(this, "成功", "用户已删除");
            loadUsers(ui->searchEdit->text().trimmed()); // 刷新列表
        } else {
            QMessageBox::critical(this, "失败", db->lastError().isEmpty() ? "删除用户失败" : db->lastError());
        }
}

void userManagement::loadUsers(const QString &keyword)
{
    ui->tableWidget->setRowCount(0);
        MySql *db = MySql::getMySql();
        QList<QMap<QString, QString>> users = db->getAllUsers(keyword);
        for (const auto &map : users) {
            int row = ui->tableWidget->rowCount();
            ui->tableWidget->insertRow(row);
            ui->tableWidget->setItem(row, 0, new QTableWidgetItem(map["name"]));
            ui->tableWidget->setItem(row, 1, new QTableWidgetItem(map["card"]));
            ui->tableWidget->setItem(row, 2, new QTableWidgetItem(map["age"]));
            ui->tableWidget->setItem(row, 3, new QTableWidgetItem(map["sex"]));
            ui->tableWidget->setItem(row, 4, new QTableWidgetItem(map["status"]));
        }
        ui->tableWidget->resizeColumnsToContents();
}

QString userManagement::getSelectedUserName()
{
    int row = ui->tableWidget->currentRow();
        if (row < 0) return QString();
        QTableWidgetItem *item = ui->tableWidget->item(row, 0);
        return item ? item->text() : QString();
}
