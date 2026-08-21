#ifndef WIDGET_H
#define WIDGET_H
#include "mysql.h"
#include <QWidget>
#include "gmwidget.h"
#include <QMessageBox>
#include "userwidget.h"
#include <QPoint>

namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = nullptr);
    ~Widget();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void on_GmBtn_clicked();

    void on_UserBtn_clicked();

    void on_GmLoginBtn_clicked();

    void on_userLoginBtn_clicked();
    void onGmLogout();

private:
    Ui::Widget *ui;
    Gmwidget *gm;
    UserWidget *user;
    bool m_dragging;
    QPoint m_dragPosition;

};

#endif // WIDGET_H
