#ifndef CONSUMEDIALOG_H
#define CONSUMEDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include "mysql.h"

class ConsumeDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ConsumeDialog(const QString &userName, double currentBalance, QWidget *parent = nullptr);

    int selectedTerminalId() const;
    QString selectedTerminalName() const;
    double amount() const;

private:
    void setupUI();
    void loadTerminals();

    QString m_userName;
    double m_balance;

    QComboBox *m_terminalCombo;
    QDoubleSpinBox *m_amountSpin;
    QLabel *m_infoLabel;
    QLabel *m_balanceLabel;
    QPushButton *m_okBtn;
    QPushButton *m_cancelBtn;
};

#endif
