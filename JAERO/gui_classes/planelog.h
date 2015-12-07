#ifndef PLANELOG_H
#define PLANELOG_H

#include <QMainWindow>
#include <QThread>
#include "../aerol.h"

namespace Ui {
class PlaneLog;
}

/*class ImageWorker : public QObject {
    Q_OBJECT

public:
    ImageWorker();
    ~ImageWorker();

public slots:
    void process();

signals:
    void finished();
    void error(QString err);
    void imageResult(QPixmap image);
private:
    // add your variables here
};*/


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
    QString imagesfolder;
    QString planelookup;
protected:
    void closeEvent(QCloseEvent *event);
    void showEvent(QShowEvent *event);
    void contextMenuEvent(QContextMenuEvent *event) Q_DECL_OVERRIDE;
public slots:
    void ACARSslot(ACARSItem &acarsitem);

private slots:

    void on_actionClear_triggered();

    void on_actionUpDown_triggered();

    void on_actionLeftRight_triggered();

    void on_actionStopSorting_triggered();

    void on_tableWidget_currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn);

    void on_toolButtonimg_clicked();

    void on_actionCopy_triggered();

    void plainTextEditnotesChanged();

    void imageUpdateslot(const QPixmap &test);

private:
    Ui::PlaneLog *ui;
    int wantedheightofrow;
    QToolBar * toolBar;
    void updateinfopain(int row);
    QList<int> savedsplitter2;

    int updateinfoplanrow;

    ImageController *ic;


};

#endif // PLANELOG_H
