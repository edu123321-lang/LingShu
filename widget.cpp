#include "widget.h"
#include "ui_widget.h"
#include <QDebug>
#include <QGraphicsDropShadowEffect>
#include <QButtonGroup>
#include <QMouseEvent>

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget),
    m_dragging(false)
{
    ui->setupUi(this);
    MySql *db= MySql::getMySql();
    db->createTable();
    gm=new Gmwidget;

    user=new UserWidget;
    connect(gm, &Gmwidget::logoutRequested, this, &Widget::onGmLogout);
    connect(user, &UserWidget::logoutRequested, this, &Widget::onGmLogout);

    ui->GmBtn->setCheckable(true);
    ui->UserBtn->setCheckable(true);
    ui->GmBtn->setChecked(true);

    QButtonGroup *btnGroup = new QButtonGroup(this);
    btnGroup->setExclusive(true);
    btnGroup->addButton(ui->GmBtn);
    btnGroup->addButton(ui->UserBtn);

    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(40);
    shadow->setXOffset(0);
    shadow->setYOffset(8);
    shadow->setColor(QColor(0, 0, 0, 80));
    ui->stackedWidget->setGraphicsEffect(shadow);

    ui->GmLineEdit->setAttribute(Qt::WA_MacShowFocusRect, false);
    ui->GmPwdLineEdit->setAttribute(Qt::WA_MacShowFocusRect, false);
    ui->UserLineEdit->setAttribute(Qt::WA_MacShowFocusRect, false);
    ui->UserPwdLineEdit->setAttribute(Qt::WA_MacShowFocusRect, false);
}

void Widget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragPosition = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
}

void Widget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPos() - m_dragPosition);
        event->accept();
    }
}

void Widget::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    m_dragging = false;
}

Widget::~Widget()
{
    delete gm;
    delete user;
    delete ui;
}

void Widget::on_GmBtn_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
}

void Widget::on_UserBtn_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
}

void Widget::on_GmLoginBtn_clicked()
{
    QString name = ui->GmLineEdit->text();
    QString pwd = ui->GmPwdLineEdit->text();

    // 简单判空
    if (name.isEmpty() || pwd.isEmpty()) {
        QMessageBox::warning(this, "提示", "用户名或密码不能为空");
        return;
      }
    MySql *db = MySql::getMySql();
       if (db->adminIsExits(name, pwd)) {
           this->hide();
           gm->show();
       }
       else {
           QMessageBox::warning(this, "错误", "用户名或密码错误，请重试");
           ui->GmLineEdit->clear();
           ui->GmPwdLineEdit->clear();
           ui->GmLineEdit->setFocus();
       }

}

void Widget::on_userLoginBtn_clicked()
{
    QString name = ui->UserLineEdit->text();
    QString pwd = ui->UserPwdLineEdit->text();

    if (name.isEmpty() || pwd.isEmpty()) {
        QMessageBox::warning(this, "提示", "用户名或密码不能为空");
        return;
      }
    MySql *db = MySql::getMySql();
    if (db->userIsExits(name, pwd)) {   // 改为验证 user 表
           this->hide();
           user->show();
           user->setUserInfo(name);
       } else {
           QMessageBox::warning(this, "错误", "用户名或密码错误，请重试");
           ui->UserLineEdit->clear();
           ui->UserPwdLineEdit->clear();
           ui->UserLineEdit->setFocus();
    }
}

void Widget::onGmLogout()
{
    ui->GmLineEdit->clear();
       ui->GmPwdLineEdit->clear();
       ui->UserLineEdit->clear();
       ui->UserPwdLineEdit->clear();
       ui->GmLineEdit->setFocus();

       gm->hide();
       user->hide();   // 新增
       this->show();
}
