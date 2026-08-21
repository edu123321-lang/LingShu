#include "noticeuser.h"
#include "ui_noticeuser.h"
#include "mysql.h"
#include <QMessageBox>
#include <QDesktopServices>
#include <QFile>
#include <QCoreApplication>
#include <QStandardPaths>   // 新增

noticeUser::noticeUser(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::noticeUser)
{
    ui->setupUi(this);

    ui->contentTextEdit->setReadOnly(true);
    ui->dowmloadBtn->setVisible(false);

    connect(ui->listWidget, &QListWidget::itemClicked, this, &noticeUser::onItemClicked);
    connect(ui->dowmloadBtn, &QPushButton::clicked, this, &noticeUser::on_dowmloadBtn_clicked);

    refreshList();
}

noticeUser::~noticeUser()
{
    delete ui;
}

void noticeUser::refreshList()
{
    ui->listWidget->clear();
    QList<NoticeInfo> notices = MySql::getMySql()->getAllNotices();
    for (const NoticeInfo &info : notices) {
        QString displayText = info.title + " (" + info.createTime + ")";
        QListWidgetItem *item = new QListWidgetItem(displayText);
        item->setData(Qt::UserRole, info.id);
        ui->listWidget->addItem(item);
    }
    ui->contentTextEdit->clear();
    ui->dowmloadBtn->setVisible(false);
    m_currentFilePath.clear();
}

void noticeUser::onItemClicked(QListWidgetItem *item)
{
    if (!item) return;

    int id = item->data(Qt::UserRole).toInt();
    if (id == 0) {
        QMessageBox::warning(this, "错误", "无效的公告ID");
        return;
    }

    NoticeInfo info = MySql::getMySql()->getNoticeById(id);
    if (info.id == 0) {
        QMessageBox::warning(this, "错误", "未找到该公告");
        return;
    }

    ui->contentTextEdit->setText(info.content);

    if (!info.filePath.isEmpty()) {
        ui->dowmloadBtn->setVisible(true);
        m_currentFilePath = info.filePath;
    } else {
        ui->dowmloadBtn->setVisible(false);
        m_currentFilePath.clear();
    }
}

void noticeUser::on_dowmloadBtn_clicked()
{
    if (m_currentFilePath.isEmpty()) {
        QMessageBox::warning(this, "提示", "没有附件可下载");
        return;
    }
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString fullPath = dataDir + "/" + m_currentFilePath;
    if (QFile::exists(fullPath)) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fullPath));
    } else {
        QMessageBox::warning(this, "错误", "附件文件不存在");
    }
}

void noticeUser::on_refreshBtn_clicked()
{
    refreshList();
}
