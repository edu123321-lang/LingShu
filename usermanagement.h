#ifndef USERMANAGEMENT_H
#define USERMANAGEMENT_H

#include <QWidget>
#include <QTableWidgetItem>
#include "mysql.h"

namespace Ui {
class userManagement;
}

class userManagement : public QWidget
{
    Q_OBJECT

public:
    explicit userManagement(QWidget *parent = nullptr);
    ~userManagement();

private slots:
    void on_searchBtn_clicked();

    void on_refreshBtn_clicked();

    void on_changePwdBtn_clicked();

    void on_resetPwdBtn_clicked();

    void on_tableWidget_itemSelectionChanged();

    void on_deleteUserBtn_clicked();

private:
    Ui::userManagement *ui;
    void loadUsers(const QString &keyword = "");
    QString getSelectedUserName();

};

#endif // USERMANAGEMENT_H
