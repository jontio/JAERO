#ifndef DATABASETEXT_H
#define DATABASETEXT_H

#include <QObject>
#include <QThread>
#include <QFile>
#include <QTextStream>
#include <QCache>
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QMetaEnum>

#define DataBaseTextUser_MAX_PACKETS_IN_QUEUE 100

//Only DataBaseTextUser and DBase should be used directly

class DataBaseWorkerText : public QObject
{
    Q_OBJECT
public:
public slots:
    void DbLookupFromAES(const QString &dirname, const QString &AEStext,int userdata,QObject *sender,const char * member);
signals:
private:
    QCache<QString, QStringList> cache;
    QSqlDatabase db;

};

class DataBaseText : public QObject
{
    Q_OBJECT
    QThread workerThread;
public:
    explicit DataBaseText(QObject *parent = 0);
    ~DataBaseText();
signals:
    void asyncDbLookupFromAES(const QString &dirname, const QString &AEStext,int userdata,QObject *sender,const char * member);
public slots:
private slots:
};

extern DataBaseText *dbtext;

class DBase
{

};
class DataBaseTextUser : public QObject
{
    Q_OBJECT
public:

    enum DataBaseSchema
    {
        ModeS,
        ModeSCountry,
        Registration,
        Manufacturer,
        ICAOTypeCode,
        Type,
        RegisteredOwners
    };
    Q_ENUM(DataBaseSchema)


    DataBaseTextUser(QObject *parent = 0);
    ~DataBaseTextUser();
    void clear();
    DBase* getuserdata(int ref);
public slots:
    void request(const QString &dirname, const QString &AEStext,DBase* userdata);
signals:
    void result(bool ok, int ref, const QStringList &result);
private:
    QMap<int,DBase*> map;
    int refcounter;
private slots:
};




#endif // DATABASETEXT_H
