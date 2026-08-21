#include "userwidget.h"
#include "ui_userwidget.h"
#include <QDebug>
#include "userattendancewidget.h"
#include <QMessageBox>
#include <QButtonGroup>

UserWidget::UserWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::UserWidget)
{
    ui->setupUi(this);

    ui->noticeBtn->setCheckable(true);
    ui->allinoneBtn->setCheckable(true);
    ui->attendanceBtn->setCheckable(true);
    ui->noticeBtn->setChecked(true);

    QButtonGroup *navGroup = new QButtonGroup(this);
    navGroup->setExclusive(true);
    navGroup->addButton(ui->noticeBtn);
    navGroup->addButton(ui->allinoneBtn);
    navGroup->addButton(ui->attendanceBtn);

    setMinimumSize(1200, 760);
}

UserWidget::~UserWidget()
{
    delete ui;
}

void UserWidget::setUserInfo(const QString &name)
{
    userAttendanceWidget *att = qobject_cast<userAttendanceWidget*>(ui->stackedWidget->widget(2));
    if (att) att->setUserName(name);

    // 传递给一卡通页面（索引1）
        userCardWidget *card = qobject_cast<userCardWidget*>(ui->stackedWidget->widget(1));
        if (card) {
            card->setUserName(name);
        } else {
        }
}

void UserWidget::on_noticeBtn_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
}

void UserWidget::on_allinoneBtn_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
}

void UserWidget::on_attendanceBtn_clicked()
{
    ui->stackedWidget->setCurrentIndex(2);
}

void UserWidget::on_logoutBtn_clicked()
{
    if (QMessageBox::question(this, "退出系统", "确定要退出系统吗？",
                                 QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
           emit logoutRequested();
       }
}
