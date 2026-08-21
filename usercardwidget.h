#ifndef USERCARDWIDGET_H
#define USERCARDWIDGET_H

#include <QWidget>
#include "mysql.h"

namespace Ui {
class userCardWidget;
}

class userCardWidget : public QWidget
{
    Q_OBJECT

public:
    explicit userCardWidget(QWidget *parent = nullptr);
    ~userCardWidget();

    //由UserWidget调用，传入当前登录的用户名
    void setUserName(const QString &name);
private slots:


    void on_refreshBtn_clicked();

private:
    Ui::userCardWidget *ui;
    QString m_userName;
    int m_cardId;//缓存的card_id

    void loadData();//加载所有数据
    void updateCardInfo();//更新余额
    void loadTransactions();//加载交易流水
};

#endif // USERCARDWIDGET_H
