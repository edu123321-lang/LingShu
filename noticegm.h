#ifndef NOTICEGM_H
#define NOTICEGM_H

#include <QWidget>
#include <QListWidgetItem>

namespace Ui {
class noticeGm;
}

class noticeGm : public QWidget
{
    Q_OBJECT

public:
    explicit noticeGm(QWidget *parent = nullptr);
    ~noticeGm();

private slots:


    void on_editBtn_clicked();

    void on_addBtn_clicked();

    void on_deleteBtn_clicked();

    void on_refreshBtn_clicked();

    void on_downloadBtn_clicked();
    void onItemClicked(QListWidgetItem *item);

private:
    Ui::noticeGm *ui;
    int m_currentId;//当前选中的公告的id
    QString m_currentFilePath;//当前的公告路径
    void refreshList();//刷新列表
};

#endif // NOTICEGM_H
