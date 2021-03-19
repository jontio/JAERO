#ifndef PLANELOG_H
#define PLANELOG_H

#include <QMainWindow>
#include <QThread>
#include "../aerol.h"
#include <QTextCharFormat>
#include <QFile>
#include <QTextStream>
#include <QCache>
#include <stdio.h>
#include "../databasetext.h"
#include <QTableWidgetItem>
#include "arincparse.h"

namespace Ui {
class PlaneLog;
}

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
        emit finished();
    }
signals:
    void resultReady(const QPixmap &result);
    void finished();
};
class ImageController : public QObject
{
    Q_OBJECT
    QThread workerThread;
    bool busy;
    bool havepeding;
    QString pedingdirname;
    QString pedingAEStext;
public:
    ImageController(QObject *parent = 0):QObject(parent)
    {
        ImageWorker *worker = new ImageWorker;
        worker->moveToThread(&workerThread);
        connect(&workerThread, &QThread::finished, worker, &QObject::deleteLater);
        connect(this, &ImageController::asyncImageLookupFromAES, worker, &ImageWorker::ImageLookupFromAES);
        connect(worker, &ImageWorker::resultReady, this, &ImageController::result);
        connect(worker, SIGNAL(finished()), this, SLOT(finished_worker()));
        connect(this,SIGNAL(asyncImageLookupFromAES(QString,QString)),this,SLOT(asyncImageLookupFromAESslot(QString,QString)));
        workerThread.start(); 
    }
    ~ImageController()
    {
        workerThread.quit();
        workerThread.wait();
    }
public slots:
signals:
   // void asyncImageLookupFromAES(const QString &dirname, const QString &AEStext);
    void result(const QPixmap &);
    void asyncImageLookupFromAES(const QString &dirname, const QString &AEStext);//compressed
    void asyncImageLookupFromAES_worker(const QString &dirname, const QString &AEStext);//non compressed

private slots:

    void asyncImageLookupFromAESslot(const QString &dirname, const QString &AEStext)
    {
        if(busy)
        {
            havepeding=true;
            pedingdirname=dirname;
            pedingAEStext=AEStext;
            return;
        }
        emit asyncImageLookupFromAES_worker(dirname,AEStext);
        busy=true;
    }
    void finished_worker()
    {
        busy=false;
        if(havepeding)
        {
            emit asyncImageLookupFromAES_worker(pedingdirname,pedingAEStext);
            busy=true;
            havepeding=false;
        }
    }
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
    void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE;
    void showEvent(QShowEvent *event) Q_DECL_OVERRIDE;
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

    void dbUpdateslot(bool ok, int ref, const QStringList &dbitem);
    void dbUpdateerrorslot(const QString &error);

    void messagesliderchsnged(int value);

    void on_actionExport_log_triggered();

    void on_actionImport_log_triggered();

private:

    void dbUpdateUserClicked(bool ok, const QStringList &dbitem);
    void dbUpdateACARSMessage(bool ok, const QStringList &dbitem);

    class DBaseRequestSource : public DBase
    {
    public:
        enum Source
        {
            Unknown,
            UserClicked,
            ACARSMessage
        };
        Source source=Unknown;
        DBaseRequestSource(DBaseRequestSource::Source source):source(source){}
        DBaseRequestSource(){}
        operator Source() const {return source;}
    };
    PlaneLog::DBaseRequestSource dBaseRequestSourceUserCliecked=PlaneLog::DBaseRequestSource(DBaseRequestSource::Source::UserClicked);
    PlaneLog::DBaseRequestSource dBaseRequestSourceACARSMessage=PlaneLog::DBaseRequestSource(DBaseRequestSource::Source::ACARSMessage);

    Ui::PlaneLog *ui;
    int wantedheightofrow;
    QToolBar * toolBar;
    void updateinfopain();
    QList<int> savedsplitter2;

    ImageController *ic;
    DataBaseTextUser *dbc;

    double wantedscrollprop;

    QTableWidgetItem *selectedAESitem;

    ArincParse arincparser;

    int findAESrow(const QString &aes);
};

#endif // PLANELOG_H
