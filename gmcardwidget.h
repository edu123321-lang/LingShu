#ifndef GMCARDWIDGET_H
#define GMCARDWIDGET_H

#include <QWidget>
#include <QTableWidgetItem>
#include "mysql.h"

namespace Ui {
class GmCardWidget;
}

class GmCardWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GmCardWidget(QWidget *parent = nullptr);
    ~GmCardWidget();

private slots:

    void on_searchBtn_clicked();

    void on_refreshBtn_clicked();

    void on_topUpBtn_clicked();

    void on_consumeBtn_clicked();

    void on_reportLossBtn_clicked();

    void on_reissueCard_clicked();
    void on_dailySummaryBtn_clicked();
    void on_userTable_itemClicked(QTableWidgetItem *item);

private:
    Ui::GmCardWidget *ui;
    int m_currentCardId;   // 当前选中的 card_id
    void loadCardData(const QString &keyword = "");
    void loadTransaction(int cardId);
    void updateBalanceStatus(int cardId);
    void showMessage(const QString &title, const QString &text, bool isError = false);
    void refreshData();
};

#endif // GMCARDWIDGET_H
