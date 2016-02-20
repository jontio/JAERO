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
    emit dbtext->asyncDbLookupFromAES(dirname,AEStext,refcounter,this,"result");
}

//------------

DataBaseText::DataBaseText(QObject *parent) : QObject(parent)
{
    DataBaseWorkerText *worker = new DataBaseWorkerText;
    worker->moveToThread(&workerThread);
    connect(&workerThread, &QThread::finished, worker, &QObject::deleteLater);
    connect(this, &DataBaseText::asyncDbLookupFromAES, worker, &DataBaseWorkerText::DbLookupFromAES);
    connect(this, &DataBaseText::importdb, worker, &DataBaseWorkerText::importdb);
    connect(worker, &DataBaseWorkerText::dbimported, this, &DataBaseText::dbimported);
    workerThread.start();
}

DataBaseText::~DataBaseText()
{
    workerThread.quit();
    workerThread.wait();
}

//---------------

//from here on main worker. WARING not in main thread.

bool DataBaseWorkerText::importdb(const QString &dirname)
{

    //clear cache
    cache.clear();

    //if new file then copy over first
    {
        QFile filenew(dirname+"/new.aircrafts_dump.csv");
        if(filenew.exists())
        {
            QFile oldfile(dirname+"/aircrafts_dump.csv");
            oldfile.setPermissions(QFile::ReadOther | QFile::WriteOther);
            oldfile.remove();
            filenew.rename(dirname+"/aircrafts_dump.csv");
        }
    }

    QFile file(dirname+"/aircrafts_dump.csv");
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        file.rename(dirname+"/new.aircrafts_dump.csv");
        emit dbimported(false,"Cant open csv file");
        qDebug()<<"Cant open csv file";
        return false;
    }

    //open db
    if((!db.isOpen())||(db.databaseName()!=(dirname+"/aircrafts_dump.db")))
    {
        if(!db.isOpen())db = QSqlDatabase::addDatabase("QSQLITE");
        db.setDatabaseName(dirname+"/aircrafts_dump.db");
        if (!db.open())
        {
            file.rename(dirname+"/new.aircrafts_dump.csv");
            emit dbimported(false,"Cant open SQL database");
            qDebug()<<"Cant open SQL database";
            return false;
        }
    }

    QSqlQuery query;

    //new format from http://junzisun.com/adb/download as of Feb 2016
    //icao,regid,mdl,type,operator
    //000334,PU-PLS,ULAC,EDRA SUPER PETREL LS,PRIVATE OWNER

    query.exec("drop table main;");
    query.exec("create table main (AES int primary key,REG varchar(8), Model varchar(20), Type varchar(40), Owner varchar(40))");


    QStringList values;
    db.transaction();
    int cc=0;
    int linenumber=0;
    while (!file.atEnd())
    {
        //read line skip header
        linenumber++;
        QString line = file.readLine().trimmed();
        line.remove('\r');
        line.remove('\n');
        values=line.split(",");
        while((values.size()<5)&&(!file.atEnd()))//have cr/lf in entry?
        {
            line+=file.readLine().trimmed();
            line.remove('\r');
            line.remove('\n');
            values=line.split(",");
        }
        if(linenumber==1)continue;//header?
        if(values.size()>5)//have comas in entries?
        {
            for(int i=0;i<values.size()-1;i++)
            {
                if(((QString)values[i]).isEmpty())continue;
                while(i<values.size()-1)
                {
                   if(((QString)values[i]).left(1)!="\"")break;
                   if(((QString)values[i+1]).isEmpty())
                   {
                        values.removeAt(i+1);
                        continue;
                   }
                   if((((QString)values[i+1]).right(1)!="\""))
                   {
                       values[i]=values[i]+","+values[i+1];
                       values.removeAt(i+1);
                       continue;
                   }
                   values[i]=((QString)values[i]).right(((QString)values[i]).size()-1)+","+((QString)values[i+1]).left(((QString)values[i+1]).size()-1);
                   values.removeAt(i+1);
                   break;
                }
            }
        }
        if(values.size()!=5)
        {
            db.rollback();
            file.rename(dirname+"/new.aircrafts_dump.csv");
            emit dbimported(false,"CSV file illformed");
            qDebug()<<values;
            qDebug()<<"CSV file illformed cols!=5, cols="<<values.size();
            return false;
        }
        bool bStatus = false;
        uint aes=values[0].toUInt(&bStatus,16);
        if(!bStatus)
        {
            db.rollback();
            file.rename(dirname+"/new.aircrafts_dump.csv");
            emit dbimported(false,"CSV entry error");
            qDebug()<<"CSV entry error";
            return false;
        }

        //insert line
        QString req = "INSERT INTO main VALUES(";
        req+=QString::number(aes)+",'";
        for(int i=1; i<values.length ();++i)
        {
            values[i].replace("'","''");//escape '
            req.append(values.at(i));
            req.append("','");
        }
        req.chop(2);
        req.append(");");
        if(!query.exec(req)&&cc<10)
        {
            cc++;
            if(cc<10)
            {
                qDebug()<<req;
                qDebug()<<"Error: "+query.lastError().text();
            }else qDebug()<<"Suppressing more errors";
        }

    }
    db.commit();


    //Old format
    //eg "006011","C9-BAP","B735","83d52a4","LAM144","TM144","Boeing 737-53S","LAM Mozambique Airlines","1449857095"
/*    query.exec("drop table main;");
    query.exec("create table main (AES int primary key,REG varchar(8), Model varchar(20),unknown varchar(20), Call_Sign  varchar(20), Flight  varchar(20), Type varchar(40), Owner varchar(40), Updated  varchar(20))");


    QStringList values;
    db.transaction();
    int cc=0;
    while (!file.atEnd())
    {

        //read line
        QString line = file.readLine().trimmed();
        while((line[line.size()-1]=='\\')&&(!file.atEnd()))line+=file.readLine().trimmed();
        line.replace("\\N,","\"\",");
        values=line.split("\",\"");
        if(values.size()!=9)
        {
            db.rollback();
            file.rename(dirname+"/new.aircrafts_dump.csv");
            emit dbimported(false,"CSV file illformed");
            return false;
        }
        values[0].remove(0,1);
        values[values.size()-1].chop(1);
        bool bStatus = false;
        uint aes=values[0].toUInt(&bStatus,16);
        if(!bStatus)
        {
            db.rollback();
            file.rename(dirname+"/new.aircrafts_dump.csv");
            emit dbimported(false,"CSV entry error");
            return false;
        }

        //insert line
        QString req = "INSERT INTO main VALUES(";
        req+=QString::number(aes)+",'";
        for(int i=1; i<values.length ();++i)
        {
            values[i].replace("'","''");//escape '
            req.append(values.at(i));
            req.append("','");
        }
        req.chop(2);
        req.append(");");
        if(!query.exec(req)&&cc<10)
        {
            cc++;
            if(cc<10)
            {
                qDebug()<<req;
                qDebug()<<"Error: "+query.lastError().text();
            }else qDebug()<<"Suppressing more errors";
        }

    }
    db.commit();*/

    qDebug()<<"new db imported";

    emit dbimported(true,"Done");
    return true;
}

void DataBaseWorkerText::DbLookupFromAES(const QString &dirname, const QString &AEStext,int userdata,QObject *sender,const char * member)
{

    QStringList values;

    if(cache.maxCost()<300)cache.setMaxCost(300);

    //import if new csv file
    QFile filenew(dirname+"/new.aircrafts_dump.csv");
    if(filenew.exists())
    {
        if(!importdb(dirname))
        {
            values.push_back("Cant import CSV file into Database");
            QMetaObject::invokeMethod(sender,member, Qt::QueuedConnection,Q_ARG(bool, false),Q_ARG(int, userdata),Q_ARG(const QStringList&, values));
            return;
        }
    }

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
    if((!db.isOpen())||(db.databaseName()!=(dirname+"/aircrafts_dump.db")))
    {

        QFile file(dirname+"/aircrafts_dump.db");
        if(!file.exists())
        {
            values.push_back("Database file aircrafts_dump.db missing. Download first.");
            QMetaObject::invokeMethod(sender,member, Qt::QueuedConnection,Q_ARG(bool, false),Q_ARG(int, userdata),Q_ARG(const QStringList&, values));
            return;
        }

        if(!db.isOpen())db = QSqlDatabase::addDatabase("QSQLITE");
        db.setDatabaseName(dirname+"/aircrafts_dump.db");
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
    QString req = "SELECT * FROM main WHERE AES = "+QString::number(aes)+";";
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
        values.clear();values.push_back("Not found5");
        QMetaObject::invokeMethod(sender,member, Qt::QueuedConnection,Q_ARG(bool, false),Q_ARG(int, userdata),Q_ARG(const QStringList&, values));
        return;
    }

    QSqlRecord rec = query.record();
    values.push_back(AEStext);
    for(int i=1;i<rec.count();i++)
    {
        values.push_back(rec.value(i).toString());
    }
    cache.insert(AEStext,new QStringList(values));
    QMetaObject::invokeMethod(sender,member, Qt::QueuedConnection,Q_ARG(bool, true),Q_ARG(int, userdata),Q_ARG(const QStringList&, values));
    return;


}

