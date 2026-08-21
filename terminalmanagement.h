#ifndef TERMINALMANAGEMENT_H
#define TERMINALMANAGEMENT_H

#include <QWidget>
#include "mysql.h"

class QTableWidget;
class QPushButton;
class QLineEdit;
class QLabel;

class TerminalManagement : public QWidget
{
    Q_OBJECT
public:
    explicit TerminalManagement(QWidget *parent = nullptr);

signals:

private slots:
    void refresh();
    void onAdd();
    void onRename();
    void onToggleStatus();
    void onSelectionChanged();

private:
    void setupUI();
    int selectedTerminalId();

    QTableWidget *m_tableWidget;
    QPushButton *m_addBtn;
    QPushButton *m_renameBtn;
    QPushButton *m_toggleBtn;
    QPushButton *m_refreshBtn;
    QLabel *m_hintLabel;
};

#endif
