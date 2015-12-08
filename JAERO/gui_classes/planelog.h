#ifndef PLANELOG_H
#define PLANELOG_H

#include <QMainWindow>
#include <QThread>
#include "../aerol.h"
#include <QTextCharFormat>
#include <QFile>
#include <QTextStream>
#include <QCache>

namespace Ui {
class PlaneLog;
}

class DbWorker : public QObject
{
    Q_OBJECT
public slots:
    void DbLookupFromAES(const QString &dirname,const QString &AEStext)
    {
        // ... here is the expensive or blocking operation ...

        if(cache.maxCost()<300)cache.setMaxCost(300);

        QStringList *pvalues=cache.object(AEStext);
        if(pvalues)
        {
            QStringList values=*pvalues;
            emit resultReady(values);
            emit finished();
            return;
        }
//        int k=1;

        QFile file(dirname+"/aircrafts_dump.csv");
        if(!file.open(QIODevice::ReadOnly | QIODevice::Text)){emit error("Cant open file");emit finished();return;}
        while (!file.atEnd())
        {
            QString line = file.readLine().trimmed();
            while((line[line.size()-1]=='\\')&&(!file.atEnd()))line+=file.readLine().trimmed();
            line.replace("\\N,","\"\",");
            QStringList values=line.split("\",\"");
//            k++;
            if(values.size()!=9)
            {
//                qDebug()<<k<<values<<values.size();
                emit error("Database illformed");
                emit finished();
                return;
            }
            values[0].remove(0,1);
            values[values.size()-1].chop(1);
            if(values[0]==AEStext)
            {
                cache.insert(AEStext,new QStringList(values));
                emit resultReady(values);
                emit finished();
                return;
            }
        }

        emit error("Not found");
        emit finished();

    }
signals:
    void resultReady(const QStringList &result);
    void error(const QString &text);
    void finished();
private:
    QCache<QString, QStringList> cache;//not sure if this is right, thread problems?
};
class DbLookupController : public QObject
{
    Q_OBJECT
    QThread workerThread;
    bool busy;
    bool havepeding;
    QString pedingdirname;
    QString pedingAEStext;
public:
    DbLookupController(QObject *parent = 0):QObject(parent)
    {
        havepeding=false;
        busy=false;
        DbWorker *worker = new DbWorker;
        worker->moveToThread(&workerThread);
        connect(&workerThread, &QThread::finished, worker, &QObject::deleteLater);
        connect(this, &DbLookupController::asyncDbLookupFromAES_worker, worker, &DbWorker::DbLookupFromAES);
        connect(worker, &DbWorker::resultReady, this, &DbLookupController::result);
        connect(worker, &DbWorker::error, this, &DbLookupController::error);
        connect(worker, SIGNAL(finished()), this, SLOT(finished_worker()));
        connect(this,SIGNAL(asyncDbLookupFromAES(QString,QString)),this,SLOT(asyncDbLookupFromAESslot(QString,QString)));
        workerThread.start();
    }
    ~DbLookupController()
    {
        workerThread.quit();
        workerThread.wait();
    }
public slots:

signals:
    void asyncDbLookupFromAES(const QString &dirname, const QString &AEStext);//compressed
    void asyncDbLookupFromAES_worker(const QString &dirname, const QString &AEStext);//non compressed
    void result(const QStringList &);
    void error(const QString &text);
private slots:
    void asyncDbLookupFromAESslot(const QString &dirname, const QString &AEStext)
    {
        if(busy)
        {
            havepeding=true;
            pedingdirname=dirname;
            pedingAEStext=AEStext;
            return;
        }
        emit asyncDbLookupFromAES_worker(dirname,AEStext);
        busy=true;
    }
    void finished_worker()
    {
        busy=false;
        if(havepeding)
        {
            emit asyncDbLookupFromAES_worker(pedingdirname,pedingAEStext);
            busy=true;
            havepeding=false;
        }
    }
};


class ImageWorker : public QObject
{
    Q_OBJECT
public slots:
    void ImageLookupFromAES(const QString &dirname,const QString &AEStext)
    {
        QPixmap result;
        // ... here is the expensive or blocking operation ...

        if((!result.load(dirname+"/"+AEStext+".png"))&&(!result.load(dirname+"/"+AEStext+".jpg")))result.load(":/images/Plane_clip_art.svg");

        emit resultReady(result);
    }
signals:
    void resultReady(const QPixmap &result);
};
class ImageController : public QObject
{
    Q_OBJECT
    QThread workerThread;
public:
    ImageController(QObject *parent = 0):QObject(parent)
    {
        ImageWorker *worker = new ImageWorker;
        worker->moveToThread(&workerThread);
        connect(&workerThread, &QThread::finished, worker, &QObject::deleteLater);
        connect(this, &ImageController::asyncImageLookupFromAES, worker, &ImageWorker::ImageLookupFromAES);
        connect(worker, &ImageWorker::resultReady, this, &ImageController::result);
        workerThread.start();
    }
    ~ImageController()
    {
        workerThread.quit();
        workerThread.wait();
    }
public slots:
signals:
    void asyncImageLookupFromAES(const QString &dirname, const QString &AEStext);
    void result(const QPixmap &);
};



class PlaneLog : public QWidget
{
    Q_OBJECT

public:
    explicit PlaneLog(QWidget *parent = 0);
    ~PlaneLog();
    QString planesfolder;
    QString planelookup;
protected:
    void closeEvent(QCloseEvent *event);
    void showEvent(QShowEvent *event);
    void contextMenuEvent(QContextMenuEvent *event) Q_DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
public slots:
    void ACARSslot(ACARSItem &acarsitem);

private slots:

    void updatescrollbar();

    void on_actionClear_triggered();

    void on_actionUpDown_triggered();

    void on_actionLeftRight_triggered();

    void on_actionStopSorting_triggered();

    void on_tableWidget_currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn);

    void on_toolButtonimg_clicked();

    void on_actionCopy_triggered();

    void plainTextEditnotesChanged();

    void imageUpdateslot(const QPixmap &image);

    void dbUpdateslot(const QStringList &dbitem);

    void messagesliderchsnged(int value);

private:
    Ui::PlaneLog *ui;
    int wantedheightofrow;
    QToolBar * toolBar;
    void updateinfopain(int row);
    QList<int> savedsplitter2;

    int updateinfoplanrow;

    ImageController *ic;
    DbLookupController *dbc;

    double wantedscrollprop;


};

#endif // PLANELOG_H
