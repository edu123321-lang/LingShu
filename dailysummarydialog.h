#ifndef DAILYSUMMARYDIALOG_H
#define DAILYSUMMARYDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QDateEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QTableWidget>
#include "mysql.h"

class DailySummaryDialog : public QDialog
{
    Q_OBJECT
public:
    explicit DailySummaryDialog(QWidget *parent = nullptr);

private:
    void setupUI();
    void loadTerminals();
    void showSummary();
    void showAllSummary(const QString &date);  // 新增：显示全部终端的当日汇总表

    QComboBox *m_terminalCombo;
    QDateEdit *m_dateEdit;
    QPushButton *m_queryBtn;
    QPushButton *m_allBtn;       // 一键查看全部终端当日汇总
    QPushButton *m_closeBtn;

    // 单终端汇总卡片区
    QGroupBox *m_singleBox;
    QLabel *m_terminalNameLabel;
    QLabel *m_dateLabel;
    QLabel *m_countLabel;
    QLabel *m_amountLabel;

    // 多终端汇总表格
    QTableWidget *m_tableWidget;
    QGroupBox *m_tableBox;
};

#endif
