#include "databasetext.h"

#include <QFileInfo>
#include <QDir>

DataBaseText *dbtext=NULL;

DataBaseTextUser::DataBaseTextUser(QObject *parent ):QObject(parent)
{
    if(dbtext==NULL)dbtext=new DataBaseText(parent);
    refcounter=0;
    map.clear();
}

DataBaseTextUser::~DataBaseTextUser()
{
    clear();
}

void DataBaseTextUser::clear()
{
    QList<DBase*> list=map.values();
    for(int i=0;i<list.size();i++)
    {
        delete list[i];
    }
    map.clear();
}

DBase* DataBaseTextUser::getuserdata(int ref)
{
    int cnt=map.count(ref);
    if(cnt==0)return NULL;
     else if(cnt>1)
     {
        qDebug()<<"Error more than one entry in DataBaseTextUser map";
        map.clear();
        return NULL;
     }
    DBase *obj = map.value(ref);
    map.remove(ref);
    return obj;
}

void DataBaseTextUser::request(const QString &dirname, const QString &AEStext,DBase* userdata)
{
    if(userdata==NULL)
    {
        dbtext->asyncDbLookupFromAES(dirname,AEStext,-1,this,"result");
        return;
    }
    if(refcounter<0)refcounter=0;
    refcounter++;refcounter%=DataBaseTextUser_MAX_PACKETS_IN_QUEUE;
    DBase *obj;
    while( (obj=map.take(refcounter))!=NULL )delete obj;
    map.insert(refcounter,userdata);
    emit dbtext->asyncDbLookupFromAES(dirname,AEStext,refcounter,this,"result");
}

//------------

DataBaseText::DataBaseText(QObject *parent) : QObject(parent)
{
    DataBaseWorkerText *worker = new DataBaseWorkerText;
    worker->moveToThread(&workerThread);
    connect(&workerThread, &QThread::finished, worker, &QObject::deleteLater);
    connect(this, &DataBaseText::asyncDbLookupFromAES, worker, &DataBaseWorkerText::DbLookupFromAES);
    workerThread.start();
}

DataBaseText::~DataBaseText()
{
    workerThread.quit();
    workerThread.wait();
}

//---------------

//from here on main worker. WARING not in main thread.

void DataBaseWorkerText::DbLookupFromAES(const QString &dirname, const QString &AEStext,int userdata,QObject *sender,const char * member)
{

    QStringList values;

    if(cache.maxCost()<300)cache.setMaxCost(300);

    //serve cached entries
    QStringList *pvalues=cache.object(AEStext);
    if(pvalues)
    {
        QStringList values=*pvalues;
        if(!values.size())
        {
            values.push_back("Not found");
            QMetaObject::invokeMethod(sender,member, Qt::QueuedConnection,Q_ARG(bool, false),Q_ARG(int, userdata),Q_ARG(const QStringList&, values));
            return;
        }
        QMetaObject::invokeMethod(sender,member, Qt::QueuedConnection,Q_ARG(bool, true),Q_ARG(int, userdata),Q_ARG(const QStringList&, values));
        return;
    }

    //open db

    if(!db.isOpen())
    {
        //get a file that kind of matches, ignore case and add a bit of wildcards
        QDir dir(dirname);
        QStringList files=dir.entryList(QStringList()<<"basestation*.sqb",QDir::Files | QDir::Readable | QDir::NoDotAndDotDot | QDir::NoDot);
        QFile file;
        if(files.size())file.setFileName(dirname+"/"+files[0]);
        if(!files.size()||!file.exists())
        {
            values.push_back("Database file basestation.sqb missing. from "+dirname);
            QMetaObject::invokeMethod(sender,member, Qt::QueuedConnection,Q_ARG(bool, false),Q_ARG(int, userdata),Q_ARG(const QStringList&, values));
            return;
        }
        //try and open the database
        db = QSqlDatabase::addDatabase("QSQLITE");
        db.setDatabaseName(file.fileName());
        if (!db.open())
        {
            values.push_back("Cant open SQL database");
            QMetaObject::invokeMethod(sender,member, Qt::QueuedConnection,Q_ARG(bool, false),Q_ARG(int, userdata),Q_ARG(const QStringList&, values));
            return;
        }
        qDebug()<<"db opened ("<<db.databaseName()<<")";

    }

    //req from db
    QSqlQuery query;
    bool bStatus = false;
    uint aes=AEStext.toUInt(&bStatus,16);
    QString aes_as_hex_uppercase_and_6_places = QString("%1").arg(aes, 6, 16, QLatin1Char( '0' )).toUpper();//in case aes is less than 6 places
    QString req = "SELECT * FROM Aircraft WHERE ModeS LIKE '"+aes_as_hex_uppercase_and_6_places+"';";

    if(!query.exec(req))
    {
        values.push_back("Error: "+query.lastError().text());
        QMetaObject::invokeMethod(sender,member, Qt::QueuedConnection,Q_ARG(bool, false),Q_ARG(int, userdata),Q_ARG(const QStringList&, values));
        return;
    }

    if(!query.next())
    {
        //not found
        cache.insert(AEStext,new QStringList);
        values.clear();values.push_back("Not found");
        QMetaObject::invokeMethod(sender,member, Qt::QueuedConnection,Q_ARG(bool, false),Q_ARG(int, userdata),Q_ARG(const QStringList&, values));
        return;
    }

    //fill in the local shema DataBaseTextUser::DataBaseSchema
    QSqlRecord rec = query.record();
    QMetaEnum en = QMetaEnum::fromType<DataBaseTextUser::DataBaseSchema>();
    for(int k=0;k<en.keyCount();k++)
    {
        values.push_back(rec.value(en.key(k)).toString().trimmed());
    }

    cache.insert(AEStext,new QStringList(values));

    QMetaObject::invokeMethod(sender,member, Qt::QueuedConnection,Q_ARG(bool, true),Q_ARG(int, userdata),Q_ARG(const QStringList&, values));
    return;

}

