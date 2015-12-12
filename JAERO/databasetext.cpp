#include "databasetext.h"

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
    dbtext->asyncDbLookupFromAES(dirname,AEStext,refcounter,this,"result");
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

//main worker. WARING not in main thread.
void DataBaseWorkerText::DbLookupFromAES(const QString &dirname, const QString &AEStext,int userdata,QObject *sender,const char * member)
{

    QStringList values;

    if(cache.maxCost()<300)cache.setMaxCost(300);

    QFile filenew(dirname+"/new.aircrafts_dump.csv");
    if(filenew.exists())
    {
        QFile oldfile(dirname+"/aircrafts_dump.csv");
        oldfile.setPermissions(QFile::ReadOther | QFile::WriteOther);
        oldfile.remove();
        filenew.rename(dirname+"/aircrafts_dump.csv");
        cache.clear();
        qDebug()<<"using new db";
    }

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

    QFile file(dirname+"/aircrafts_dump.csv");
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        values.push_back("Cant open file");
        QMetaObject::invokeMethod(sender,member, Qt::QueuedConnection,Q_ARG(bool, false),Q_ARG(int, userdata),Q_ARG(const QStringList&, values));
        return;
    }

    while (!file.atEnd())
    {
        QString line = file.readLine().trimmed();
        while((line[line.size()-1]=='\\')&&(!file.atEnd()))line+=file.readLine().trimmed();
        line.replace("\\N,","\"\",");
        values=line.split("\",\"");
        if(values.size()!=9)
        {
            values.clear();values.push_back("Database illformed");
            QMetaObject::invokeMethod(sender,member, Qt::QueuedConnection,Q_ARG(bool, false),Q_ARG(int, userdata),Q_ARG(const QStringList&, values));
            return;
        }
        values[0].remove(0,1);
        values[values.size()-1].chop(1);
        if(values[0]==AEStext)
        {
            cache.insert(AEStext,new QStringList(values));
            QMetaObject::invokeMethod(sender,member, Qt::QueuedConnection,Q_ARG(bool, true),Q_ARG(int, userdata),Q_ARG(const QStringList&, values));
            return;
        }
    }

    cache.insert(AEStext,new QStringList);
    values.clear();values.push_back("Not found");
    QMetaObject::invokeMethod(sender,member, Qt::QueuedConnection,Q_ARG(bool, false),Q_ARG(int, userdata),Q_ARG(const QStringList&, values));

}



