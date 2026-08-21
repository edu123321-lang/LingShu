#ifndef USERWIDGET_H
#define USERWIDGET_H

#include <QWidget>

namespace Ui {
class UserWidget;
}

class UserWidget : public QWidget
{
    Q_OBJECT

public:
    explicit UserWidget(QWidget *parent = nullptr);
    ~UserWidget();

    void setUserInfo(const QString &name);

private slots:
    void on_noticeBtn_clicked();

    void on_allinoneBtn_clicked();

    void on_attendanceBtn_clicked();
    void on_logoutBtn_clicked();

signals:
    void logoutRequested();

private:
    Ui::UserWidget *ui;
};

#endif // USERWIDGET_H
