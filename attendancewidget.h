#ifndef ATTENDANCEWIDGET_H
#define ATTENDANCEWIDGET_H

#include <QWidget>

namespace Ui {
class AttendanceWidget;
}

class AttendanceWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AttendanceWidget(QWidget *parent = nullptr);
    ~AttendanceWidget();

private slots:
    void on_clearTableBtn_clicked();

    void on_saveTimeRuleBtn_clicked();

    void on_dateEdit_dateChanged(const QDate &date);

    void on_editRecordBtn_clicked();

    void on_deleteRecordBtn_clicked();

    void on_refreshBtn_clicked();

    void on_tableWidget_itemSelectionChanged();

private:
    Ui::AttendanceWidget *ui;
    void loadRecordsByDate(const QDate &date);
    void loadTimeRules();
    void addRecordToTable(int id, const QString &cardNumber, const QString &userName,
                          const QString &time, const QString &type, const QString &remark);
    int getSelectedRecordId();
};

#endif // ATTENDANCEWIDGET_H
