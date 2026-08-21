#include "noticegm.h"
#include "ui_noticegm.h"
#include "mysql.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QDesktopServices>
#include <QInputDialog>
#include <QDateTime>
#include <QFile>
#include <QDir>
#include <QCoreApplication>
#include <QStandardPaths>   // 新增
#include <QDebug>

noticeGm::noticeGm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::noticeGm)
{
    ui->setupUi(this);

    connect(ui->addBtn, &QPushButton::clicked, this, &noticeGm::on_addBtn_clicked);
    connect(ui->editBtn, &QPushButton::clicked, this, &noticeGm::on_editBtn_clicked);
    connect(ui->deleteBtn, &QPushButton::clicked, this, &noticeGm::on_deleteBtn_clicked);
    connect(ui->refreshBtn, &QPushButton::clicked, this, &noticeGm::on_refreshBtn_clicked);
    connect(ui->listWidget, &QListWidget::itemClicked, this, &noticeGm::onItemClicked);

    ui->editBtn->setEnabled(false);
    ui->deleteBtn->setEnabled(false);
    ui->downloadBtn->setVisible(false);
    m_currentId = 0;
    refreshList();
}

noticeGm::~noticeGm()
{
    delete ui;
}

void noticeGm::on_editBtn_clicked()
{
    if (m_currentId == 0) {
        QMessageBox::warning(this, "提示", "请先选择一条公告");
        return;
    }
    NoticeInfo info = MySql::getMySql()->getNoticeById(m_currentId);
    if (info.id == 0) {
        QMessageBox::warning(this, "错误", "公告不存在");
        return;
    }

    bool ok;
    QString title = QInputDialog::getText(this, "编辑公告", "标题:", QLineEdit::Normal, info.title, &ok);
    if (!ok || title.isEmpty()) return;

    QString content = QInputDialog::getMultiLineText(this, "编辑公告", "内容:", info.content, &ok);
    if (!ok || content.isEmpty()) return;

    QString filePath = info.filePath;

    // 询问是否更换附件
    int ret = QMessageBox::question(this, "附件", "是否更换附件？", QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        QString fileName = QFileDialog::getOpenFileName(this, "选择附件", "", "所有文件 (*.*)");
        if (!fileName.isEmpty()) {
            // 使用 QStandardPaths 获取应用数据目录
            QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
            QDir dir(dataDir + "/attachments");
            if (!dir.exists()) dir.mkpath(".");

            QString newFileName = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + "_" + QFileInfo(fileName).fileName();
            QString destPath = dataDir + "/attachments/" + newFileName;
            if (QFile::copy(fileName, destPath)) {
                filePath = "attachments/" + newFileName;  // 存储相对路径
            } else {
                QMessageBox::warning(this, "错误", "附件复制失败");
                return;
            }
        }
    }

    info.title = title;
    info.content = content;
    info.filePath = filePath;

    if (MySql::getMySql()->updateNotice(info)) {
        QMessageBox::information(this, "成功", "公告更新成功");
        refreshList();
    } else {
        QMessageBox::warning(this, "错误", "更新公告失败");
    }
}

void noticeGm::on_addBtn_clicked()
{
    bool ok;
    QString title = QInputDialog::getText(this, "添加公告", "标题:", QLineEdit::Normal, "", &ok);
    if (!ok || title.isEmpty()) return;

    QString content = QInputDialog::getMultiLineText(this, "添加公告", "内容:", "", &ok);
    if (!ok || content.isEmpty()) return;

    QString filePath;
    QString fileName = QFileDialog::getOpenFileName(this, "选择附件（可选）", "", "所有文件 (*.*)");
    if (!fileName.isEmpty()) {
        QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir dir(dataDir + "/attachments");
        if (!dir.exists()) dir.mkpath(".");

        QString newFileName = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + "_" + QFileInfo(fileName).fileName();
        QString destPath = dataDir + "/attachments/" + newFileName;
        if (QFile::copy(fileName, destPath)) {
            filePath = "attachments/" + newFileName;
        } else {
            QMessageBox::warning(this, "错误", "附件复制失败");
        }
    }

    NoticeInfo info;
    info.title = title;
    info.content = content;
    info.filePath = filePath;
    if (MySql::getMySql()->addNotice(info)) {
        QMessageBox::information(this, "成功", "公告添加成功");
        refreshList();
    } else {
        QMessageBox::warning(this, "错误", "添加公告失败");
    }
}

void noticeGm::on_deleteBtn_clicked()
{
    if (m_currentId == 0) {
        QMessageBox::warning(this, "提示", "请先选择一条公告");
        return;
    }
    if (QMessageBox::question(this, "确认删除", "确定要删除该公告吗？", QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        if (MySql::getMySql()->deleteNotice(m_currentId)) {
            QMessageBox::information(this, "成功", "删除成功");
            refreshList();
        } else {
            QMessageBox::warning(this, "错误", "删除失败");
        }
    }
}

void noticeGm::on_refreshBtn_clicked()
{
    refreshList();
}

void noticeGm::on_downloadBtn_clicked()
{
    if (m_currentFilePath.isEmpty()) return;
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString fullPath = dataDir + "/" + m_currentFilePath;
    if (QFile::exists(fullPath)) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fullPath));
    } else {
        QMessageBox::warning(this, "错误", "附件文件不存在");
    }
}

void noticeGm::onItemClicked(QListWidgetItem *item)
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

    ui->contentTextEdit->setPlainText(info.content);

    if (!info.filePath.isEmpty()) {
        ui->downloadBtn->setVisible(true);
        m_currentFilePath = info.filePath;
    } else {
        ui->downloadBtn->setVisible(false);
        m_currentFilePath.clear();
    }

    m_currentId = id;
    ui->editBtn->setEnabled(true);
    ui->deleteBtn->setEnabled(true);
}

void noticeGm::refreshList()
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
    ui->downloadBtn->setVisible(false);
    ui->editBtn->setEnabled(false);
    ui->deleteBtn->setEnabled(false);
    m_currentId = 0;
    m_currentFilePath.clear();
}
