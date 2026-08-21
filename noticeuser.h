#ifndef NOTICEUSER_H
#define NOTICEUSER_H

#include <QWidget>
#include <QListWidgetItem>

namespace Ui {
class noticeUser;
}

class noticeUser : public QWidget
{
    Q_OBJECT

public:
    explicit noticeUser(QWidget *parent = nullptr);
    ~noticeUser();
private slots:
    void onItemClicked(QListWidgetItem *item);


    void on_dowmloadBtn_clicked();

    void on_refreshBtn_clicked();

private:
    Ui::noticeUser *ui;
    QString m_currentFilePath;
    void refreshList();
};

#endif // NOTICEUSER_H
