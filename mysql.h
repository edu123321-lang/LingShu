#ifndef MYSQL_H
#define MYSQL_H

#include <QObject>
#include <mutex>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QMap>
#include <vector>
#include <tuple>
#include <cstring>

//公告的结构体
struct NoticeInfo {
    int id;
    QString title;
    QString content;
    QString filePath;
    QString createTime;
    QString updateTime;
    bool isVisible;
};

//消费终端结构体
struct TerminalInfo {
    int id;
    QString name;
    QString status;      // "正常" / "停用"
    QString createTime;
};

//日结汇总结构体
struct DailySummary {
    QString terminalName;
    QString date;
    int totalCount;
    double totalAmount;
};

class MySql : public QObject
{
    Q_OBJECT
private:
    explicit MySql(QObject *parent = nullptr);
    //初始化一个数据库
    MySql(QString dbName,QObject *parent = nullptr);
    ~MySql();
    //定义一个静态成员指针
    static MySql * const p;
    QString m_lastError;         // 新增：缓存错误信息
    // 全局数据库访问互斥锁：Qt 的 QSqlDatabase 默认连接禁止跨线程并发访问，
    // 刷脸识别的后台线程（QtConcurrent）与主线程会同时查询，必须串行化
    static std::recursive_mutex s_dbMutex;   // 可重入：方法之间互相调用（如 getWorkSignInDeadline->getSystemSetting）时避免同线程死锁
public:
    void createTable();
    void insertData(QString name,QString pwd,QString age,QString card,QString sex);
    //获取静态成员函数唯一的地址
    static MySql *getMySql(void);
    //查询管理员的用户名和密码
    bool adminIsExits(QString name,QString pwd);
    //查询用户的用户名和密码
    bool userIsExits(QString name, QString pwd);

    QList<NoticeInfo> getAllNotices();// 获取所有可见公告（按时间倒序)
    NoticeInfo getNoticeById(int id);
    bool addNotice(const NoticeInfo &info);
    bool updateNotice(const NoticeInfo &info);
    bool deleteNotice(int id);// 软删除（设置 is_visible=0）

    int getCardIdByCardNumber(const QString &cardNumber);      // 根据卡号字符串获取 cards.card_id
    QString getUserNameByCardNumber(const QString &cardNumber); // 根据卡号获取用户名
    bool addSignRecord(int cardId, const QString &name, const QString &type, const QString &remark = ""); // 插入签到/签退记录，remark如"迟到"/"早退"
    QString getLastSignType(int cardId, const QString &date);   // 获取某卡某天最后一次记录的类型
    QList<QMap<QString, QString>> getTodaySignRecords();       // 获取今日所有签到记录（管理员用）
    QList<QMap<QString, QString>> getUserSignRecords(const QString &name); // 获取某用户所有记录
    bool updateSignRecord(int recordId, const QString &type, const QString &signTime, const QString &remark); // 修改签到记录
    bool deleteSignRecord(int recordId);                       // 删除签到记录
    QList<QMap<QString, QString>> getSignRecordsByDate(const QString &date); // 按日期获取签到记录（管理员用）

    QString getSystemSetting(const QString &key, const QString &defaultValue = ""); // 读取系统设置
    bool setSystemSetting(const QString &key, const QString &value);                // 写入系统设置
    QString getWorkSignInDeadline();    // 上班签到截止时间，默认"09:00"
    QString getWorkSignOffStart();      // 下班签退允许时间，默认"18:00"
    bool setWorkSignInDeadline(const QString &hhmm);
    bool setWorkSignOffStart(const QString &hhmm);

    QString normalizedCardNumber(const QString &card);


    // 获取所有卡片信息（联合查询 user 和 cards）
    QList<QMap<QString, QString>> getAllCardInfo();

    // 按姓名或卡号模糊搜索
    QList<QMap<QString, QString>> searchCards(const QString &keyword);

    // 获取某张卡的所有交易记录（充值+消费合并，按时间倒序）
    QList<QMap<QString, QString>> getCardTransactions(int cardId);

    // 获取某个用户（通过 cardId）的姓名
    QString getUserNameByCardId(int cardId);

    // 充值
    bool topUp(int cardId, double amount);

    // 扣费（消费），terminalId=-1 表示未指定（兼容旧数据，终端名填"未指定"）
    bool consume(int cardId, double amount, int terminalId = -1);

    // 挂失
    bool reportLoss(int cardId);

    // 补办（更新状态 + 更换卡号）
    bool reissueCard(int cardId, const QString &newCardNumber);

    // 根据用户名获取 card_id
    int getCardIdByUserName(const QString &userName);

    // 根据卡号获取卡片详情（余额、状态）
    QMap<QString, QString> getCardInfoByCardId(int cardId);

    //消费终端管理
    QList<TerminalInfo> getAllTerminals();          // 获取所有终端（含停用）
    QList<TerminalInfo> getActiveTerminals();        // 获取仅正常状态终端（下拉框用）
    bool addTerminal(const QString &name);           // 新增终端
    bool renameTerminal(int id, const QString &name);// 修改终端名称
    bool setTerminalStatus(int id, const QString &status); // 启用/停用：status="正常"或"停用"
    QString getTerminalName(int id);                 // 根据id获取终端名
    DailySummary getDailySummary(int terminalId, const QString &date); // 单终端单日汇总

    //哈希辅助函数
    static QString hashPassword(const QString &password);

     QString lastError() const;   // 新增：获取最后一条错误信息

     QList<QMap<QString, QString>> getAllUsers(const QString &keyword = "");
     bool updateUserPassword(const QString &userName, const QString &newPassword);
     bool resetUserPassword(const QString &userName);
     bool deleteUser(const QString &userName);

     bool addFaceFeature(const QString &userName, const std::vector<float> &feature);
     bool getAllFaceFeatures(std::vector<std::tuple<QString, QString, std::vector<float>>> &users);
     bool hasFaceFeature(const QString &userName);
signals:

public slots:
};

#endif // MYSQL_H
