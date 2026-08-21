#include "mysql.h"
#include <QDebug>
#include <QCryptographicHash>
//初始化
MySql * const MySql::p=new MySql("SmartCard.db");
std::recursive_mutex MySql::s_dbMutex;

MySql::MySql(QObject *parent) : QObject(parent)
{

}

MySql::MySql(QString dbName, QObject *parent):QObject(parent)
{
   QSqlDatabase db=QSqlDatabase::addDatabase("QSQLITE");

   db.setDatabaseName(dbName);
   if(db.open())
   {
       qDebug()<<"open db ok"<<endl;
       QSqlQuery pragmaQuery;
       pragmaQuery.exec("PRAGMA foreign_keys = ON");
   }
   else
   {
        qDebug()<<"open db error"<<db.lastError().text()<<endl;
   }
}

MySql::~MySql()
{
    QSqlDatabase db = QSqlDatabase::database();
    if (db.isOpen()) {
        db.close();
    }
}

void MySql::createTable()
{
    std::lock_guard<std::recursive_mutex> locker(s_dbMutex);

    // ===== 兼容旧库：修复引用非唯一列 user(name) 的外键 =====
    // SQLite 要求外键引用列必须 UNIQUE/PRIMARY KEY，而 user.name 无唯一约束。
    // 旧版建表语句 cards / face_features 的 FOREIGN KEY ... REFERENCES user(name)
    // 会导致这两张表的所有 DML 报 "foreign key mismatch" 而失败
    // （典型现象：注册人脸时提示"保存人脸特征失败: Parameter count mismatch"）。
    // 这里检测旧表定义并重建为无外键版本，关联数据一致性由代码（deleteUser 等）手动维护。
    {
        // 注意：QSqlQuery 的 SELECT 结果集在对象存活期间保持打开，同一连接后续 DDL
        // 会被 SQLite 判为 "database table is locked"，因此读取完 SQL 后立即 finish() 释放。

        // face_features：无其他表引用它，可直接重建
        QString ffSql;
        {
            QSqlQuery qff;
            qff.exec("SELECT sql FROM sqlite_master WHERE type='table' AND name='face_features'");
            if (qff.next()) ffSql = qff.value(0).toString();
            qff.finish();   // 释放结果集，避免后续 DDL 被锁
        }
        if (ffSql.contains("REFERENCES user(name)", Qt::CaseInsensitive)) {
            qDebug() << "检测到 face_features 旧外键定义，重建表...";
            QSqlQuery q;
            q.exec("DROP TABLE IF EXISTS face_features_old");   // 清理上次迁移可能的残留
            q.exec("CREATE TABLE face_features_old AS SELECT * FROM face_features");
            q.exec("DROP TABLE face_features");
            q.exec("CREATE TABLE face_features(user_name TEXT NOT NULL PRIMARY KEY, feature BLOB NOT NULL)");
            q.exec("INSERT OR REPLACE INTO face_features SELECT * FROM face_features_old");
            q.exec("DROP TABLE face_features_old");
            qDebug() << "face_features 重建完成，错误信息:" << q.lastError().text();
        }

        // cards：被 sign_in_records / consumption_records 外键引用，需临时关闭外键再重建
        QString cardsSql;
        {
            QSqlQuery qc;
            qc.exec("SELECT sql FROM sqlite_master WHERE type='table' AND name='cards'");
            if (qc.next()) cardsSql = qc.value(0).toString();
            qc.finish();
        }
        if (cardsSql.contains("REFERENCES user(name)", Qt::CaseInsensitive)) {
            qDebug() << "检测到 cards 旧外键定义，重建表...";
            QSqlQuery q;
            q.exec("PRAGMA foreign_keys = OFF");
            q.exec("DROP TABLE IF EXISTS cards_old");
            q.exec("CREATE TABLE cards_old AS SELECT * FROM cards");
            q.exec("DROP TABLE cards");
            q.exec("CREATE TABLE cards("
                    "card_id INTEGER PRIMARY KEY AUTOINCREMENT,"
                    "holder_user_name TEXT NOT NULL,"
                    "balance DECIMAL DEFAULT 0.0,"
                    "create_time DATETIME NOT NULL,"
                    "status TEXT DEFAULT '正常' CHECK(status IN ('正常','挂失','已补办')))");
            q.exec("INSERT OR REPLACE INTO cards SELECT * FROM cards_old");
            q.exec("DROP TABLE cards_old");
            q.exec("PRAGMA foreign_keys = ON");
            qDebug() << "cards 重建完成，错误信息:" << q.lastError().text();
        }
    }

    QSqlQuery query;
    QString sql="CREATE TABLE IF NOT EXISTS admin(name TEXT NOT NULL PRIMARY KEY,pwd TEXT NOT NULL)";

    if(query.exec(sql))
    {
        qDebug()<<"create table admin ok"<<endl;
    }
    else
    {
        qDebug()<<"create table admin error"<<query.lastError().text()<<endl;
    }

    sql="CREATE TABLE IF NOT EXISTS user("
            "name TEXT NOT NULL,"
            "pwd TEXT NOT NULL,"
            "age int NOT NULL CHECK(age>=0 AND age<=100),"
            "sex TEXT NOT NULL CHECK(sex IN('男','女')),"
            "card TEXT NOT NULL PRIMARY KEY)";
    if(query.exec(sql))
    {
        qDebug()<<"create table user ok"<<endl;
    }
    else
    {
        qDebug()<<"create table user error"<<query.lastError().text()<<endl;
    }


    sql="CREATE TABLE IF NOT EXISTS cards("
        "card_id INTEGER PRIMARY KEY AUTOINCREMENT,"
         "holder_user_name TEXT NOT NULL,"
         "balance DECIMAL DEFAULT 0.0,"
         "create_time DATETIME NOT NULL,"
         "status TEXT DEFAULT '正常' CHECK(status IN ('正常','挂失','已补办')))";
    if(query.exec(sql))
    {
        qDebug()<<"create table cards ok"<<endl;
    }
    else
    {
        qDebug()<<"create table cards error"<<query.lastError().text()<<endl;
    }
    //签到签退记录表（升级：新增 remark 列用于标记迟到/早退等）
    sql="CREATE TABLE IF NOT EXISTS sign_in_records("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "card_id INTEGER NOT NULL,"
            "name TEXT NOT NULL,"
            "sign_date TEXT NOT NULL,"
            "sign_time TEXT NOT NULL,"
            "type TEXT NOT NULL,"
            "remark TEXT DEFAULT '',"
            "FOREIGN KEY (card_id) REFERENCES cards(card_id) ON DELETE CASCADE,"
            "CONSTRAINT unique_daily_check UNIQUE (card_id, sign_date, type))";

    if(query.exec(sql)) {
        qDebug() << "create table sign_in_records ok";
    } else {
        qDebug() << "create table sign_in_records error" << query.lastError().text();
    }
    query.exec("ALTER TABLE sign_in_records ADD COLUMN remark TEXT DEFAULT ''");

    // 系统设置表：存储签到/签退时间等全局配置
    sql = "CREATE TABLE IF NOT EXISTS sys_settings("
          "setting_key TEXT NOT NULL PRIMARY KEY,"
          "setting_value TEXT NOT NULL)";
    if (query.exec(sql)) {
        qDebug() << "create table sys_settings ok";
    } else {
        qDebug() << "create table sys_settings error" << query.lastError().text();
    }

    //消费记录表（升级：增加终端字段）
    sql = "CREATE TABLE IF NOT EXISTS consumption_records("
          "id INTEGER PRIMARY KEY AUTOINCREMENT,"
          "card_id INTEGER NOT NULL,"
          "name TEXT NOT NULL,"
          "amount REAL NOT NULL,"
          "create_time DATETIME NOT NULL,"
          "terminal_id INTEGER DEFAULT 0,"
          "terminal_name TEXT DEFAULT '未指定',"
          "FOREIGN KEY (card_id) REFERENCES cards(card_id) ON DELETE CASCADE)";
    if(query.exec(sql)) {
        qDebug() << "create table consumption_records ok";
    } else {
        qDebug() << "create table consumption_records error" << query.lastError().text();
    }

    // SQLite: 如果表已存在（老用户数据库），则手动 ADD COLUMN，重复执行会忽略错误
    query.exec("ALTER TABLE consumption_records ADD COLUMN terminal_id INTEGER DEFAULT 0");
    query.exec("ALTER TABLE consumption_records ADD COLUMN terminal_name TEXT DEFAULT '未指定'");

    //消费终端表
    sql = "CREATE TABLE IF NOT EXISTS consume_terminals("
          "id INTEGER PRIMARY KEY AUTOINCREMENT,"
          "name TEXT NOT NULL UNIQUE,"
          "status TEXT DEFAULT '正常' CHECK(status IN ('正常','停用')),"
          "create_time DATETIME NOT NULL)";
    if(query.exec(sql)) {
        qDebug() << "create table consume_terminals ok";
    } else {
        qDebug() << "create table consume_terminals error" << query.lastError().text();
    }

    // 初始化默认消费终端（不存在才插入，UNIQUE冲突忽略）
    QStringList defaultTerminals;
    defaultTerminals << "一食堂" << "超市" << "打印店";
    QSqlQuery insTerminal;
    insTerminal.prepare("INSERT OR IGNORE INTO consume_terminals(name, create_time) VALUES(?, ?)");
    foreach (const QString &name, defaultTerminals) {
        insTerminal.addBindValue(name);
        insTerminal.addBindValue(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
        insTerminal.exec();
    }

    //充值记录表
    sql = "CREATE TABLE IF NOT EXISTS topup_records("
          "id INTEGER PRIMARY KEY AUTOINCREMENT,"
          "card_id INTEGER NOT NULL,"
          "name TEXT NOT NULL,"
          "amount REAL NOT NULL,"
          "create_time DATETIME NOT NULL,"
          "FOREIGN KEY (card_id) REFERENCES cards(card_id) ON DELETE CASCADE)";
    if(query.exec(sql)) {
        qDebug() << "create table topup_records ok";
    } else {
        qDebug() << "create table topup_records error" << query.lastError().text();
    }

    //nitoce表：存储公告信息
    sql="CREATE TABLE IF NOT EXISTS notice("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "title TEXT NOT NULL,"
            "content TEXT NOT NULL,"
            "file_path TEXT,"
            "create_time DATETIME NOT NULL,"
            "update_time DATETIME NOT NULL,"
            " is_visible INTEGER DEFAULT 1)";
    if(query.exec(sql)) {
        qDebug() << "create table notice ok";
    } else {
        qDebug() << "create table notice error" << query.lastError().text();
    }



    sql = "CREATE TABLE IF NOT EXISTS face_features("
          "user_name TEXT NOT NULL PRIMARY KEY,"
          "feature BLOB NOT NULL)";
    if (query.exec(sql)) {
        qDebug() << "create table face_features ok";
    } else {
        qDebug() << "create table face_features error" << query.lastError().text();
    }

    QString insertSql = "INSERT INTO admin(name, pwd) VALUES (?, ?)";
    query.prepare(insertSql);
    query.addBindValue("root");
    query.addBindValue(MySql::hashPassword("123456"));
      if(query.exec())
      {
          qDebug()<<"初始化管理员root成功";
      }
      else
      {
          QString err = query.lastError().text();
          qDebug()<<"插入root提示:"<<err;
          if(err.contains("UNIQUE"))
              qDebug()<<"管理员root已存在，无需重复添加";
      }



}

void MySql::insertData(QString name, QString pwd, QString age, QString card, QString sex)
{
    std::lock_guard<std::recursive_mutex> locker(s_dbMutex);
    if (name.trimmed().isEmpty() || pwd.trimmed().isEmpty() || card.trimmed().isEmpty()) {
        m_lastError = "用户名、密码和卡号不能为空";
        return;
    }

    int ageInt = age.toInt();
    if (ageInt < 0 || ageInt > 100) {
        m_lastError = "年龄超出有效范围(0-100)，已自动设为0";
        ageInt = 0;
    }

    QString normalizedCard = normalizedCardNumber(card);

    QSqlQuery fkOff;
    fkOff.exec("PRAGMA foreign_keys = OFF");

    QSqlDatabase db = QSqlDatabase::database();
    if (!db.transaction()) {
        QSqlQuery fkOn;
        fkOn.exec("PRAGMA foreign_keys = ON");
        m_lastError = "开启事务失败: " + db.lastError().text();
        qDebug() << m_lastError;
        return;
    }

    QString errMsg;
    bool ok = false;

    do {
        QList<int> orphanCardIds;
        QSqlQuery cardIdQuery;
        cardIdQuery.prepare("SELECT card_id FROM cards WHERE holder_user_name = ?");
        cardIdQuery.addBindValue(name);
        if (cardIdQuery.exec()) {
            while (cardIdQuery.next()) {
                orphanCardIds.append(cardIdQuery.value(0).toInt());
            }
        }

        foreach (int cid, orphanCardIds) {
            QSqlQuery q1;
            q1.prepare("DELETE FROM consumption_records WHERE card_id = ?");
            q1.addBindValue(cid);
            q1.exec();
            QSqlQuery q2;
            q2.prepare("DELETE FROM topup_records WHERE card_id = ?");
            q2.addBindValue(cid);
            q2.exec();
            QSqlQuery q3;
            q3.prepare("DELETE FROM sign_in_records WHERE card_id = ?");
            q3.addBindValue(cid);
            q3.exec();
        }

        QSqlQuery qCards;
        qCards.prepare("DELETE FROM cards WHERE holder_user_name = ?");
        qCards.addBindValue(name);
        qCards.exec();

        QSqlQuery qSign;
        qSign.prepare("DELETE FROM sign_in_records WHERE name = ?");
        qSign.addBindValue(name);
        qSign.exec();
        QSqlQuery qCons;
        qCons.prepare("DELETE FROM consumption_records WHERE name = ?");
        qCons.addBindValue(name);
        qCons.exec();
        QSqlQuery qTop;
        qTop.prepare("DELETE FROM topup_records WHERE name = ?");
        qTop.addBindValue(name);
        qTop.exec();

        QSqlQuery query;
        query.prepare("INSERT INTO \"user\" (name, pwd, age, sex, card) VALUES (?, ?, ?, ?, ?)");
        query.addBindValue(name);
        query.addBindValue(MySql::hashPassword(pwd));
        query.addBindValue(ageInt);
        query.addBindValue(sex);
        query.addBindValue(normalizedCard);

        if (!query.exec()) {
            errMsg = "插入用户失败: " + query.lastError().text();
            qDebug() << errMsg;
            break;
        }

        QSqlQuery cardQuery;
        cardQuery.prepare("INSERT INTO cards (holder_user_name, balance, create_time) VALUES (?, ?, ?)");
        cardQuery.addBindValue(name);
        cardQuery.addBindValue(0.0);
        cardQuery.addBindValue(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
        if (!cardQuery.exec()) {
            errMsg = "创建卡片记录失败: " + cardQuery.lastError().text();
            qDebug() << errMsg;
            break;
        }

        if (!db.commit()) {
            errMsg = "事务提交失败: " + db.lastError().text();
            qDebug() << errMsg;
            break;
        }
        ok = true;
    } while (false);

    if (!ok) {
        db.rollback();
        m_lastError = errMsg.isEmpty() ? "插入用户失败" : errMsg;
    } else {
        m_lastError.clear();
        qDebug() << "插入用户成功，卡号：" << normalizedCard;
    }

    QSqlQuery fkOn;
    fkOn.exec("PRAGMA foreign_keys = ON");
}

MySql *MySql::getMySql()
{
    return p;
}
//数据库中查询
bool MySql::adminIsExits(QString name, QString pwd)
{
    std::lock_guard<std::recursive_mutex> locker(s_dbMutex);
    QSqlQuery query;
    query.prepare("SELECT * FROM admin WHERE name=? AND pwd=?");
    query.addBindValue(name);
    query.addBindValue(MySql::hashPassword(pwd));

    if(query.exec())
    {
        qDebug()<<"执行查询语句成功"<<endl;
    }
    else
    {
        qDebug()<<"执行查询语句失败"<<endl;
    }

    while(query.next())
        return  true;
    return false;
}

bool MySql::userIsExits(QString name, QString pwd)
{
    std::lock_guard<std::recursive_mutex> locker(s_dbMutex);
    QSqlQuery query;
        query.prepare("SELECT * FROM \"user\" WHERE name=? AND pwd=?");
        query.addBindValue(name);
        query.addBindValue(MySql::hashPassword(pwd));

        if (query.exec()) {
            qDebug() << "执行用户查询语句成功";
        } else {
            qDebug() << "执行用户查询语句失败" << query.lastError().text();
            return false;
        }

        while (query.next())
            return true;
        return false;
}

// 获取所有可见公告
QList<NoticeInfo> MySql::getAllNotices() {
    std::lock_guard<std::recursive_mutex> locker(s_dbMutex);
    QList<NoticeInfo> list;
    QSqlQuery query("SELECT id, title, content, file_path, create_time, update_time "
                    "FROM notice WHERE is_visible=1 ORDER BY create_time DESC");
    while (query.next()) {
        NoticeInfo info;
        info.id = query.value(0).toInt();
        info.title = query.value(1).toString();
        info.content = query.value(2).toString();
        info.filePath = query.value(3).toString();
        info.createTime = query.value(4).toString();
        info.updateTime = query.value(5).toString();
        info.isVisible = true;
        list.append(info);
    }
    return list;
}

NoticeInfo MySql::getNoticeById(int id)
{
    std::lock_guard<std::recursive_mutex> locker(s_dbMutex);
    NoticeInfo info;
    QSqlQuery query;
    query.prepare("SELECT id, title, content, file_path, create_time, update_time FROM notice WHERE id=? AND is_visible=1");
    query.addBindValue(id);
       if (query.exec() && query.next()) {
           info.id = query.value(0).toInt();
           info.title = query.value(1).toString();
           info.content = query.value(2).toString();
           info.filePath = query.value(3).toString();
           info.createTime = query.value(4).toString();
           info.updateTime = query.value(5).toString();
           info.isVisible = true;
       }
       return info;
}

// 添加公告
bool MySql::addNotice(const NoticeInfo &info)
{
    std::lock_guard<std::recursive_mutex> locker(s_dbMutex);
    QSqlQuery query;
    QString now = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    query.prepare("INSERT INTO notice (title, content, file_path, create_time, update_time, is_visible) "
                  "VALUES (?, ?, ?, ?, ?, 1)");
    query.addBindValue(info.title);
    query.addBindValue(info.content);
    query.addBindValue(info.filePath);
    query.addBindValue(now);
    query.addBindValue(now);

    if (!query.exec()) {
        m_lastError = "添加公告失败: " + query.lastError().text();
        return false;
    }

    // 成功，清除错误
    m_lastError.clear();
    return true;
}

// 更新公告
bool MySql::updateNotice(const NoticeInfo &info) {
    std::lock_guard<std::recursive_mutex> locker(s_dbMutex);
    QSqlQuery query;
    query.prepare("UPDATE notice SET title=?, content=?, file_path=?, update_time=? "
                  "WHERE id=? AND is_visible=1");
    query.addBindValue(info.title);
    query.addBindValue(info.content);
    query.addBindValue(info.filePath);
    query.addBindValue(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    query.addBindValue(info.id);
    return query.exec();
}

bool MySql::deleteNotice(int id)
{
    std::lock_guard<std::recursive_mutex> locker(s_dbMutex);
    QSqlQuery query;
        query.prepare("UPDATE notice SET is_visible=0 WHERE id=?");
        query.addBindValue(id);
        return query.exec();
}

int MySql::getCardIdByCardNumber(const QString &cardNumber)
{
    std::lock_guard<std::recursive_mutex> locker(s_dbMutex);
    QSqlQuery query;
    query.prepare("SELECT c.card_id FROM cards c JOIN \"user\" u ON c.holder_user_name = u.name WHERE u.card = ?");
    query.addBindValue(cardNumber);
    if (query.exec() && query.next())
    {
        return query.value(0).toInt();
    }
        return -1;
}

QString MySql::getUserNameByCardNumber(const QString &cardNumber)
{
    std::lock_guard<std::recursive_mutex> locker(s_dbMutex);
    QSqlQuery query;
    query.prepare("SELECT name FROM \"user\" WHERE card = ?");
    query.addBindValue(cardNumber);
    if (query.exec() && query.next())
    {
        return query.value(0).toString();
    }
        return QString();
}

bool MySql::addSignRecord(int cardId, const QString &name, const QString &type, const QString &remark)
{
    std::lock_guard<std::recursive_mutex> locker(s_dbMutex);
    QSqlQuery query;
    query.prepare("INSERT INTO sign_in_records (card_id, name, sign_date, sign_time, type, remark) "
                      "VALUES (?, ?, ?, ?, ?, ?)");
    query.addBindValue(cardId);
    query.addBindValue(name);
    query.addBindValue(QDate::currentDate().toString("yyyy-MM-dd"));
    query.addBindValue(QTime::currentTime().toString("hh:mm:ss"));
    query.addBindValue(type);
    query.addBindValue(remark);
    bool ok = query.exec();
    if (!ok) m_lastError = query.lastError().text();
    return ok;
}

QString MySql::getLastSignType(int cardId, const QString &date)
{
    std::lock_guard<std::recursive_mutex> locker(s_dbMutex);
    QSqlQuery query;
    query.prepare("SELECT type FROM sign_in_records WHERE card_id = ? AND sign_date = ? ORDER BY sign_time DESC LIMIT 1");
    query.addBindValue(cardId);
    query.addBindValue(date);
    if (query.exec() && query.next())
    {
        return query.value(0).toString();
    }
        return QString();  // 无记录返回空
}

QList<QMap<QString, QString> > MySql::getTodaySignRecords()
{
    std::lock_guard<std::recursive_mutex> locker(s_dbMutex);
    QList<QMap<QString, QString>> list;
        QString today = QDate::currentDate().toString("yyyy-MM-dd");
        QSqlQuery query;
        query.prepare("SELECT id, card_id, name, sign_time, type, COALESCE(remark,'') FROM sign_in_records WHERE sign_date = ? ORDER BY sign_time DESC");
        query.addBindValue(today);
        if (query.exec()) {
            while (query.next()) {
                QMap<QString, QString> record;
                record["id"] = query.value(0).toString();
                record["cardId"] = query.value(1).toString();
                record["name"] = query.value(2).toString();
                record["time"] = query.value(3).toString();
                record["type"] = query.value(4).toString();
                record["remark"] = query.value(5).toString();
                list.append(record);
            }
        }
        return list;
}

QList<QMap<QString, QString> > MySql::getUserSignRecords(const QString &name)
{
    std::lock_guard<std::recursive_mutex> locker(s_dbMutex);
    QList<QMap<QString, QString>> list;
       QSqlQuery query;
       query.prepare("SELECT sign_date, sign_time, type, COALESCE(remark,'') FROM sign_in_records WHERE name = ? ORDER BY sign_date DESC, sign_time DESC");
       query.addBindValue(name);
       if (query.exec()) {
           while (query.next()) {
               QMap<QString, QString> record;
               record["date"] = query.value(0).toString();
               record["time"] = query.value(1).toString();
               record["type"] = query.value(2).toString();
               record["remark"] = query.value(3).toString();
               list.append(record);
           }
       }
       return list;
}

QString MySql::getSystemSetting(const QString &key, const QString &defaultValue)
{
    std::lock_guard<std::recursive_mutex> locker(s_dbMutex);
    QSqlQuery query;
    query.prepare("SELECT setting_value FROM sys_settings WHERE setting_key = ?");
    query.addBindValue(key);
    if (query.exec() && query.next()) {
        return query.value(0).toString();
    }
    return defaultValue;
}

bool MySql::setSystemSetting(const QString &key, const QString &value)
{
    std::lock_guard<std::recursive_mutex> locker(s_dbMutex);
    QSqlQuery query;
    query.prepare("INSERT INTO sys_settings(setting_key, setting_value) VALUES(?, ?) "
                  "ON CONFLICT(setting_key) DO UPDATE SET setting_value=excluded.setting_value");
    query.addBindValue(key);
    query.addBindValue(value);
    bool ok = query.exec();
    if (!ok) m_lastError = query.lastError().text();
    return ok;
}

QString MySql::getWorkSignInDeadline()
{
    std::lock_guard<std::recursive_mutex> locker(s_dbMutex);
    return getSystemSetting("work_signin_deadline", "09:00");
}

QString MySql::getWorkSignOffStart()
{
    std::lock_guard<std::recursive_mutex> locker(s_dbMutex);
    return getSystemSetting("work_signoff_start", "18:00");
}

bool MySql::setWorkSignInDeadline(const QString &hhmm)
{
    std::lock_guard<std::recursive_mutex> locker(s_dbMutex);
    return setSystemSetting("work_signin_deadline", hhmm);
}

bool MySql::setWorkSignOffStart(const QString &hhmm)
{
    std::lock_guard<std::recursive_mutex> locker(s_dbMutex);
    return setSystemSetting("work_signoff_start", hhmm);
}

QString MySql::normalizedCardNumber(const QString &card)
{
    std::lock_guard<std::recursive_mutex> locker(s_dbMutex);
    QString normalized;
    for (QChar ch : card) {
        if (ch.isLetterOrNumber())
        {
            normalized.append(ch.toLower());
        }
    }
    return normalized;
}

//获取卡片信息
QList<QMap<QString, QString> > MySql::getAllCardInfo()
{
    std::lock_guard<std::recursive_mutex> locker(s_dbMutex);
    QList<QMap<QString, QString>> list;
        QSqlQuery query;
        QString sql = "SELECT u.name, u.card AS card_number, c.balance, c.status, c.card_id "
                      "FROM \"user\" u JOIN cards c ON u.name = c.holder_user_name";
        if (!query.exec(sql)) {
            qDebug() << "getAllCardInfo 查询失败：" << query.lastError().text();
            qDebug() << "执行的SQL：" << sql;
            return list;
        }
        while (query.next()) {
            QMap<QString, QString> map;
            map["name"] = query.value(0).toString();
            map["card_number"] = query.value(1).toString();
            map["balance"] = query.value(2).toString();
            map["status"] = query.value(3).toString();
            map["card_id"] = query.value(4).toString();
            list.append(map);
        }
        qDebug() << "getAllCardInfo 查询到" << list.size() << "条记录";
        return list;
}

//搜索功能
QList<QMap<QString, QString> > MySql::searchCards(const QString &keyword)
{
    std::lock_guard<std::recursive_mutex> locker(s_dbMutex);
    QList<QMap<QString, QString>> list;
    QSqlQuery query;
    QString kw = "%" + keyword + "%";
    query.prepare(
        "SELECT u.name, u.card AS card_number, c.balance, c.status, c.card_id "
        "FROM \"user\" u JOIN cards c ON u.name = c.holder_user_name "
        "WHERE u.name LIKE ? OR u.card LIKE ?"
    );
    query.addBindValue(kw);
    query.addBindValue(kw);
    if (query.exec()) {
        while (query.next())
        {
            QMap<QString, QString> map;
            map["name"] = query.value(0).toString();
            map["card_number"] = query.value(1).toString();
            map["balance"] = query.value(2).toString();
            map["status"] = query.value(3).toString();
            map["card_id"] = query.value(4).toString();
            list.append(map);
        }
    }
    return list;
}

//获取交易记录
QList<QMap<QString, QString> > MySql::getCardTransactions(int cardId)
{
    std::lock_guard<std::recursive_mutex> locker(s_dbMutex);
    QList<QMap<QString, QString>> list;
    // 使用 UNION 合并两张表，按时间倒序
    QSqlQuery query;
    query.prepare(
        "SELECT create_time, '充值' AS type, amount, '系统充值' AS terminal FROM topup_records WHERE card_id = ? "
        "UNION ALL "
        "SELECT create_time, '消费' AS type, -amount AS amount, terminal_name AS terminal FROM consumption_records WHERE card_id = ? "
        "ORDER BY create_time DESC"
    );
    query.addBindValue(cardId);
    query.addBindValue(cardId);
    if (query.exec()) {
        while (query.next())
        {
            QMap<QString, QString> map;
            map["time"] = query.value(0).toString();
            map["type"] = query.value(1).toString();
            map["amount"] = query.value(2).toString();  // 消费为负数
            map["terminal"] = query.value(3).toString();
            list.append(map);
        }
    }
    return list;
}

//根据card_id获取持卡人的姓名
QString MySql::getUserNameByCardId(int cardId)
{
    std::lock_guard<std::recursive_mutex> locker(s_dbMutex);
    QSqlQuery query;
    query.prepare("SELECT holder_user_name FROM cards WHERE card_id = ?");
    query.addBindValue(cardId);
    if (query.exec() && query.next())
    {
        return query.value(0).toString();
    }
    return QString();
}

//充值功能
bool MySql::topUp(int cardId, double amount)
{
    std::lock_guard<std::recursive_mutex> locker(s_dbMutex);
    if (amount <= 0) {
            m_lastError = "充值金额必须大于0";
            return false;
        }
        QSqlDatabase::database().transaction();
        QSqlQuery query;
        query.prepare("UPDATE cards SET balance = balance + ? WHERE card_id = ?");
        query.addBindValue(amount);
        query.addBindValue(cardId);
        if (!query.exec()) {
            m_lastError = "更新余额失败: " + query.lastError().text();
            QSqlDatabase::database().rollback();
            return false;
        }
        QString name = getUserNameByCardId(cardId);
        if (name.isEmpty()) {
            m_lastError = "未找到持卡人";
            QSqlDatabase::database().rollback();
            return false;
        }
        query.prepare("INSERT INTO topup_records (card_id, name, amount, create_time) "
                      "VALUES (?, ?, ?, ?)");
        query.addBindValue(cardId);
        query.addBindValue(name);
        query.addBindValue(amount);
        query.addBindValue(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
        if (!query.exec()) {
            m_lastError = "插入充值记录失败: " + query.lastError().text();
            QSqlDatabase::database().rollback();
            return false;
        }
        if (!QSqlDatabase::database().commit()) {
            m_lastError = "事务提交失败: " + QSqlDatabase::database().lastError().text();
            return false;
        }
        m_lastError.clear();   // 成功后清除错误
        return true;
}

//扣费
bool MySql::consume(int cardId, double amount, int terminalId)
{
    std::lock_guard<std::recursive_mutex> locker(s_dbMutex);
    // 参数校验
    if (amount <= 0) {
        m_lastError = "扣费金额必须大于0";
        return false;
    }

    // 查询当前余额
    QSqlQuery check;
    check.prepare("SELECT balance FROM cards WHERE card_id = ?");
    check.addBindValue(cardId);
    if (!check.exec()) {
        m_lastError = "查询余额失败: " + check.lastError().text();
        return false;
    }
    if (!check.next()) {
        m_lastError = "未找到该卡片 (card_id = " + QString::number(cardId) + ")";
        return false;
    }
    double balance = check.value(0).toDouble();
    if (balance < amount) {
        m_lastError = "余额不足，当前余额: " + QString::number(balance, 'f', 2);
        return false;
    }

    // 解析终端
    QString terminalName = "未指定";
    int realTerminalId = 0;
    if (terminalId > 0) {
        QString name = getTerminalName(terminalId);
        if (!name.isEmpty()) {
            terminalName = name;
            realTerminalId = terminalId;
        }
    }

    // 开启事务
    QSqlDatabase::database().transaction();

    // 更新余额
    QSqlQuery query;
    query.prepare("UPDATE cards SET balance = balance - ? WHERE card_id = ?");
    query.addBindValue(amount);
    query.addBindValue(cardId);
    if (!query.exec()) {
        m_lastError = "更新余额失败: " + query.lastError().text();
        QSqlDatabase::database().rollback();
        return false;
    }

    // 获取持卡人姓名
    QString name = getUserNameByCardId(cardId);
    if (name.isEmpty()) {
        m_lastError = "未找到持卡人 (card_id = " + QString::number(cardId) + ")";
        QSqlDatabase::database().rollback();
        return false;
    }

    // 插入消费记录（带终端）
    query.prepare("INSERT INTO consumption_records (card_id, name, amount, create_time, terminal_id, terminal_name) "
                  "VALUES (?, ?, ?, ?, ?, ?)");
    query.addBindValue(cardId);
    query.addBindValue(name);
    query.addBindValue(amount);
    query.addBindValue(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    query.addBindValue(realTerminalId);
    query.addBindValue(terminalName);
    if (!query.exec()) {
        m_lastError = "插入消费记录失败: " + query.lastError().text();
        QSqlDatabase::database().rollback();
        return false;
    }

    // 提交事务
    if (!QSqlDatabase::database().commit()) {
        m_lastError = "事务提交失败: " + QSqlDatabase::database().lastError().text();
        return false;
    }

    // 成功，清除错误
    m_lastError.clear();
    return true;
}

//挂失
bool MySql::reportLoss(int cardId)
{
    std::lock_guard<std::recursive_mutex> locker(s_dbMutex);
    QSqlQuery query;
    query.prepare("UPDATE cards SET status = '挂失' WHERE card_id = ?");
    query.addBindValue(cardId);
    return query.exec();
}

//补办
bool MySql::reissueCard(int cardId, const QString &newCardNumber)
{
    std::lock_guard<std::recursive_mutex> locker(s_dbMutex);
    if (newCardNumber.isEmpty()) return false;
        // 先检查新卡号是否已被使用（在 user 表中）
        QSqlQuery check;
        check.prepare("SELECT card FROM \"user\" WHERE card = ?");
        check.addBindValue(newCardNumber);
        if (check.exec() && check.next()) {
            return false;  // 卡号已存在
        }
        // 获取当前持卡人姓名
        QString name = getUserNameByCardId(cardId);
        if (name.isEmpty()) return false;

        QSqlDatabase::database().transaction();
        QSqlQuery query;
        // 更新 cards 状态为正常
        query.prepare("UPDATE cards SET status = '正常' WHERE card_id = ?");
        query.addBindValue(cardId);
        if (!query.exec()) {
            QSqlDatabase::database().rollback();
            return false;
        }
        // 更新 user 表中的卡号
        query.prepare("UPDATE \"user\" SET card = ? WHERE name = ?");
        query.addBindValue(newCardNumber);
        query.addBindValue(name);
        if (!query.exec()) {
            QSqlDatabase::database().rollback();
            return false;
        }
        return QSqlDatabase::database().commit();
}

int MySql::getCardIdByUserName(const QString &userName)
{
    std::lock_guard<std::recursive_mutex> locker(s_dbMutex);
    QSqlQuery query;
    query.prepare("SELECT c.card_id FROM cards c JOIN \"user\" u ON c.holder_user_name = u.name WHERE u.name = ?");
    query.addBindValue(userName);
    if (query.exec() && query.next())
    {
        return query.value(0).toInt();
    }
    return -1;
}

QMap<QString, QString> MySql::getCardInfoByCardId(int cardId)
{
    std::lock_guard<std::recursive_mutex> locker(s_dbMutex);
    QMap<QString, QString> info;
    QSqlQuery query;
    query.prepare("SELECT balance, status FROM cards WHERE card_id = ?");
    query.addBindValue(cardId);
    if (query.exec() && query.next())
    {
        info["balance"] = query.value(0).toString();
        info["status"] = query.value(1).toString();
    }
    return info;
}

QString MySql::hashPassword(const QString &password)
{
    std::lock_guard<std::recursive_mutex> locker(s_dbMutex);
    QByteArray hash = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256);
    return QString(hash.toHex());
}

QString MySql::lastError() const
{
    std::lock_guard<std::recursive_mutex> locker(s_dbMutex);
    return m_lastError;
}

QList<QMap<QString, QString> > MySql::getAllUsers(const QString &keyword)
{
    std::lock_guard<std::recursive_mutex> locker(s_dbMutex);
    QList<QMap<QString, QString>> list;
        QSqlQuery query;
        QString sql = "SELECT u.name, u.card, u.age, u.sex, c.status "
                      "FROM \"user\" u LEFT JOIN cards c ON u.name = c.holder_user_name";
        if (!keyword.isEmpty()) {
            sql += " WHERE u.name LIKE ? OR u.card LIKE ?";
        }
        query.prepare(sql);
        if (!keyword.isEmpty()) {
            QString kw = "%" + keyword + "%";
            query.addBindValue(kw);
            query.addBindValue(kw);
        }
        if (query.exec()) {
            while (query.next()) {
                QMap<QString, QString> map;
                map["name"] = query.value(0).toString();
                map["card"] = query.value(1).toString();
                map["age"] = query.value(2).toString();
                map["sex"] = query.value(3).toString();
                map["status"] = query.value(4).toString();
                if (map["status"].isEmpty()) map["status"] = "未开通";
                list.append(map);
            }
        } else {
            qDebug() << "getAllUsers error:" << query.lastError().text();
        }
        return list;
}

bool MySql::updateUserPassword(const QString &userName, const QString &newPassword)
{
    std::lock_guard<std::recursive_mutex> locker(s_dbMutex);
    if (userName.isEmpty() || newPassword.isEmpty()) {
            m_lastError = "用户名或密码不能为空";
            return false;
        }
        QSqlQuery query;
        query.prepare("UPDATE \"user\" SET pwd = ? WHERE name = ?");
        query.addBindValue(MySql::hashPassword(newPassword));
        query.addBindValue(userName);
        if (query.exec()) {
            if (query.numRowsAffected() > 0) {
                m_lastError.clear();
                return true;
            }
            m_lastError = "未找到用户 " + userName;
            return false;
        }
        m_lastError = "修改密码失败: " + query.lastError().text();
        return false;
}

bool MySql::resetUserPassword(const QString &userName)
{
    std::lock_guard<std::recursive_mutex> locker(s_dbMutex);
    return updateUserPassword(userName, "123456");
}

bool MySql::deleteUser(const QString &userName)
{
    std::lock_guard<std::recursive_mutex> locker(s_dbMutex);
    if (userName.isEmpty()) {
            m_lastError = "用户名不能为空";
            return false;
        }

        QSqlQuery fkOff;
        fkOff.exec("PRAGMA foreign_keys = OFF");

        QSqlDatabase db = QSqlDatabase::database();
        if (!db.transaction()) {
            QSqlQuery fkOn;
            fkOn.exec("PRAGMA foreign_keys = ON");
            m_lastError = "开启事务失败: " + db.lastError().text();
            qDebug() << m_lastError;
            return false;
        }

        QString errMsg;
        bool success = false;

        do {
            QSqlQuery userQuery;
            userQuery.prepare("SELECT name, card FROM \"user\" WHERE name = ?");
            userQuery.addBindValue(userName);
            if (!userQuery.exec()) {
                errMsg = "查询用户失败: " + userQuery.lastError().text();
                qDebug() << errMsg;
                break;
            }
            if (!userQuery.next()) {
                errMsg = "未找到用户 " + userName;
                break;
            }
            QString actualName = userQuery.value(0).toString();
            QString cardNumber = userQuery.value(1).toString();

            QList<int> cardIds;
            QSqlQuery cardIdQuery;
            cardIdQuery.prepare("SELECT card_id FROM cards WHERE holder_user_name = ?");
            cardIdQuery.addBindValue(actualName);
            if (!cardIdQuery.exec()) {
                errMsg = "查询card_id失败: " + cardIdQuery.lastError().text();
                qDebug() << errMsg;
                break;
            }
            while (cardIdQuery.next()) {
                cardIds.append(cardIdQuery.value(0).toInt());
            }

            foreach (int cid, cardIds) {
                QSqlQuery q1;
                q1.prepare("DELETE FROM consumption_records WHERE card_id = ?");
                q1.addBindValue(cid);
                if (!q1.exec()) { qDebug() << "删消费记录失败" << cid << q1.lastError().text(); }

                QSqlQuery q2;
                q2.prepare("DELETE FROM topup_records WHERE card_id = ?");
                q2.addBindValue(cid);
                if (!q2.exec()) { qDebug() << "删充值记录失败" << cid << q2.lastError().text(); }

                QSqlQuery q3;
                q3.prepare("DELETE FROM sign_in_records WHERE card_id = ?");
                q3.addBindValue(cid);
                if (!q3.exec()) { qDebug() << "删考勤记录失败" << cid << q3.lastError().text(); }
            }

            QSqlQuery qCards;
            qCards.prepare("DELETE FROM cards WHERE holder_user_name = ?");
            qCards.addBindValue(actualName);
            if (!qCards.exec()) { qDebug() << "删cards失败" << qCards.lastError().text(); }

            if (!cardNumber.isEmpty()) {
                QSqlQuery qSign;
                qSign.prepare("DELETE FROM sign_in_records WHERE name = ?");
                qSign.addBindValue(actualName);
                if (!qSign.exec()) { qDebug() << "删考勤(name)失败" << qSign.lastError().text(); }

                QSqlQuery qCons;
                qCons.prepare("DELETE FROM consumption_records WHERE name = ?");
                qCons.addBindValue(actualName);
                if (!qCons.exec()) { qDebug() << "删消费(name)失败" << qCons.lastError().text(); }

                QSqlQuery qTop;
                qTop.prepare("DELETE FROM topup_records WHERE name = ?");
                qTop.addBindValue(actualName);
                if (!qTop.exec()) { qDebug() << "删充值(name)失败" << qTop.lastError().text(); }
            }

            QSqlQuery deleteUserQuery;
            deleteUserQuery.prepare("DELETE FROM \"user\" WHERE name = ?");
            deleteUserQuery.addBindValue(actualName);
            if (deleteUserQuery.exec()) {
                if (deleteUserQuery.numRowsAffected() > 0) {
                    if (!db.commit()) {
                        errMsg = "事务提交失败: " + db.lastError().text();
                        qDebug() << errMsg;
                        break;
                    }
                    m_lastError.clear();
                    success = true;
                } else {
                    errMsg = "未找到用户 " + actualName;
                }
            } else {
                errMsg = "删除用户失败: " + deleteUserQuery.lastError().text();
                qDebug() << errMsg;
            }
        } while (false);

        if (!success) {
            db.rollback();
            if (errMsg.isEmpty()) errMsg = "删除用户失败";
            m_lastError = errMsg;
        }

        QSqlQuery fkOn;
        fkOn.exec("PRAGMA foreign_keys = ON");
        return success;
}

// ===== 消费终端管理 =====

QList<TerminalInfo> MySql::getAllTerminals()
{
    std::lock_guard<std::recursive_mutex> locker(s_dbMutex);
    QList<TerminalInfo> list;
    QSqlQuery query("SELECT id, name, status, create_time FROM consume_terminals ORDER BY id ASC");
    while (query.next()) {
        TerminalInfo info;
        info.id = query.value(0).toInt();
        info.name = query.value(1).toString();
        info.status = query.value(2).toString();
        info.createTime = query.value(3).toString();
        list.append(info);
    }
    return list;
}

QList<TerminalInfo> MySql::getActiveTerminals()
{
    std::lock_guard<std::recursive_mutex> locker(s_dbMutex);
    QList<TerminalInfo> list;
    QSqlQuery query;
    query.prepare("SELECT id, name, status, create_time FROM consume_terminals WHERE status = '正常' ORDER BY id ASC");
    if (query.exec()) {
        while (query.next()) {
            TerminalInfo info;
            info.id = query.value(0).toInt();
            info.name = query.value(1).toString();
            info.status = query.value(2).toString();
            info.createTime = query.value(3).toString();
            list.append(info);
        }
    }
    return list;
}

bool MySql::addTerminal(const QString &name)
{
    std::lock_guard<std::recursive_mutex> locker(s_dbMutex);
    if (name.trimmed().isEmpty()) {
        m_lastError = "终端名称不能为空";
        return false;
    }
    QSqlQuery query;
    query.prepare("INSERT INTO consume_terminals(name, status, create_time) VALUES(?, '正常', ?)");
    query.addBindValue(name.trimmed());
    query.addBindValue(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    if (query.exec()) {
        m_lastError.clear();
        return true;
    }
    m_lastError = "新增失败：" + query.lastError().text();
    if (m_lastError.contains("UNIQUE", Qt::CaseInsensitive)) {
        m_lastError = "已存在同名终端，请更换名称";
    }
    return false;
}

bool MySql::renameTerminal(int id, const QString &name)
{
    std::lock_guard<std::recursive_mutex> locker(s_dbMutex);
    if (id <= 0 || name.trimmed().isEmpty()) {
        m_lastError = "参数错误：ID或名称非法";
        return false;
    }
    QSqlQuery query;
    query.prepare("UPDATE consume_terminals SET name = ? WHERE id = ?");
    query.addBindValue(name.trimmed());
    query.addBindValue(id);
    if (query.exec()) {
        if (query.numRowsAffected() > 0) {
            m_lastError.clear();
            return true;
        }
        m_lastError = "未找到对应终端";
        return false;
    }
    m_lastError = "重命名失败：" + query.lastError().text();
    if (m_lastError.contains("UNIQUE", Qt::CaseInsensitive)) {
        m_lastError = "已存在同名终端，请更换名称";
    }
    return false;
}

bool MySql::setTerminalStatus(int id, const QString &status)
{
    std::lock_guard<std::recursive_mutex> locker(s_dbMutex);
    if (id <= 0 || (status != "正常" && status != "停用")) {
        m_lastError = "参数错误";
        return false;
    }
    QSqlQuery query;
    query.prepare("UPDATE consume_terminals SET status = ? WHERE id = ?");
    query.addBindValue(status);
    query.addBindValue(id);
    if (query.exec()) {
        if (query.numRowsAffected() > 0) {
            m_lastError.clear();
            return true;
        }
        m_lastError = "未找到对应终端";
        return false;
    }
    m_lastError = "状态更新失败：" + query.lastError().text();
    return false;
}

QString MySql::getTerminalName(int id)
{
    std::lock_guard<std::recursive_mutex> locker(s_dbMutex);
    if (id <= 0) return QString();
    QSqlQuery query;
    query.prepare("SELECT name FROM consume_terminals WHERE id = ?");
    query.addBindValue(id);
    if (query.exec() && query.next()) {
        return query.value(0).toString();
    }
    return QString();
}

DailySummary MySql::getDailySummary(int terminalId, const QString &date)
{
    std::lock_guard<std::recursive_mutex> locker(s_dbMutex);
    DailySummary summary;
    summary.terminalName = getTerminalName(terminalId);
    if (summary.terminalName.isEmpty() && terminalId > 0) {
        summary.terminalName = QString("终端#%1").arg(terminalId);
    } else if (terminalId <= 0) {
        summary.terminalName = "未指定终端";
    }
    summary.date = date;
    summary.totalCount = 0;
    summary.totalAmount = 0.0;

    QString likeDate = date + "%";
    QSqlQuery query;
    query.prepare("SELECT COUNT(*), COALESCE(SUM(amount),0) "
                  "FROM consumption_records "
                  "WHERE terminal_id = ? AND create_time LIKE ?");
    query.addBindValue(terminalId <= 0 ? 0 : terminalId);
    query.addBindValue(likeDate);
    if (query.exec() && query.next()) {
        summary.totalCount = query.value(0).toInt();
        summary.totalAmount = query.value(1).toDouble();
    }
    return summary;
}

bool MySql::addFaceFeature(const QString &userName, const std::vector<float> &feature)
{
    std::lock_guard<std::recursive_mutex> locker(s_dbMutex);
    QSqlQuery query;
    query.prepare("INSERT OR REPLACE INTO face_features (user_name, feature) VALUES (?, ?)");
    query.bindValue(0, userName);
    QByteArray blob(reinterpret_cast<const char*>(feature.data()),
                    feature.size() * sizeof(float));
    query.bindValue(1, blob);
    if (!query.exec()) {
        m_lastError = "保存人脸特征失败: " + query.lastError().text();
        qDebug() << m_lastError;
        return false;
    }
    m_lastError.clear();
    return true;
}

bool MySql::getAllFaceFeatures(std::vector<std::tuple<QString, QString, std::vector<float>>> &users)
{
    std::lock_guard<std::recursive_mutex> locker(s_dbMutex);
    QSqlQuery query;
    QString sql = "SELECT u.card, ff.user_name, ff.feature "
                  "FROM face_features ff JOIN \"user\" u ON ff.user_name = u.name";
    if (!query.exec(sql)) {
        m_lastError = "获取人脸特征失败: " + query.lastError().text();
        qDebug() << m_lastError;
        return false;
    }
    users.clear();
    while (query.next()) {
        QString cardNumber = query.value(0).toString();
        QString userName = query.value(1).toString();
        QByteArray blob = query.value(2).toByteArray();
        std::vector<float> feature(blob.size() / sizeof(float));
        if (!feature.empty()) {
            std::memcpy(feature.data(), blob.data(), blob.size());
        }
        users.emplace_back(cardNumber, userName, feature);
    }
    m_lastError.clear();
    return true;
}

bool MySql::hasFaceFeature(const QString &userName)
{
    std::lock_guard<std::recursive_mutex> locker(s_dbMutex);
    QSqlQuery query;
    query.prepare("SELECT 1 FROM face_features WHERE user_name = ?");
    query.bindValue(0, userName);
    if (query.exec() && query.next()) {
        return true;
    }
    return false;
}

bool MySql::updateSignRecord(int recordId, const QString &type, const QString &signTime, const QString &remark)
{
    std::lock_guard<std::recursive_mutex> locker(s_dbMutex);
    if (recordId <= 0) {
        m_lastError = "无效的记录ID";
        return false;
    }
    if (type != "签到" && type != "签退") {
        m_lastError = "类型必须是'签到'或'签退'";
        return false;
    }
    QSqlQuery query;
    query.prepare("UPDATE sign_in_records SET type = ?, sign_time = ?, remark = ? WHERE id = ?");
    query.addBindValue(type);
    query.addBindValue(signTime);
    query.addBindValue(remark);
    query.addBindValue(recordId);
    if (query.exec()) {
        if (query.numRowsAffected() > 0) {
            m_lastError.clear();
            return true;
        }
        m_lastError = "未找到对应记录";
        return false;
    }
    m_lastError = "更新签到记录失败: " + query.lastError().text();
    return false;
}

bool MySql::deleteSignRecord(int recordId)
{
    std::lock_guard<std::recursive_mutex> locker(s_dbMutex);
    if (recordId <= 0) {
        m_lastError = "无效的记录ID";
        return false;
    }
    QSqlQuery query;
    query.prepare("DELETE FROM sign_in_records WHERE id = ?");
    query.addBindValue(recordId);
    if (query.exec()) {
        if (query.numRowsAffected() > 0) {
            m_lastError.clear();
            return true;
        }
        m_lastError = "未找到对应记录";
        return false;
    }
    m_lastError = "删除签到记录失败: " + query.lastError().text();
    return false;
}

QList<QMap<QString, QString>> MySql::getSignRecordsByDate(const QString &date)
{
    std::lock_guard<std::recursive_mutex> locker(s_dbMutex);
    QList<QMap<QString, QString>> list;
    QSqlQuery query;
    query.prepare("SELECT id, card_id, name, sign_time, type, COALESCE(remark,'') FROM sign_in_records WHERE sign_date = ? ORDER BY sign_time DESC");
    query.addBindValue(date);
    if (query.exec()) {
        while (query.next()) {
            QMap<QString, QString> record;
            record["id"] = query.value(0).toString();
            record["cardId"] = query.value(1).toString();
            record["name"] = query.value(2).toString();
            record["time"] = query.value(3).toString();
            record["type"] = query.value(4).toString();
            record["remark"] = query.value(5).toString();
            list.append(record);
        }
    }
    return list;
}
